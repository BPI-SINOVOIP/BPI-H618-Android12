/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef _PERSIST_ATTR_H_
#define _PERSIST_ATTR_H_

#include <utils/String8.h>
#include "persist_property.h"
#include "hardware/sunxi_display2.h"
#include "sunxi_typedef.h"

template <typename T>
class PersistAttr : public PersistProperty {
public:
    PersistAttr(android::String8& path);
    int save(const T& rhs);
    int load();
    const T& get() const;
    void setHook(void (*func)(const T&));
    void dump(android::String8& out) const;
private:
    T mValue;
    void (*mHook)(const T& rhs);
};

template <typename T>
PersistAttr<T>::PersistAttr(android::String8& path)
  : PersistProperty(path.string()),
    mHook(nullptr) {
    load();
}

template <typename T>
int PersistAttr<T>::load() {
    char tmp[64];
    read(tmp, 64);
    unflatten(tmp, mValue);
    return 0;
}

template <typename T>
int PersistAttr<T>::save(const T& rhs) {
    if (rhs == mValue)
        return 0;
    if (mHook)
        mHook(rhs);
    mValue = rhs;
    char tmp[64] = {0};
    flatten(tmp, mValue);
    return write(tmp);
}

template <typename T>
const T& PersistAttr<T>::get() const {
    return mValue;
}

template <typename T>
void PersistAttr<T>::setHook(void (*func)(const T&)) {
    mHook = func;
}

template <typename T>
void PersistAttr<T>::dump(android::String8& out) const {
    out.append(toString(mValue));
}

/* device config define start > -------------------------------------------------- */
inline bool operator==(const disp_device_config& lhs, const disp_device_config& rhs) {
    if ((lhs.type != rhs.type) || (lhs.mode != rhs.mode) ||
            (lhs.format != rhs.format) || (lhs.bits != rhs.bits) ||
            (lhs.eotf != rhs.eotf) || (lhs.cs != rhs.cs))
        return false;
    return true;
}

inline android::String8 toString(const disp_device_config& rhs) {
    android::String8 out;
    out.appendFormat("device config: type=%d mode=%d format=%d bits=%d eotf=%d cs=%d",
                     rhs.type, rhs.mode, rhs.format, rhs.bits, rhs.eotf, rhs.cs);
    return out;
}

inline void flatten(char *buf, const disp_device_config& rhs) {
    sprintf(buf, "%d,%d,%d,%d,%d,%d",
            rhs.type, rhs.mode, rhs.format, rhs.bits, rhs.eotf, rhs.cs);
}

inline int unflatten(const char *buf, disp_device_config& lhs) {
    int tmp[6];
    int count = sscanf(buf, "%d,%d,%d,%d,%d,%d",
                       &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);
    if (count != 6) {
        return 0;
    }
    lhs.type   = (enum disp_output_type)tmp[0];
    lhs.mode   = (enum disp_tv_mode)tmp[1];
    lhs.format = (enum disp_csc_type)tmp[2];
    lhs.bits   = (enum disp_data_bits)tmp[3];
    lhs.eotf   = (enum disp_eotf)tmp[4];
    lhs.cs     = (enum disp_color_space)tmp[5];
    return 0;
}
/* device config define end > ---------------------------------------------------- */

/* Margin define start > --------------------------------------------------------- */
struct Margin {
    int left, right;
    int top,  bottom;
};

inline bool operator==(const Margin& lhs, const Margin& rhs) {
    if ((lhs.left != rhs.left) || (lhs.right != rhs.right) ||
            (lhs.top != rhs.top) || (lhs.bottom != rhs.bottom))
        return false;
    return true;
}

inline android::String8 toString(const Margin& rhs) {
    android::String8 out;
    out.appendFormat("Margin: left=%d right=%d top=%d bottom=%d",
                     rhs.left, rhs.right, rhs.top, rhs.bottom);
    return out;
}

inline void flatten(char *buf, const Margin& rhs) {
    sprintf(buf, "%d,%d,%d,%d",
            rhs.left, rhs.right, rhs.top, rhs.bottom);
}

