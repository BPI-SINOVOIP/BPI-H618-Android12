#pragma once

#include <ui/Rect.h>
#include <utils/RefBase.h>

namespace android {

class RegionListener : public virtual RefBase {
public:
    virtual int onUpdateRegion(const Rect& r, bool needForceRefresh = false) = 0;
    virtual void onUpdateBuffer()= 0;
};

};
