
#include <assert.h>
#include <string>
#include <fstream>
#include <streambuf>

#include "LayerPlannerTest.h"

#ifdef  DEBUG_ON
#define DEBUG(x...) printf(x...)
#else
#define DEBUG(x...)
#endif

using namespace sunxi;

HWC2::Composition string2Composition(const std::string& str)
{
    if (str == std::string("Device"))
        return HWC2::Composition::Device;
    if (str == std::string("Client"))
        return HWC2::Composition::Client;
    if (str == std::string("SolidColor"))
        return HWC2::Composition::SolidColor;
    if (str == std::string("Cursor"))
        return HWC2::Composition::Cursor;
    if (str == std::string("Invalid"))
        return HWC2::Composition::Invalid;

    return HWC2::Composition::Invalid;
}

HWC2::BlendMode string2BlendMode(const std::string& str)
{
    if (str == std::string("Premultiplied"))
        return HWC2::BlendMode::Premultiplied;
    if (str == std::string("Coverage"))
        return HWC2::BlendMode::Coverage;
    if (str == std::string("Invalid"))
        return HWC2::BlendMode::Invalid;
    if (str == std::string("None"))
        return HWC2::BlendMode::None;

    return HWC2::BlendMode::Invalid;
}

int32_t string2PixelFormat(const std::string& str)
{
    if (str == std::string("HAL_PIXEL_FORMAT_RGBA_8888"))
        return HAL_PIXEL_FORMAT_RGBA_8888;
    if (str == std::string("HAL_PIXEL_FORMAT_BGRA_8888"))
        return HAL_PIXEL_FORMAT_BGRA_8888;
    if (str == std::string("HAL_PIXEL_FORMAT_YV12"))
        return HAL_PIXEL_FORMAT_YV12;

    return HAL_PIXEL_FORMAT_RGBA_8888;
}


class JsonPaser {
public:
    static bool decodeExpectResult(const Json::Value& node, ExpectResult& rhs) {
        if (node.size() != 3) {
            return false;
        }
        rhs.Composition = string2Composition(node["Composition"].asString());
        rhs.Channel = node["Channel"].asInt();
        rhs.Slot = node["Slot"].asInt();
        return true;
    }

    static bool decodeFloatRect(const Json::Value& node, hwc_frect_t& rhs) {
        if(!node.isArray() || node.size() != 4) {
            return false;
        }
        rhs.left   = node[0].asFloat();
        rhs.top    = node[1].asFloat();
        rhs.right  = node[2].asFloat();
        rhs.bottom = node[3].asFloat();
        return true;
    }

    static bool decodeRect(const Json::Value& node, hwc_rect_t& rhs) {
        if(!node.isArray() || node.size() != 4) {
            return false;
        }
        rhs.left   = node[0].asInt();
        rhs.top    = node[1].asInt();
        rhs.right  = node[2].asInt();
        rhs.bottom = node[3].asInt();
        return true;
    }

    static bool decodeLayerProperty(const Json::Value& node, LayerProperty& rhs) {
        if(node.size() != 9) {
            return false;
        }
        rhs.PixelFormat = string2PixelFormat(node["PixelFormat"].asString());
        rhs.Zorder = node["Zorder"].asInt();
        rhs.Alpha = node["Alpha"].asFloat();
        rhs.BlendMode = (int32_t)string2BlendMode(node["BlendMode"].asString());
        rhs.Composition = (int32_t)string2Composition(node["Composition"].asString());
        rhs.Dataspace = node["Dataspace"].asInt();
        rhs.Transform = node["Transform"].asInt();
        JsonPaser::decodeFloatRect(node["Crop"], rhs.Crop);
        JsonPaser::decodeRect(node["DisplayFrame"], rhs.DisplayFrame);
        return true;
    }
};

void TestCaseLoader::dumpLayerInfo(const LayerProperty& prop, const ExpectResult& expect)
{
    printf("LayerProperty:\n"
           "  format %d z %u alpha %.2f blend %d composition %d dataspace %d transform %d\n"
           "  Crop  [%.2f %.2f %.2f %.2f]\n"
           "  Frame [%d %d %d %d]\n",
           prop.PixelFormat, prop.Zorder, prop.Alpha, prop.BlendMode, prop.Composition,
           prop.Dataspace, prop.Transform,
           prop.Crop.left, prop.Crop.top, prop.Crop.right, prop.Crop.bottom,
           prop.DisplayFrame.left, prop.DisplayFrame.top,
           prop.DisplayFrame.right, prop.DisplayFrame.bottom);
    printf("ExpectResult: Composition %d channel %d Slot %d\n",
            expect.Composition, expect.Channel, expect.Slot);
}

std::vector<std::shared_ptr<TestedLayer>>
TestCaseLoader::parseInputLayers(const Json::Value& layerList)
{
    std::vector<std::shared_ptr<TestedLayer>> out;
    for (Json::Value::const_iterator it = layerList.begin(); it != layerList.end(); ++it) {
        const Json::Value& lnode = *it;
        LayerProperty prop;
        ExpectResult  expect;
        JsonPaser::decodeLayerProperty(lnode["Property"], prop);
        JsonPaser::decodeExpectResult(lnode["Expect"], expect);
        std::shared_ptr<TestedLayer> layer = std::make_shared<TestedLayer>();
        layer->InputLayer = mLayerFactory.create(prop);
        layer->AssignedResult = expect;
        out.push_back(layer);
#ifdef DEBUG_ON
        dumpLayerInfo(prop, expect);
#endif
    }
    return out;
}

std::shared_ptr<TestCaseInfo> TestCaseLoader::parseCase(const Json::Value& cinfo)
{
    assert(cinfo.size() == 3);

    std::shared_ptr<TestCaseInfo> caseInfo = std::make_shared<TestCaseInfo>();
    caseInfo->Name = cinfo["CaseName"].asString();

    const Json::Value& inputLayers = cinfo["InputLayers"];
    caseInfo->mInputs = parseInputLayers(inputLayers);
    JsonPaser::decodeExpectResult(cinfo["ClientTarget"], caseInfo->mClientTargetAssignedResult);

    if (cinfo.isMember("ClientTargetProperty")) {
        LayerProperty prop;
        JsonPaser::decodeLayerProperty(cinfo["ClientTargetProperty"], prop);
        caseInfo->mClientTargetLayer = mLayerFactory.create(prop);
    } else {
        caseInfo->mClientTargetLayer = nullptr;
    }

    if (cinfo.isMember("BandwidthLimit")) {
        caseInfo->BandwidthLimit = cinfo["BandwidthLimit"].asInt();
        DEBUG("BandwidthLimit: %d\n", caseInfo->BandwidthLimit);
    }

    DEBUG("parse test case: %s total input layer %d\n", caseInfo->Name.c_str(), caseInfo->mInputs.size());
    return caseInfo;
}

void TestCaseLoader::load(const char *file)
{
    std::ifstream t(file);
    std::string json((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());

    Json::Reader read;
    Json::Value  root;

    if (!read.parse(json, root)) {
        if (!root.isObject()) {
            printf("parse json '%s' return error", file);
        }
        printf("Message: %s", read.getFormattedErrorMessages().c_str());
    }

    DEBUG("rootNode's size: %zd\n", root.size());
    for(Json::Value::iterator it = root.begin(); it != root.end(); ++it) {
        const Json::Value& lnode = *it;
        std::shared_ptr<TestCaseInfo> caseInfo = parseCase(lnode);
        mTestCaseList.push_back(caseInfo);
    }
}

