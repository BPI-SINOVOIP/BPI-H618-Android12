
#include <memory>
#include <gtest/gtest.h>
#include "Display.h"
#include "LayerPlanner.h"
#include "Debug.h"

#include "private_handle.h"
#include "LayerPlannerTest.h"

using namespace sunxi;

#define GTEST_COUT std::cerr << "[ INFO     ] "

std::unique_ptr<TestCaseLoader> CaseLoader;
std::unique_ptr<TestCaseLoader> MemoryLimitCaseLoader;
std::unique_ptr<TestCaseLoader> HybridChannelWithOutAlphaCaseLoader;

class ParamLayerPlannerTest:
        public ::testing::TestWithParam<std::shared_ptr<TestCaseInfo>>
{
protected:
    virtual void SetUp() override
    {
        GTEST_COUT << "ParamLayerPlannerTest SetUp" << std::endl;

        mLayerFactory = std::make_shared<LayerFactory>();

        Debug::get().setDebugTag((0x01 << kTagStrategy), true);
        Planner = std::make_shared<LayerPlanner>();

        // Constraint
        mPlanConstraint.MaxHybridChannel = 1;
        mPlanConstraint.MaxRGBChannel    = 3;
        mPlanConstraint.MaxBandwidth     = 0;
        mPlanConstraint.Output3D         = false;
        mPlanConstraint.HybridChannelWithAlpha = true;

        LayerProperty layerpro = {
            .PixelFormat = HAL_PIXEL_FORMAT_RGBA_8888,
            .Zorder = 0xff,
            .Alpha = 1.0f,
            .BlendMode = HWC2_BLEND_MODE_PREMULTIPLIED,
            .Composition = HWC2_COMPOSITION_DEVICE,
            .Dataspace = 0,
            .Transform = 0,
            .Crop = {0.0f, 0.0f, 1280.0f, 720.0f},
            .DisplayFrame = {0, 0, 1280, 720},
        };
        mClientTarget = mLayerFactory->create(layerpro);
        GTEST_COUT << "Client target: " << mClientTarget.get() << std::endl;
    }

    void buildCompositionContext(std::shared_ptr<TestCaseInfo> caseinfo, std::shared_ptr<Layer> fb = nullptr)
    {
        mContext.InputLayers.clear();

        for (int i = 0; i < caseinfo->mInputs.size(); i++) {
            auto layer = caseinfo->mInputs[i]->InputLayer;
            layer->setValidatedType(HWC2::Composition::Invalid);
            mContext.InputLayers.emplace(layer);
        }
        mClientTarget->setValidatedType(HWC2::Composition::Invalid);
        if (fb == nullptr) {
            mContext.FramebufferTarget = mClientTarget;
        } else {
            mContext.FramebufferTarget = fb;
        }
    }

    void verifyAssignedResult(std::shared_ptr<Layer>& layer, ExpectResult& expect)
    {
        ASSERT_EQ(layer->compositionFromValidated(), expect.Composition);

        int count = mOutput.Tracks.count(layer->id());
        ASSERT_EQ(count, 1);

        const TrackNode& track = mOutput.Tracks.at(layer->id());
        int channel = track.Channel != nullptr ? track.Channel->Index : -1;

        ASSERT_EQ(channel, expect.Channel);
        ASSERT_EQ(track.Slot, expect.Slot);
    }

protected:
    std::shared_ptr<LayerFactory> mLayerFactory;
    std::shared_ptr<LayerPlanner> Planner;
    PlanConstraint mPlanConstraint;

    CompositionContext mContext;
    PlanningOutput mOutput;
    std::shared_ptr<Layer> mClientTarget;
};

std::string generateTestCaseName(
        ::testing::TestParamInfo<std::shared_ptr<TestCaseInfo>> caseinfo)
{
    return caseinfo.param->Name;
}

INSTANTIATE_TEST_CASE_P(
        TestGroup,
        ParamLayerPlannerTest,
        testing::ValuesIn(CaseLoader->getTestCaseList()),
        generateTestCaseName);

TEST_P(ParamLayerPlannerTest, AutoTestcase)
{
    std::shared_ptr<TestCaseInfo> info = GetParam();
    GTEST_COUT << info->Name << std::endl;

    buildCompositionContext(info);
    Planner->setStrategy(eHardwareFirst);
    Planner->advanceFrame(mPlanConstraint, &mContext, &mOutput);

    for (size_t i = 0; i < info->mInputs.size(); i++) {
        auto& layer  = info->mInputs[i]->InputLayer;
        auto& expect = info->mInputs[i]->AssignedResult;
        verifyAssignedResult(layer, expect);
    }

    // verify client target
    auto& expect = info->mClientTargetAssignedResult;
    verifyAssignedResult(mClientTarget, expect);
}

