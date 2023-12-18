/*
 * Copyright (C) 2021 by Allwinnertech Co. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "C2Store"
//#define LOG_NDEBUG 0
#include "C2Store.h"

#include <C2AllocatorGralloc.h>
#include <C2AllocatorIon.h>
#include <C2BqBufferPriv.h>
#include <C2BufferPriv.h>
#include <C2Component.h>
#include <C2Config.h>
#include <C2PlatformStorePluginLoader.h>
#include <C2PlatformSupport.h>
#include <dlfcn.h>
#include <unistd.h>
#include <util/C2InterfaceHelper.h>
#include <utils/Log.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <cutils/properties.h>
#include "C2HwSupport.h"

namespace android {
c2_status_t C2AwComponentStore::ComponentModule::init(std::string libPath) {
  ALOGD("dlopen %s.", libPath.c_str());
  ALOGV("in %s", __func__);
  ALOGV("loading dll");
  mLibHandle = dlopen(libPath.c_str(), RTLD_NOW | RTLD_NODELETE);
  LOG_ALWAYS_FATAL_IF(mLibHandle == nullptr, "could not dlopen %s: %s",
                      libPath.c_str(), dlerror());

  createFactory = (C2ComponentFactory::CreateCodec2FactoryFunc)dlsym(
      mLibHandle, "CreateCodec2Factory");
  LOG_ALWAYS_FATAL_IF(createFactory == nullptr, "createFactory is null in %s",
                      libPath.c_str());

  destroyFactory = (C2ComponentFactory::DestroyCodec2FactoryFunc)dlsym(
      mLibHandle, "DestroyCodec2Factory");
  LOG_ALWAYS_FATAL_IF(destroyFactory == nullptr, "destroyFactory is null in %s",
                      libPath.c_str());

  mComponentFactory = createFactory();
  if (mComponentFactory == nullptr) {
    ALOGD("could not create factory in %s", libPath.c_str());
    mInit = C2_NO_MEMORY;
  } else {
    mInit = C2_OK;
  }

  if (mInit != C2_OK) {
    return mInit;
  }

  std::shared_ptr<C2ComponentInterface> intf;
  c2_status_t res = createInterface(0, &intf);
  if (res != C2_OK) {
    ALOGD("failed to create interface: %d", res);
    return mInit;
  }

  std::shared_ptr<C2Component::Traits> traits(new (std::nothrow)
                                                  C2Component::Traits);
  if (traits) {
    traits->name = intf->getName();

    C2ComponentKindSetting kind;
    C2ComponentDomainSetting domain;
    res = intf->query_vb({&kind, &domain}, {}, C2_MAY_BLOCK, nullptr);
    bool fixDomain = res != C2_OK;
    if (res == C2_OK) {
      traits->kind = kind.value;
      traits->domain = domain.value;
    } else {
      ALOGD("failed to query interface for kind and domain: %d", res);

      traits->kind = (traits->name.find("encoder") != std::string::npos)
                         ? C2Component::KIND_ENCODER
                         : (traits->name.find("decoder") != std::string::npos)
                               ? C2Component::KIND_DECODER
                               : C2Component::KIND_OTHER;
    }

    uint32_t mediaTypeIndex = traits->kind == C2Component::KIND_ENCODER
                                  ? C2PortMediaTypeSetting::output::PARAM_TYPE
                                  : C2PortMediaTypeSetting::input::PARAM_TYPE;
    std::vector<std::unique_ptr<C2Param>> params;
    res = intf->query_vb({}, {mediaTypeIndex}, C2_MAY_BLOCK, &params);
    if (res != C2_OK) {
      ALOGD("failed to query interface: %d", res);
      return mInit;
    }
    if (params.size() != 1u) {
      ALOGD("failed to query interface: unexpected vector size: %zu",
            params.size());
      return mInit;
    }
    C2PortMediaTypeSetting *mediaTypeConfig =
        C2PortMediaTypeSetting::From(params[0].get());
    if (mediaTypeConfig == nullptr) {
      ALOGD("failed to query media type");
      return mInit;
    }
    traits->mediaType = std::string(
        mediaTypeConfig->m.value,
        strnlen(mediaTypeConfig->m.value, mediaTypeConfig->flexCount()));

    if (fixDomain) {
      if (strncmp(traits->mediaType.c_str(), "audio/", 6) == 0) {
        traits->domain = C2Component::DOMAIN_AUDIO;
      } else if (strncmp(traits->mediaType.c_str(), "video/", 6) == 0) {
        traits->domain = C2Component::DOMAIN_VIDEO;
      } else if (strncmp(traits->mediaType.c_str(), "image/", 6) == 0) {
        traits->domain = C2Component::DOMAIN_IMAGE;
      } else {
        traits->domain = C2Component::DOMAIN_OTHER;
      }
    }

    switch (traits->domain) {
      case C2Component::DOMAIN_AUDIO:
        traits->rank = 8;
        break;
      default:
        traits->rank = 512;
    }

    params.clear();
    res = intf->query_vb({}, {C2ComponentAliasesSetting::PARAM_TYPE},
                         C2_MAY_BLOCK, &params);
    if (res == C2_OK && params.size() == 1u) {
      C2ComponentAliasesSetting *aliasesSetting =
          C2ComponentAliasesSetting::From(params[0].get());
      if (aliasesSetting) {
        // Split aliases on ','
        // This looks simpler in plain C and even std::string would still
        // make a copy.
        char *aliases =
            ::strndup(aliasesSetting->m.value, aliasesSetting->flexCount());
        ALOGD("'%s' has aliases: '%s'", intf->getName().c_str(), aliases);

        for (char *tok, *ptr, *str = aliases;
             (tok = ::strtok_r(str, ",", &ptr)); str = nullptr) {
          traits->aliases.push_back(tok);
          ALOGD("adding alias: '%s'", tok);
        }
        free(aliases);
      }
    }
  }
  mTraits = traits;

  return mInit;
}

C2AwComponentStore::ComponentModule::~ComponentModule() {
  ALOGV("in %s", __func__);
  if (destroyFactory && mComponentFactory) {
    destroyFactory(mComponentFactory);
  }
  if (mLibHandle) {
    ALOGV("unloading dll");
    dlclose(mLibHandle);
  }
}

c2_status_t C2AwComponentStore::ComponentModule::createInterface(
    c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *interface,
    std::function<void(::C2ComponentInterface *)> deleter) {
  interface->reset();
  if (mInit != C2_OK) {
    return mInit;
  }
  std::shared_ptr<ComponentModule> module = shared_from_this();
  c2_status_t res = mComponentFactory->createInterface(
      id, interface, [module, deleter](C2ComponentInterface *p) mutable {
        // capture module so that we ensure we still have it while deleting
        // interface
        deleter(p);      // delete interface first
        module.reset();  // remove module ref (not technically needed)
      });
  return res;
}

c2_status_t C2AwComponentStore::ComponentModule::createComponent(
    c2_node_id_t id, std::shared_ptr<C2Component> *component,
    std::function<void(::C2Component *)> deleter) {
  component->reset();
  if (mInit != C2_OK) {
    return mInit;
  }
  ALOGD("function %s. line %d.", __func__, __LINE__);
  std::shared_ptr<ComponentModule> module = shared_from_this();
  c2_status_t res = mComponentFactory->createComponent(
      id, component, [module, deleter](C2Component *p) mutable {
        // capture module so that we ensure we still have it while deleting
        // component
        deleter(p);      // delete component first
        module.reset();  // remove module ref (not technically needed)
      });
  return res;
}

std::shared_ptr<const C2Component::Traits>
C2AwComponentStore::ComponentModule::getTraits() {
  std::unique_lock<std::recursive_mutex> lock(mLock);
  return mTraits;
}

C2AwComponentStore::C2AwComponentStore()
    : mVisited(false),
      mReflector(std::make_shared<C2ReflectorHelper>()),
      mInterface(mReflector) {
  bool mRemoveHardwareCodec2 = false;
  char value[PROPERTY_VALUE_MAX];
  property_get("vendor.codec2.disable.hw", value, "0");
  ALOGD("%s value = %d", __func__, atoi(value));
  if (atoi(value) == 1) {
    mRemoveHardwareCodec2 = true;
  }
  auto emplace = [this](const char *libPath) {
    mComponents.emplace(libPath, libPath);
  };
  if (!mRemoveHardwareCodec2) {
    ALOGD("load normal libs");
    emplace("libcodec2_hw_avcdec.so");
    emplace("libcodec2_hw_hevcdec.so");
    emplace("libcodec2_hw_mpeg2dec.so");
    emplace("libcodec2_hw_mpeg4dec.so");
    emplace("libcodec2_hw_h263dec.so");
    emplace("libcodec2_hw_s263dec.so");
    emplace("libcodec2_hw_mjpegdec.so");
    emplace("libcodec2_hw_mpeg1dec.so");
    emplace("libcodec2_hw_xviddec.so");
    emplace("libcodec2_hw_avcenc.so");
    // emplace("libcodec2_hw_vp8dec.so");
    // emplace("libcodec2_hw_vp9dec.so");
  } else {
    ALOGD("do not load normal libs");
  }
  emplace("libcodec2_hw_av1dec.so");
}

c2_status_t C2AwComponentStore::copyBuffer(
    std::shared_ptr<C2GraphicBuffer> /*src*/,
    std::shared_ptr<C2GraphicBuffer> /*dst*/) {
  return C2_OMITTED;
}