inline int unflatten(const char *buf, Margin& lhs) {
    int tmp[4];
    int count = sscanf(buf, "%d,%d,%d,%d",
                       &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
    if (count != 4) {
        lhs.left = lhs.right = lhs.top = lhs.bottom = 100;
        return 0;
    }
    lhs.left   = tmp[0];
    lhs.right  = tmp[1];
    lhs.top    = tmp[2];
    lhs.bottom = tmp[3];
    return 0;
}
/* Margin define end > ----------------------------------------------------------- */

/* dataspace define start > ------------------------------------------------------ */

inline android::String8 toString(const dataspace_t& rhs) {
    switch (rhs) {
    case DATASPACE_MODE_AUTO : return android::String8("dataspace: AUTO ");
    case DATASPACE_MODE_HDR  : return android::String8("dataspace: HDR  ");
    case DATASPACE_MODE_WCG  : return android::String8("dataspace: WCG  ");
    case DATASPACE_MODE_SDR  : return android::String8("dataspace: SDR  ");
    case DATASPACE_MODE_OTHER: return android::String8("dataspace: OTHRE");
    default: return android::String8("dataspace: unknow");
    }
}

inline void flatten(char *buf, const dataspace_t& rhs) {
    sprintf(buf, "%d", rhs);
}

inline int unflatten(const char *buf, dataspace_t& lhs) {
    int count = sscanf(buf, "%d", &lhs);
    if (count != 1)
        lhs = DATASPACE_MODE_AUTO;
    return 0;
}
/* dataspace define end > -------------------------------------------------------- */

/* pixelformat define start > ---------------------------------------------------- */

inline android::String8 toString(const pixelformat_t& rhs) {
    switch (rhs) {
    case PIXEL_FORMAT_AUTO:         return android::String8("pixelformat: AUTO ");
    case PIXEL_FORMAT_YUV422_10bit: return android::String8("pixelformat: YUV422 10bit");
    case PIXEL_FORMAT_YUV420_10bit: return android::String8("pixelformat: YUV420 10bit");
    case PIXEL_FORMAT_YUV444_8bit:  return android::String8("pixelformat: YUV444 8bit");
    case PIXEL_FORMAT_RGB888_8bit:  return android::String8("pixelformat: RGB888 8bit");
    default: return android::String8("pixelformat: unknow");
    }
}

inline void flatten(char *buf, const pixelformat_t& rhs) {
    sprintf(buf, "%d", rhs);
}

inline int unflatten(const char *buf, pixelformat_t& lhs) {
    int count = sscanf(buf, "%d", &lhs);
    if (count != 1)
        lhs = PIXEL_FORMAT_AUTO;
    return 0;
}
/* pixelformat define end > ------------------------------------------------------ */

inline android::String8 toString(const aspect_ratio_t& rhs) {
    switch (rhs) {
    case ASPECT_RATIO_AUTO:       return android::String8("aspect ratio: AUTO ");
    case ASPECT_RATIO_FULL:       return android::String8("aspect ratio: FULL");
    case ASPECT_RATIO_4_3:        return android::String8("aspect ratio: 4:3");
    case ASPECT_RATIO_16_9:       return android::String8("aspect ratio: 16:9");
    case ASPECT_RATIO_WIDTH:      return android::String8("aspect ratio: WIDTH FIRST");
    case ASPECT_RATIO_HEIGHT:     return android::String8("aspect ratio: HEIGHT FIRST");
    case ASPECT_RATIO_FULL_ONCE:  return android::String8("aspect ratio: FULL ONCE");
    case ASPECT_RATIO_RATIO_LOAD: return android::String8("aspect ratio: RESET");
    default: return android::String8("aspect ratio: unknow");
    }
}

inline void flatten(char *buf, const aspect_ratio_t& rhs) {
    sprintf(buf, "%d", rhs);
}

inline int unflatten(const char *buf, aspect_ratio_t& lhs) {
    int count = sscanf(buf, "%d", &lhs);
    if (count != 1)
        lhs = ASPECT_RATIO_AUTO;
    return 0;
}

#define _device_congi_property(name) (android::String8("persist.disp.device_config.")+=(name))
#define _margin_property(name)       (android::String8("persist.disp.margin.")+=(name))
#define _dataspace_property(name)    (android::String8("persist.disp.dataspace.")+=(name))
#define _pixelformat_property(name)  (android::String8("persist.disp.pixelformat.")+=(name))
#define _aspect_ratio_property(name) (android::String8("persist.disp.aspectratio.")+=(name))

#endif