TEST_P(ParamLayerPlannerTest, ForceGpuComposition)
{
    std::shared_ptr<TestCaseInfo> info = GetParam();
    GTEST_COUT << info->Name << std::endl;

    buildCompositionContext(info);
    Planner->setStrategy(eGPUOnly);
    Planner->advanceFrame(mPlanConstraint, &mContext, &mOutput);

    for (size_t i = 0; i < info->mInputs.size(); i++) {
        auto& layer  = info->mInputs[i]->InputLayer;
        ExpectResult expect;
        expect.Composition = HWC2::Composition::Client;
        expect.Channel = -1;
        expect.Slot    = -1;
        verifyAssignedResult(layer, expect);
    }

    // verify client target
    ExpectResult expect;
    expect.Composition = HWC2::Composition::Device;
    expect.Channel = 0;
    expect.Slot    = 0;
    verifyAssignedResult(mClientTarget, expect);
}

class LayerPlannerWithMemoryLimitTest: public ParamLayerPlannerTest {

};

INSTANTIATE_TEST_CASE_P(
        TestGroup,
        LayerPlannerWithMemoryLimitTest,
        testing::ValuesIn(MemoryLimitCaseLoader->getTestCaseList()),
        generateTestCaseName);

TEST_P(LayerPlannerWithMemoryLimitTest, MemoryLimitTest)
{
    std::shared_ptr<TestCaseInfo> info = GetParam();
    GTEST_COUT << info->Name << std::endl;

    mPlanConstraint.MaxBandwidth = info->BandwidthLimit;

    if (!info->mClientTargetLayer)
        info->mClientTargetLayer = mClientTarget;

    buildCompositionContext(info, info->mClientTargetLayer);
    Planner->setStrategy(eHardwareFirst);
    Planner->advanceFrame(mPlanConstraint, &mContext, &mOutput);

    for (size_t i = 0; i < info->mInputs.size(); i++) {
        auto& layer  = info->mInputs[i]->InputLayer;
        auto& expect = info->mInputs[i]->AssignedResult;
        verifyAssignedResult(layer, expect);
    }

    // verify client target
    auto& expect = info->mClientTargetAssignedResult;
    verifyAssignedResult(info->mClientTargetLayer, expect);
}

TEST_F(ParamLayerPlannerTest, RandomTest)
{
    Planner->setStrategy(eHardwareFirst);

    std::vector<std::shared_ptr<TestCaseInfo>> caseList = CaseLoader->getTestCaseList();
    int caseCount = caseList.size();

    int repeat = 10;
    while (--repeat > 0) {
        int idx = (rand() % caseCount);
        std::shared_ptr<TestCaseInfo> info = caseList[idx];

        mOutput.Configs.clear();
        mOutput.Tracks.clear();

        buildCompositionContext(info);
        Planner->advanceFrame(mPlanConstraint, &mContext, &mOutput);


        for (size_t i = 0; i < info->mInputs.size(); i++) {
            auto& layer  = info->mInputs[i]->InputLayer;
            auto& expect = info->mInputs[i]->AssignedResult;
            verifyAssignedResult(layer, expect);
        }

        // verify client target
        auto& expect = info->mClientTargetAssignedResult;
        verifyAssignedResult(mClientTarget, expect);
    }
}

class HybridChannelWithOutAlphaTest: public ParamLayerPlannerTest {

};

INSTANTIATE_TEST_CASE_P(
        TestGroup,
        HybridChannelWithOutAlphaTest,
        testing::ValuesIn(HybridChannelWithOutAlphaCaseLoader->getTestCaseList()),
        generateTestCaseName);

TEST_P(HybridChannelWithOutAlphaTest, HybridChannelWithOutAlpha)
{
    std::shared_ptr<TestCaseInfo> info = GetParam();
    GTEST_COUT << info->Name << std::endl;

    mPlanConstraint.HybridChannelWithAlpha = false;

    buildCompositionContext(info);
    Planner->setStrategy(eHardwareFirst);
    Planner->advanceFrame(mPlanConstraint, &mContext, &mOutput);

    for (size_t i = 0; i < info->mInputs.size(); i++) {
        auto& layer  = info->mInputs[i]->InputLayer;
        auto& expect = info->mInputs[i]->AssignedResult;
        verifyAssignedResult(layer, expect);
    }

    // verify client target
    auto& expect = info->mClientTargetAssignedResult;
    verifyAssignedResult(mClientTarget, expect);
}

void LayerPlannerTestInit() {
    CaseLoader = std::make_unique<TestCaseLoader>();
    CaseLoader->load("cases/default.json");

    MemoryLimitCaseLoader = std::make_unique<TestCaseLoader>();
    MemoryLimitCaseLoader->load("cases/MemoryLimit.json");

    HybridChannelWithOutAlphaCaseLoader = std::make_unique<TestCaseLoader>();
    HybridChannelWithOutAlphaCaseLoader->load("cases/HybridChannelWithOutAlpha.json");
}