c2_status_t C2AwComponentStore::query_sm(
    const std::vector<C2Param *> &stackParams,
    const std::vector<C2Param::Index> &heapParamIndices,
    std::vector<std::unique_ptr<C2Param>> *const heapParams) const {
  return mInterface.query(stackParams, heapParamIndices, C2_MAY_BLOCK,
                          heapParams);
}

c2_status_t C2AwComponentStore::config_sm(
    const std::vector<C2Param *> &params,
    std::vector<std::unique_ptr<C2SettingResult>> *const failures) {
  return mInterface.config(params, C2_MAY_BLOCK, failures);
}

void C2AwComponentStore::visitComponents() {
  std::lock_guard<std::mutex> lock(mMutex);
  if (mVisited) {
    return;
  }
  for (auto &pathAndLoader : mComponents) {
    const C2String &path = pathAndLoader.first;
    ComponentLoader &loader = pathAndLoader.second;
    std::shared_ptr<ComponentModule> module;
    if (loader.fetchModule(&module) == C2_OK) {
      std::shared_ptr<const C2Component::Traits> traits = module->getTraits();
      if (traits) {
        mComponentList.push_back(traits);
        mComponentNameToPath.emplace(traits->name, path);
        for (const C2String &alias : traits->aliases) {
          mComponentNameToPath.emplace(alias, path);
        }
      }
    }
  }
  mVisited = true;
}

