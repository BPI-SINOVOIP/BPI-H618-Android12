// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanDispatch.h"

#include "base/PathUtils.h"
#include "base/System.h"
#include "base/Lock.h"
#include "base/SharedLibrary.h"

using android::base::AutoLock;
using android::base::Lock;
using android::base::pj;

namespace emugl {

static void setIcdPath(const std::string& path) {
    if (android::base::pathExists(path.c_str())) {
        // LOG(VERBOSE) << "setIcdPath: path exists: " << path;
    } else {
        // LOG(VERBOSE) << "setIcdPath: path doesn't exist: " << path;
    }
    android::base::setEnvironmentVariable("VK_ICD_FILENAMES", path);
}

static std::string icdJsonNameToProgramAndLauncherPaths(
        const std::string& icdFilename) {

    std::string suffix = pj({"lib64", "vulkan", icdFilename});

    return pj({android::base::getProgramDirectory(), suffix}) + ":" +
           pj({android::base::getLauncherDirectory(), suffix});
}

static void initIcdPaths(bool forTesting) {
    auto androidIcd = android::base::getEnvironmentVariable("ANDROID_EMU_VK_ICD");
    android::base::setEnvironmentVariable("ANDROID_EMU_SANDBOX", "1");
    if (android::base::getEnvironmentVariable("ANDROID_EMU_SANDBOX") == "1") {
        // Rely on user to set VK_ICD_FILENAMES
        return;
    } else {
        if (forTesting || androidIcd == "swiftshader") {
            auto res = pj({android::base::getProgramDirectory(), "lib64", "vulkan"});
            // LOG(VERBOSE) << "In test environment or ICD set to swiftshader, using "
                            "Swiftshader ICD";
            auto libPath = pj({android::base::getProgramDirectory(), "lib64", "vulkan", "libvk_swiftshader.so"});;
            if (android::base::pathExists(libPath.c_str())) {
                // LOG(VERBOSE) << "Swiftshader library exists";
            } else {
                // LOG(VERBOSE) << "Swiftshader library doesn't exist, trying launcher path";
                libPath = pj({android::base::getLauncherDirectory(), "lib64", "vulkan", "libvk_swiftshader.so"});;
                if (android::base::pathExists(libPath.c_str())) {
                    // LOG(VERBOSE) << "Swiftshader library found in launcher path";
                } else {
                    // LOG(VERBOSE) << "Swiftshader library not found in program nor launcher path";
                }
            }
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
            android::base::setEnvironmentVariable("ANDROID_EMU_VK_ICD", "swiftshader");
        } else {
            // LOG(VERBOSE) << "Not in test environment. ICD (blank for default): ["
                         // << androidIcd << "]";
            // Mac: Use MoltenVK by default unless GPU mode is set to swiftshader,
            // and switch between that and gfx-rs libportability-icd depending on
            // the environment variable setting.
    #ifdef __APPLE__
            if (androidIcd == "portability") {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos.json"));
            } else if (androidIcd == "portability-debug") {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos-debug.json"));
            } else {
                if (androidIcd == "swiftshader" ||
                    emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER ||
                    emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
                    setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
                    android::base::setEnvironmentVariable("ANDROID_EMU_VK_ICD", "swiftshader");
                } else {
                    setIcdPath(icdJsonNameToProgramAndLauncherPaths("MoltenVK_icd.json"));
                    android::base::setEnvironmentVariable("ANDROID_EMU_VK_ICD", "moltenvk");
                }
            }
#else
            // By default, on other platforms, just use whatever the system
            // is packing.
#endif
        }
    }
}

#ifdef __APPLE__
#define VULKAN_LOADER_FILENAME "libvulkan.dylib"
#else
#ifdef _WIN32
#define VULKAN_LOADER_FILENAME "vulkan-1.dll"
#else
#define VULKAN_LOADER_FILENAME "libvulkan.so"
#endif

#endif
static std::string getLoaderPath(const std::string& directory, bool forTesting) {
    auto path = android::base::getEnvironmentVariable("ANDROID_EMU_VK_LOADER_PATH");
    if (!path.empty()) {
        return path;
    }
    auto androidIcd = android::base::getEnvironmentVariable("ANDROID_EMU_VK_ICD");
    if (forTesting || androidIcd == "mock") {
        auto path = pj({directory, "testlib64", VULKAN_LOADER_FILENAME});
        // LOG(VERBOSE) << "In test environment or using Swiftshader. Using loader: " << path;
        return path;
    } else {
#ifdef _WIN32
        // LOG(VERBOSE) << "Not in test environment. Using loader: " << VULKAN_LOADER_FILENAME;
        return VULKAN_LOADER_FILENAME;
#else
        auto path = pj({directory, "lib64", "vulkan", VULKAN_LOADER_FILENAME});
        // LOG(VERBOSE) << "Not in test environment. Using loader: " << path;
        return path;
#endif
    }
}

#ifdef __APPLE_
static std::string getMoltenVkPath(const std::string& directory, bool forTesting) {
    auto path = android::base::getEnvironmentVariable("ANDROID_EMU_VK_LOADER_PATH");
    if (!path.empty()) {
        return path;
    }
    auto androidIcd = android::base::getEnvironmentVariable("ANDROID_EMU_VK_ICD");

    // Skip loader when using MoltenVK as this gives us access to
    // VK_MVK_moltenvk, which is required for external memory support.
    if (!forTesting && androidIcd == "moltenvk") {
        auto path = pj({directory, "lib64", "vulkan", "libMoltenVK.dylib"});
        LOG(VERBOSE) << "Skipping loader and using ICD directly: " << path;
        return path;
    }
    return "";
}
#endif

class SharedLibraries {
public:
    explicit SharedLibraries(size_t sizeLimit = 1) : mSizeLimit(sizeLimit) {}

