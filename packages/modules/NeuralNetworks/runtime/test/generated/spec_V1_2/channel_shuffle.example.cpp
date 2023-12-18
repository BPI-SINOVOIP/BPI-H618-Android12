// Generated from channel_shuffle.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0 = TestModelManager::get().add("channel_shuffle_dim4_axis0", get_test_model_dim4_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis0_all_inputs_as_internal", get_test_model_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg = TestModelManager::get().add("channel_shuffle_dim4_axis0_neg", get_test_model_dim4_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis0_neg_all_inputs_as_internal", get_test_model_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1 = TestModelManager::get().add("channel_shuffle_dim4_axis1", get_test_model_dim4_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis1_all_inputs_as_internal", get_test_model_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg = TestModelManager::get().add("channel_shuffle_dim4_axis1_neg", get_test_model_dim4_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis1_neg_all_inputs_as_internal", get_test_model_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2 = TestModelManager::get().add("channel_shuffle_dim4_axis2", get_test_model_dim4_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param5
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis2_all_inputs_as_internal", get_test_model_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg = TestModelManager::get().add("channel_shuffle_dim4_axis2_neg", get_test_model_dim4_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder5
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis2_neg_all_inputs_as_internal", get_test_model_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3 = TestModelManager::get().add("channel_shuffle_dim4_axis3", get_test_model_dim4_axis3());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param7
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis3_all_inputs_as_internal", get_test_model_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg = TestModelManager::get().add("channel_shuffle_dim4_axis3_neg", get_test_model_dim4_axis3_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder7
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param8
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim4_axis3_neg_all_inputs_as_internal", get_test_model_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0 = TestModelManager::get().add("channel_shuffle_dim3_axis0", get_test_model_dim3_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder8
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param9
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis0_all_inputs_as_internal", get_test_model_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg = TestModelManager::get().add("channel_shuffle_dim3_axis0_neg", get_test_model_dim3_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder9
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param10
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis0_neg_all_inputs_as_internal", get_test_model_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1 = TestModelManager::get().add("channel_shuffle_dim3_axis1", get_test_model_dim3_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder10
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis1_all_inputs_as_internal", get_test_model_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg = TestModelManager::get().add("channel_shuffle_dim3_axis1_neg", get_test_model_dim3_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis1_neg_all_inputs_as_internal", get_test_model_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2 = TestModelManager::get().add("channel_shuffle_dim3_axis2", get_test_model_dim3_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis2_all_inputs_as_internal", get_test_model_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg = TestModelManager::get().add("channel_shuffle_dim3_axis2_neg", get_test_model_dim3_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim3_axis2_neg_all_inputs_as_internal", get_test_model_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0 = TestModelManager::get().add("channel_shuffle_dim2_axis0", get_test_model_dim2_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim2_axis0_all_inputs_as_internal", get_test_model_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg = TestModelManager::get().add("channel_shuffle_dim2_axis0_neg", get_test_model_dim2_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param16
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim2_axis0_neg_all_inputs_as_internal", get_test_model_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1 = TestModelManager::get().add("channel_shuffle_dim2_axis1", get_test_model_dim2_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder16
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param17
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim2_axis1_all_inputs_as_internal", get_test_model_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg = TestModelManager::get().add("channel_shuffle_dim2_axis1_neg", get_test_model_dim2_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder17
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param18
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim2_axis1_neg_all_inputs_as_internal", get_test_model_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0 = TestModelManager::get().add("channel_shuffle_dim1_axis0", get_test_model_dim1_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder18
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param19
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim1_axis0_all_inputs_as_internal", get_test_model_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg = TestModelManager::get().add("channel_shuffle_dim1_axis0_neg", get_test_model_dim1_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder19
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param20
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_dim1_axis0_neg_all_inputs_as_internal", get_test_model_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis0 = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis0", get_test_model_relaxed_dim4_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder20
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis0_all_inputs_as_internal", get_test_model_relaxed_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis0_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis0_neg", get_test_model_relaxed_dim4_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param22
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis0_neg_all_inputs_as_internal", get_test_model_relaxed_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis1 = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis1", get_test_model_relaxed_dim4_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder22
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param23
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis1_all_inputs_as_internal", get_test_model_relaxed_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis1_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis1_neg", get_test_model_relaxed_dim4_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder23
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param24
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis1_neg_all_inputs_as_internal", get_test_model_relaxed_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis2 = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis2", get_test_model_relaxed_dim4_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder24
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param25
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis2_all_inputs_as_internal", get_test_model_relaxed_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis2_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis2_neg", get_test_model_relaxed_dim4_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder25
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param26
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis2_neg_all_inputs_as_internal", get_test_model_relaxed_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis3 = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis3", get_test_model_relaxed_dim4_axis3());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder26
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param27
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis3_all_inputs_as_internal", get_test_model_relaxed_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis3_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis3_neg", get_test_model_relaxed_dim4_axis3_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder27
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param28
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim4_axis3_neg_all_inputs_as_internal", get_test_model_relaxed_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis0 = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis0", get_test_model_relaxed_dim3_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder28
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param29
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis0_all_inputs_as_internal", get_test_model_relaxed_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis0_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis0_neg", get_test_model_relaxed_dim3_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder29
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param30
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis0_neg_all_inputs_as_internal", get_test_model_relaxed_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis1 = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis1", get_test_model_relaxed_dim3_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder30
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param31
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis1_all_inputs_as_internal", get_test_model_relaxed_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis1_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis1_neg", get_test_model_relaxed_dim3_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder31
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param32
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis1_neg_all_inputs_as_internal", get_test_model_relaxed_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis2 = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis2", get_test_model_relaxed_dim3_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder32
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param33
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis2_all_inputs_as_internal", get_test_model_relaxed_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis2_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis2_neg", get_test_model_relaxed_dim3_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder33
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param34
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim3_axis2_neg_all_inputs_as_internal", get_test_model_relaxed_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis0 = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis0", get_test_model_relaxed_dim2_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder34
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param35
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis0_all_inputs_as_internal", get_test_model_relaxed_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis0_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis0_neg", get_test_model_relaxed_dim2_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder35
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param36
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis0_neg_all_inputs_as_internal", get_test_model_relaxed_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis1 = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis1", get_test_model_relaxed_dim2_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder36
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param37
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis1_all_inputs_as_internal", get_test_model_relaxed_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis1_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis1_neg", get_test_model_relaxed_dim2_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder37
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param38
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim2_axis1_neg_all_inputs_as_internal", get_test_model_relaxed_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim1_axis0 = TestModelManager::get().add("channel_shuffle_relaxed_dim1_axis0", get_test_model_relaxed_dim1_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder38
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param39
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim1_axis0_all_inputs_as_internal", get_test_model_relaxed_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim1_axis0_neg = TestModelManager::get().add("channel_shuffle_relaxed_dim1_axis0_neg", get_test_model_relaxed_dim1_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_relaxed_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder39
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param40
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_relaxed_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_relaxed_dim1_axis0_neg_all_inputs_as_internal", get_test_model_relaxed_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis0 = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis0", get_test_model_quant8_dim4_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder40
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis0_all_inputs_as_internal", get_test_model_quant8_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis0_neg = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis0_neg", get_test_model_quant8_dim4_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 192, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148, 196, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 152, 200, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 156, 204, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis0_neg_all_inputs_as_internal", get_test_model_quant8_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis1 = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis1", get_test_model_quant8_dim4_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis1_all_inputs_as_internal", get_test_model_quant8_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis1_neg = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis1_neg", get_test_model_quant8_dim4_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param44
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis1_neg_all_inputs_as_internal", get_test_model_quant8_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis2 = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis2", get_test_model_quant8_dim4_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder44
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param45
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis2_all_inputs_as_internal", get_test_model_quant8_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis2_neg = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis2_neg", get_test_model_quant8_dim4_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder45
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param46
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis2_neg_all_inputs_as_internal", get_test_model_quant8_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis3 = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis3", get_test_model_quant8_dim4_axis3());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder46
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param47
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis3_all_inputs_as_internal", get_test_model_quant8_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis3_neg = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis3_neg", get_test_model_quant8_dim4_axis3_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder47
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param48
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim4_axis3_neg_all_inputs_as_internal", get_test_model_quant8_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis0 = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis0", get_test_model_quant8_dim3_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder48
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param49
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis0_all_inputs_as_internal", get_test_model_quant8_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis0_neg = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis0_neg", get_test_model_quant8_dim3_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 144, 192, 240, 255, 255, 255, 160, 208, 255, 255, 255, 255, 132, 180, 228, 255, 255, 255, 148, 196, 244, 255, 255, 255, 164, 212, 255, 255, 255, 255, 136, 184, 232, 255, 255, 255, 152, 200, 248, 255, 255, 255, 168, 216, 255, 255, 255, 255, 140, 188, 236, 255, 255, 255, 156, 204, 252, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 255, 255, 255, 132, 180, 228, 255, 255, 255, 136, 184, 232, 255, 255, 255, 140, 188, 236, 255, 255, 255, 144, 192, 240, 255, 255, 255, 148, 196, 244, 255, 255, 255, 152, 200, 248, 255, 255, 255, 156, 204, 252, 255, 255, 255, 160, 208, 255, 255, 255, 255, 164, 212, 255, 255, 255, 255, 168, 216, 255, 255, 255, 255, 172, 220, 255, 255, 255, 255}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder49
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param50
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis0_neg_all_inputs_as_internal", get_test_model_quant8_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis1 = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis1", get_test_model_quant8_dim3_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder50
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param51
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis1_all_inputs_as_internal", get_test_model_quant8_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis1_neg = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis1_neg", get_test_model_quant8_dim3_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder51
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param52
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis1_neg_all_inputs_as_internal", get_test_model_quant8_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis2 = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis2", get_test_model_quant8_dim3_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder52
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param53
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis2_all_inputs_as_internal", get_test_model_quant8_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis2_neg = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis2_neg", get_test_model_quant8_dim3_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder53
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param54
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim3_axis2_neg_all_inputs_as_internal", get_test_model_quant8_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis0 = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis0", get_test_model_quant8_dim2_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder54
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param55
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis0_all_inputs_as_internal", get_test_model_quant8_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis0_neg = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis0_neg", get_test_model_quant8_dim2_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 144, 192, 240, 160, 208, 255, 132, 180, 228, 148, 196, 244, 164, 212, 255, 136, 184, 232, 152, 200, 248, 168, 216, 255, 140, 188, 236, 156, 204, 252, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 176, 224, 132, 180, 228, 136, 184, 232, 140, 188, 236, 144, 192, 240, 148, 196, 244, 152, 200, 248, 156, 204, 252, 160, 208, 255, 164, 212, 255, 168, 216, 255, 172, 220, 255}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder55
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param56
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis0_neg_all_inputs_as_internal", get_test_model_quant8_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis1 = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis1", get_test_model_quant8_dim2_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder56
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param57
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis1_all_inputs_as_internal", get_test_model_quant8_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis1_neg = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis1_neg", get_test_model_quant8_dim2_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172, 176, 192, 208, 180, 196, 212, 184, 200, 216, 188, 204, 220, 224, 240, 255, 228, 244, 255, 232, 248, 255, 236, 252, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255, 255, 255, 255}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder57
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param58
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim2_axis1_neg_all_inputs_as_internal", get_test_model_quant8_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim1_axis0 = TestModelManager::get().add("channel_shuffle_quant8_dim1_axis0", get_test_model_quant8_dim1_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder58
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param59
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim1_axis0_all_inputs_as_internal", get_test_model_quant8_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim1_axis0_neg = TestModelManager::get().add("channel_shuffle_quant8_dim1_axis0_neg", get_test_model_quant8_dim1_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_quant8_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 144, 160, 132, 148, 164, 136, 152, 168, 140, 156, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder59
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param60
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_quant8_dim1_axis0_neg_all_inputs_as_internal", get_test_model_quant8_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis0 = TestModelManager::get().add("channel_shuffle_float16_dim4_axis0", get_test_model_float16_dim4_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder60
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param61
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis0_all_inputs_as_internal", get_test_model_float16_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis0_neg = TestModelManager::get().add("channel_shuffle_float16_dim4_axis0_neg", get_test_model_float16_dim4_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {12, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder61
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param62
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis0_neg_all_inputs_as_internal", get_test_model_float16_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis1 = TestModelManager::get().add("channel_shuffle_float16_dim4_axis1", get_test_model_float16_dim4_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder62
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param63
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis1_all_inputs_as_internal", get_test_model_float16_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis1_neg = TestModelManager::get().add("channel_shuffle_float16_dim4_axis1_neg", get_test_model_float16_dim4_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 108.0f, 120.0f, 132.0f, 73.0f, 85.0f, 97.0f, 109.0f, 121.0f, 133.0f, 74.0f, 86.0f, 98.0f, 110.0f, 122.0f, 134.0f, 75.0f, 87.0f, 99.0f, 111.0f, 123.0f, 135.0f, 76.0f, 88.0f, 100.0f, 112.0f, 124.0f, 136.0f, 77.0f, 89.0f, 101.0f, 113.0f, 125.0f, 137.0f, 78.0f, 90.0f, 102.0f, 114.0f, 126.0f, 138.0f, 79.0f, 91.0f, 103.0f, 115.0f, 127.0f, 139.0f, 80.0f, 92.0f, 104.0f, 116.0f, 128.0f, 140.0f, 81.0f, 93.0f, 105.0f, 117.0f, 129.0f, 141.0f, 82.0f, 94.0f, 106.0f, 118.0f, 130.0f, 142.0f, 83.0f, 95.0f, 107.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder63
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param64
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis1_neg_all_inputs_as_internal", get_test_model_float16_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis2 = TestModelManager::get().add("channel_shuffle_float16_dim4_axis2", get_test_model_float16_dim4_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder64
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param65
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis2_all_inputs_as_internal", get_test_model_float16_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis2_neg = TestModelManager::get().add("channel_shuffle_float16_dim4_axis2_neg", get_test_model_float16_dim4_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 76.0f, 88.0f, 100.0f, 80.0f, 92.0f, 104.0f, 73.0f, 85.0f, 97.0f, 77.0f, 89.0f, 101.0f, 81.0f, 93.0f, 105.0f, 74.0f, 86.0f, 98.0f, 78.0f, 90.0f, 102.0f, 82.0f, 94.0f, 106.0f, 75.0f, 87.0f, 99.0f, 79.0f, 91.0f, 103.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 112.0f, 124.0f, 136.0f, 116.0f, 128.0f, 140.0f, 109.0f, 121.0f, 133.0f, 113.0f, 125.0f, 137.0f, 117.0f, 129.0f, 141.0f, 110.0f, 122.0f, 134.0f, 114.0f, 126.0f, 138.0f, 118.0f, 130.0f, 142.0f, 111.0f, 123.0f, 135.0f, 115.0f, 127.0f, 139.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f, 72.0f, 84.0f, 96.0f, 73.0f, 85.0f, 97.0f, 74.0f, 86.0f, 98.0f, 75.0f, 87.0f, 99.0f, 76.0f, 88.0f, 100.0f, 77.0f, 89.0f, 101.0f, 78.0f, 90.0f, 102.0f, 79.0f, 91.0f, 103.0f, 80.0f, 92.0f, 104.0f, 81.0f, 93.0f, 105.0f, 82.0f, 94.0f, 106.0f, 83.0f, 95.0f, 107.0f, 108.0f, 120.0f, 132.0f, 109.0f, 121.0f, 133.0f, 110.0f, 122.0f, 134.0f, 111.0f, 123.0f, 135.0f, 112.0f, 124.0f, 136.0f, 113.0f, 125.0f, 137.0f, 114.0f, 126.0f, 138.0f, 115.0f, 127.0f, 139.0f, 116.0f, 128.0f, 140.0f, 117.0f, 129.0f, 141.0f, 118.0f, 130.0f, 142.0f, 119.0f, 131.0f, 143.0f}),
                            .dimensions = {2, 2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder65
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param66
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis2_neg_all_inputs_as_internal", get_test_model_float16_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis3 = TestModelManager::get().add("channel_shuffle_float16_dim4_axis3", get_test_model_float16_dim4_axis3());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder66
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param67
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis3_all_inputs_as_internal", get_test_model_float16_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis3_neg = TestModelManager::get().add("channel_shuffle_float16_dim4_axis3_neg", get_test_model_float16_dim4_axis3_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f, 72.0f, 76.0f, 80.0f, 73.0f, 77.0f, 81.0f, 74.0f, 78.0f, 82.0f, 75.0f, 79.0f, 83.0f, 84.0f, 88.0f, 92.0f, 85.0f, 89.0f, 93.0f, 86.0f, 90.0f, 94.0f, 87.0f, 91.0f, 95.0f, 96.0f, 100.0f, 104.0f, 97.0f, 101.0f, 105.0f, 98.0f, 102.0f, 106.0f, 99.0f, 103.0f, 107.0f, 108.0f, 112.0f, 116.0f, 109.0f, 113.0f, 117.0f, 110.0f, 114.0f, 118.0f, 111.0f, 115.0f, 119.0f, 120.0f, 124.0f, 128.0f, 121.0f, 125.0f, 129.0f, 122.0f, 126.0f, 130.0f, 123.0f, 127.0f, 131.0f, 132.0f, 136.0f, 140.0f, 133.0f, 137.0f, 141.0f, 134.0f, 138.0f, 142.0f, 135.0f, 139.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f}),
                            .dimensions = {2, 2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder67
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param68
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim4_axis3_neg_all_inputs_as_internal", get_test_model_float16_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis0 = TestModelManager::get().add("channel_shuffle_float16_dim3_axis0", get_test_model_float16_dim3_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder68
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param69
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis0_all_inputs_as_internal", get_test_model_float16_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis0_neg = TestModelManager::get().add("channel_shuffle_float16_dim3_axis0_neg", get_test_model_float16_dim3_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 36.0f, 48.0f, 60.0f, 1.0f, 13.0f, 25.0f, 37.0f, 49.0f, 61.0f, 2.0f, 14.0f, 26.0f, 38.0f, 50.0f, 62.0f, 3.0f, 15.0f, 27.0f, 39.0f, 51.0f, 63.0f, 4.0f, 16.0f, 28.0f, 40.0f, 52.0f, 64.0f, 5.0f, 17.0f, 29.0f, 41.0f, 53.0f, 65.0f, 6.0f, 18.0f, 30.0f, 42.0f, 54.0f, 66.0f, 7.0f, 19.0f, 31.0f, 43.0f, 55.0f, 67.0f, 8.0f, 20.0f, 32.0f, 44.0f, 56.0f, 68.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f, 69.0f, 10.0f, 22.0f, 34.0f, 46.0f, 58.0f, 70.0f, 11.0f, 23.0f, 35.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {12, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder69
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param70
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis0_neg_all_inputs_as_internal", get_test_model_float16_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis1 = TestModelManager::get().add("channel_shuffle_float16_dim3_axis1", get_test_model_float16_dim3_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder70
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param71
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis1_all_inputs_as_internal", get_test_model_float16_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis1_neg = TestModelManager::get().add("channel_shuffle_float16_dim3_axis1_neg", get_test_model_float16_dim3_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 40.0f, 52.0f, 64.0f, 44.0f, 56.0f, 68.0f, 37.0f, 49.0f, 61.0f, 41.0f, 53.0f, 65.0f, 45.0f, 57.0f, 69.0f, 38.0f, 50.0f, 62.0f, 42.0f, 54.0f, 66.0f, 46.0f, 58.0f, 70.0f, 39.0f, 51.0f, 63.0f, 43.0f, 55.0f, 67.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f, 36.0f, 48.0f, 60.0f, 37.0f, 49.0f, 61.0f, 38.0f, 50.0f, 62.0f, 39.0f, 51.0f, 63.0f, 40.0f, 52.0f, 64.0f, 41.0f, 53.0f, 65.0f, 42.0f, 54.0f, 66.0f, 43.0f, 55.0f, 67.0f, 44.0f, 56.0f, 68.0f, 45.0f, 57.0f, 69.0f, 46.0f, 58.0f, 70.0f, 47.0f, 59.0f, 71.0f}),
                            .dimensions = {2, 12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder71
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param72
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis1_neg_all_inputs_as_internal", get_test_model_float16_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis2 = TestModelManager::get().add("channel_shuffle_float16_dim3_axis2", get_test_model_float16_dim3_axis2());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder72
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param73
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis2_all_inputs_as_internal", get_test_model_float16_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis2_neg = TestModelManager::get().add("channel_shuffle_float16_dim3_axis2_neg", get_test_model_float16_dim3_axis2_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f, 36.0f, 40.0f, 44.0f, 37.0f, 41.0f, 45.0f, 38.0f, 42.0f, 46.0f, 39.0f, 43.0f, 47.0f, 48.0f, 52.0f, 56.0f, 49.0f, 53.0f, 57.0f, 50.0f, 54.0f, 58.0f, 51.0f, 55.0f, 59.0f, 60.0f, 64.0f, 68.0f, 61.0f, 65.0f, 69.0f, 62.0f, 66.0f, 70.0f, 63.0f, 67.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f}),
                            .dimensions = {2, 3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder73
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param74
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim3_axis2_neg_all_inputs_as_internal", get_test_model_float16_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis0 = TestModelManager::get().add("channel_shuffle_float16_dim2_axis0", get_test_model_float16_dim2_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder74
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param75
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim2_axis0_all_inputs_as_internal", get_test_model_float16_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis0_neg = TestModelManager::get().add("channel_shuffle_float16_dim2_axis0_neg", get_test_model_float16_dim2_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 4.0f, 16.0f, 28.0f, 8.0f, 20.0f, 32.0f, 1.0f, 13.0f, 25.0f, 5.0f, 17.0f, 29.0f, 9.0f, 21.0f, 33.0f, 2.0f, 14.0f, 26.0f, 6.0f, 18.0f, 30.0f, 10.0f, 22.0f, 34.0f, 3.0f, 15.0f, 27.0f, 7.0f, 19.0f, 31.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 12.0f, 24.0f, 1.0f, 13.0f, 25.0f, 2.0f, 14.0f, 26.0f, 3.0f, 15.0f, 27.0f, 4.0f, 16.0f, 28.0f, 5.0f, 17.0f, 29.0f, 6.0f, 18.0f, 30.0f, 7.0f, 19.0f, 31.0f, 8.0f, 20.0f, 32.0f, 9.0f, 21.0f, 33.0f, 10.0f, 22.0f, 34.0f, 11.0f, 23.0f, 35.0f}),
                            .dimensions = {12, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder75
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param76
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim2_axis0_neg_all_inputs_as_internal", get_test_model_float16_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis1 = TestModelManager::get().add("channel_shuffle_float16_dim2_axis1", get_test_model_float16_dim2_axis1());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder76
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param77
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim2_axis1_all_inputs_as_internal", get_test_model_float16_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis1_neg = TestModelManager::get().add("channel_shuffle_float16_dim2_axis1_neg", get_test_model_float16_dim2_axis1_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f, 12.0f, 16.0f, 20.0f, 13.0f, 17.0f, 21.0f, 14.0f, 18.0f, 22.0f, 15.0f, 19.0f, 23.0f, 24.0f, 28.0f, 32.0f, 25.0f, 29.0f, 33.0f, 26.0f, 30.0f, 34.0f, 27.0f, 31.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f}),
                            .dimensions = {3, 12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder77
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param78
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim2_axis1_neg_all_inputs_as_internal", get_test_model_float16_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim1_axis0 = TestModelManager::get().add("channel_shuffle_float16_dim1_axis0", get_test_model_float16_dim1_axis0());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder78
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param79
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim1_axis0_all_inputs_as_internal", get_test_model_float16_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim1_axis0_neg = TestModelManager::get().add("channel_shuffle_float16_dim1_axis0_neg", get_test_model_float16_dim1_axis0_neg());

}  // namespace generated_tests::channel_shuffle

namespace generated_tests::channel_shuffle {

const TestModel& get_test_model_float16_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // axis
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 4.0f, 8.0f, 1.0f, 5.0f, 9.0f, 2.0f, 6.0f, 10.0f, 3.0f, 7.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f}),
                            .dimensions = {12},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder79
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param80
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::CHANNEL_SHUFFLE
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_float16_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("channel_shuffle_float16_dim1_axis0_neg_all_inputs_as_internal", get_test_model_float16_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::channel_shuffle