std::vector<std::shared_ptr<const C2Component::Traits>>
C2AwComponentStore::listComponents() {
  // This method SHALL return within 500ms.
  visitComponents();
  return mComponentList;
}

c2_status_t C2AwComponentStore::findComponent(
    C2String name, std::shared_ptr<ComponentModule> *module) {
  (*module).reset();
  visitComponents();

  auto pos = mComponentNameToPath.find(name);
  if (pos != mComponentNameToPath.end()) {
    return mComponents.at(pos->second).fetchModule(module);
  }
  return C2_NOT_FOUND;
}

c2_status_t C2AwComponentStore::createComponent(
    C2String name, std::shared_ptr<C2Component> *const component) {
  // This method SHALL return within 100ms.
  component->reset();
  std::shared_ptr<ComponentModule> module;
  c2_status_t res = findComponent(name, &module);
  if (res == C2_OK) {
    res = module->createComponent(0, component);
  }
  return res;
}

c2_status_t C2AwComponentStore::createInterface(
    C2String name, std::shared_ptr<C2ComponentInterface> *const interface) {
  // This method SHALL return within 100ms.
  interface->reset();
  std::shared_ptr<ComponentModule> module;
  c2_status_t res = findComponent(name, &module);
  if (res == C2_OK) {
    res = module->createInterface(0, interface);
  }
  return res;
}

c2_status_t C2AwComponentStore::querySupportedParams_nb(
    std::vector<std::shared_ptr<C2ParamDescriptor>> *const params) const {
  return mInterface.querySupportedParams(params);
}

c2_status_t C2AwComponentStore::querySupportedValues_sm(
    std::vector<C2FieldSupportedValuesQuery> &fields) const {
  return mInterface.querySupportedValues(fields, C2_MAY_BLOCK);
}

C2String C2AwComponentStore::getName() const {
  return "android.componentStore.allwinner";
}

std::shared_ptr<C2ParamReflector> C2AwComponentStore::getParamReflector()
    const {
  ALOGD("kay: return allwinner mReflector");
  return mReflector;
}

std::shared_ptr<C2ComponentStore> GetCodec2HwComponentStore() {
  ALOGD("kay: to GetCodec2HwComponentStore");
  static std::mutex mutex;
  static std::weak_ptr<C2ComponentStore> hwStore;
  std::lock_guard<std::mutex> lock(mutex);
  std::shared_ptr<C2ComponentStore> store = hwStore.lock();
  if (store == nullptr) {
    ALOGD("kay: to make_shared C2AwComponentStore");
    store = std::make_shared<C2AwComponentStore>();
    hwStore = store;
  }
  return store;
}

}  // namespace android