    size_t size() const { return mLibs.size(); }

    bool addLibrary(const std::string& path) {
        if (size() >= mSizeLimit) {
            fprintf(stderr, "cannot add library %s: full\n", path.c_str());
            return false;
        }

        auto library = android::base::SharedLibrary::open(path.c_str());
        if (library) {
            mLibs.push_back(library);
            fprintf(stderr, "added library %s\n", path.c_str());
            return true;
        }
        else {
            fprintf(stderr, "cannot add library %s: failed\n", path.c_str());
            return false;
        }
    }

    ~SharedLibraries() = default;

    void* dlsym(const char* name) {
        for (const auto& lib : mLibs) {
            void* funcPtr = reinterpret_cast<void*>(lib->findSymbol(name));
            if (funcPtr) {
                return funcPtr;
            }
        }
        return nullptr;
    }

private:
    size_t mSizeLimit;
    std::vector<android::base::SharedLibrary*> mLibs;
};

static constexpr size_t getVulkanLibraryNumLimits() {
    // macOS may have both Vulkan loader (for non MoltenVK-specific functions) and
    // MoltenVK library (only for MoltenVK-specific vk...MVK functions) loaded at
    // the same time. So there could be at most 2 libraries loaded. On other systems
    // only one Vulkan loader is allowed.
#ifdef __APPLE__
    return 2;
#else
    return 1;
#endif
}

class VulkanDispatchImpl {
public:
    VulkanDispatchImpl() : mVulkanLibs(getVulkanLibraryNumLimits()) {}

    void initialize(bool forTesting);

    void* dlopen() {
        bool sandbox = android::base::getEnvironmentVariable("ANDROID_EMU_SANDBOX") == "1";

        if (mVulkanLibs.size() == 0) {
            if (sandbox) {
#ifdef __linux__
                bool success = mVulkanLibs.addLibrary(VULKAN_LOADER_FILENAME);
                if (!success) {
                    mVulkanLibs.addLibrary("libvulkan.so.1");
                }
#else
                mVulkanLibs.addLibrary(VULKAN_LOADER_FILENAME);
#endif // __linux__
            }
            else {
                auto loaderPath = getLoaderPath(android::base::getProgramDirectory(), mForTesting);
                bool success = mVulkanLibs.addLibrary(loaderPath);

                if (!success) {
                    loaderPath = getLoaderPath(android::base::getLauncherDirectory(), mForTesting);
                    mVulkanLibs.addLibrary(loaderPath);
                }

#ifdef __linux__
                // On Linux, it might not be called libvulkan.so.
                // Try libvulkan.so.1 if that doesn't work.
                if (!success) {
                    loaderPath = pj({android::base::getLauncherDirectory(), "lib64", "vulkan", "libvulkan.so.1"});
                    mVulkanLibs.addLibrary(loaderPath);
                }
#endif // __linux__
#ifdef __APPLE__
                // On macOS it is possible that we are using MoltenVK as the
                // ICD. In that case we need to add MoltenVK libraries to
                // mSharedLibs to use MoltenVK-specific functions.
                auto mvkPath = getMoltenVkPath(android::base::getProgramDirectory(), mForTesting);
                if (!mvkPath.empty()) {
                    success = mVulkanLibs.addLibrary(mvkPath);
                }

                if (!success) {
                    mvkPath = getMoltenVkPath(android::base::getLauncherDirectory(), mForTesting);
                    if (!mvkPath.empty()) {
                        success = mVulkanLibs.addLibrary(mvkPath);
                    }
                }
#endif // __APPLE__
            }
        }
        return static_cast<void*>(&mVulkanLibs);
    }

    void* dlsym(void* lib, const char* name) {
        return (void*)((emugl::SharedLibraries*)(lib))->dlsym(name);
    }

    VulkanDispatch* dispatch() { return &mDispatch; }

private:
    Lock mLock;
    bool mForTesting = false;
    bool mInitialized = false;
    VulkanDispatch mDispatch;
    SharedLibraries mVulkanLibs;
};

VulkanDispatchImpl* sVulkanDispatchImpl() {
    static VulkanDispatchImpl* impl = new VulkanDispatchImpl;
    return impl;
}

static void* sVulkanDispatchDlOpen()  {
    return sVulkanDispatchImpl()->dlopen();
}

static void* sVulkanDispatchDlSym(void* lib, const char* sym) {
    return sVulkanDispatchImpl()->dlsym(lib, sym);
}

void VulkanDispatchImpl::initialize(bool forTesting) {
    AutoLock lock(mLock);

    if (mInitialized) {
        return;
    }

    mForTesting = forTesting;
    initIcdPaths(mForTesting);

    goldfish_vk::init_vulkan_dispatch_from_system_loader(
            sVulkanDispatchDlOpen,
            sVulkanDispatchDlSym,
            &mDispatch);

    mInitialized = true;
}

VulkanDispatch* vkDispatch(bool forTesting) {
    sVulkanDispatchImpl()->initialize(forTesting);
    return sVulkanDispatchImpl()->dispatch();
}

bool vkDispatchValid(const VulkanDispatch* vk) {
    return vk->vkEnumerateInstanceExtensionProperties != nullptr ||
           vk->vkGetInstanceProcAddr != nullptr ||
           vk->vkGetDeviceProcAddr != nullptr;
}

}
