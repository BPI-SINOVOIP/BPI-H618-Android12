
#ifndef _LAYER_PLANNER_TEST_H_
#define _LAYER_PLANNER_TEST_H_

#include <memory>
#include <vector>
#include <json/json.h>

#include "Layer.h"
#include "LayerFactory.h"

using namespace sunxi;

struct ExpectResult {
    HWC2::Composition Composition;
    int Channel;
    int Slot;
};

struct TestedLayer {
    std::shared_ptr<Layer> InputLayer;
    ExpectResult AssignedResult;
};

class TestCaseInfo {
public:
    std::string Name;
    std::vector<std::shared_ptr<TestedLayer>> mInputs;
    std::shared_ptr<Layer> mClientTargetLayer;
    ExpectResult mClientTargetAssignedResult;
    int BandwidthLimit = 0;
};

class TestCaseLoader {
public:
    void load(const char* file);
    std::vector<std::shared_ptr<TestCaseInfo>> getTestCaseList() const {
        return mTestCaseList;
    }

private:
    std::shared_ptr<TestCaseInfo> parseCase(const Json::Value& cinfo);
    std::vector<std::shared_ptr<TestedLayer>> parseInputLayers(const Json::Value& layerList);
    ExpectResult parseExpectResult(const Json::Value& cinfo);
    static void dumpLayerInfo(const LayerProperty& prop, const ExpectResult& expect);

private:
    LayerFactory mLayerFactory;
    std::vector<std::shared_ptr<TestCaseInfo>> mTestCaseList;
};

#endif

