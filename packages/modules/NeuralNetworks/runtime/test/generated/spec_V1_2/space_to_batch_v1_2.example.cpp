// Generated from space_to_batch_v1_2.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc = TestModelManager::get().add("space_to_batch_v1_2_nhwc", get_test_model_nhwc());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_inputs_as_internal", get_test_model_nhwc_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs", get_test_model_nhwc_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed", get_test_model_nhwc_relaxed());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_inputs_as_internal", get_test_model_nhwc_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs", get_test_model_nhwc_relaxed_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16", get_test_model_nhwc_float16());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_inputs_as_internal", get_test_model_nhwc_float16_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs", get_test_model_nhwc_float16_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder5
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8", get_test_model_nhwc_quant8());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_inputs_as_internal", get_test_model_nhwc_quant8_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs", get_test_model_nhwc_quant8_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 1, 1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder7
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw = TestModelManager::get().add("space_to_batch_v1_2_nchw", get_test_model_nchw());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_inputs_as_internal", get_test_model_nchw_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs", get_test_model_nchw_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed", get_test_model_nchw_relaxed());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_inputs_as_internal", get_test_model_nchw_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs", get_test_model_nchw_relaxed_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 2.3f, 3.2f, 4.1f, 5.4f, 6.3f, 7.2f, 8.1f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.4f, 3.2f, 5.4f, 7.2f, 2.3f, 4.1f, 6.3f, 8.1f}),
                            .dimensions = {1, 2, 2, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 3.200000047683716f, 5.400000095367432f, 7.199999809265137f, 2.299999952316284f, 4.099999904632568f, 6.300000190734863f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16", get_test_model_nchw_float16());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 3.200000047683716f, 5.400000095367432f, 7.199999809265137f, 2.299999952316284f, 4.099999904632568f, 6.300000190734863f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_inputs_as_internal", get_test_model_nchw_float16_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 3.200000047683716f, 5.400000095367432f, 7.199999809265137f, 2.299999952316284f, 4.099999904632568f, 6.300000190734863f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs", get_test_model_nchw_float16_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 2.299999952316284f, 3.200000047683716f, 4.099999904632568f, 5.400000095367432f, 6.300000190734863f, 7.199999809265137f, 8.100000381469727f}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.399999976158142f, 3.200000047683716f, 5.400000095367432f, 7.199999809265137f, 2.299999952316284f, 4.099999904632568f, 6.300000190734863f, 8.100000381469727f}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 32, 54, 72, 23, 41, 63, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8", get_test_model_nchw_quant8());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 32, 54, 72, 23, 41, 63, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_inputs_as_internal", get_test_model_nchw_quant8_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 32, 54, 72, 23, 41, 63, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs", get_test_model_nchw_quant8_all_tensors_as_inputs());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 23, 32, 41, 54, 63, 72, 81}),
                            .dimensions = {4, 2, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({14, 32, 54, 72, 23, 41, 63, 81}),
                            .dimensions = {1, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_2", get_test_model_nhwc_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_inputs_as_internal_2", get_test_model_nhwc_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_2", get_test_model_nhwc_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_2", get_test_model_nhwc_relaxed_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_inputs_as_internal_2", get_test_model_nhwc_relaxed_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_2", get_test_model_nhwc_relaxed_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_2", get_test_model_nhwc_float16_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder20
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_inputs_as_internal_2", get_test_model_nhwc_float16_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_2", get_test_model_nhwc_float16_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_2", get_test_model_nhwc_quant8_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder22
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_inputs_as_internal_2", get_test_model_nhwc_quant8_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_2", get_test_model_nhwc_quant8_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 4, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder23
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_2", get_test_model_nchw_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_inputs_as_internal_2", get_test_model_nchw_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_2", get_test_model_nchw_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_2", get_test_model_nchw_relaxed_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_inputs_as_internal_2", get_test_model_nchw_relaxed_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_2", get_test_model_nchw_relaxed_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_2", get_test_model_nchw_float16_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder28
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_inputs_as_internal_2", get_test_model_nchw_float16_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_2", get_test_model_nchw_float16_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 3.0f, 9.0f, 11.0f, 2.0f, 4.0f, 10.0f, 12.0f, 5.0f, 7.0f, 13.0f, 15.0f, 6.0f, 8.0f, 14.0f, 16.0f}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder29
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_2", get_test_model_nchw_quant8_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder30
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_inputs_as_internal_2", get_test_model_nchw_quant8_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_2", get_test_model_nchw_quant8_all_tensors_as_inputs_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0, 0, 0, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 6, 18, 22, 4, 8, 20, 24, 10, 14, 26, 30, 12, 16, 28, 32}),
                            .dimensions = {4, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32}),
                            .dimensions = {1, 1, 4, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 0
                        }, { // placeholder31
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_2 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_2", get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_2());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_3", get_test_model_nhwc_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_inputs_as_internal_3", get_test_model_nhwc_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_3", get_test_model_nhwc_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_3", get_test_model_nhwc_relaxed_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_inputs_as_internal_3", get_test_model_nhwc_relaxed_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_3", get_test_model_nhwc_relaxed_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_3", get_test_model_nhwc_float16_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder36
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_inputs_as_internal_3", get_test_model_nhwc_float16_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_3", get_test_model_nhwc_float16_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder37
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_3", get_test_model_nhwc_quant8_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder38
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_inputs_as_internal_3", get_test_model_nhwc_quant8_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_3", get_test_model_nhwc_quant8_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 5, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder39
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_3", get_test_model_nchw_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder40
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_inputs_as_internal_3", get_test_model_nchw_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_3", get_test_model_nchw_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_3", get_test_model_nchw_relaxed_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_inputs_as_internal_3", get_test_model_nchw_relaxed_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_3", get_test_model_nchw_relaxed_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_3", get_test_model_nchw_float16_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder44
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_inputs_as_internal_3", get_test_model_nchw_float16_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_3", get_test_model_nchw_float16_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 7.0f, 0.0f, 2.0f, 0.0f, 8.0f, 0.0f, 3.0f, 0.0f, 9.0f, 0.0f, 4.0f, 0.0f, 10.0f}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder45
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_3", get_test_model_nchw_quant8_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder46
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_inputs_as_internal_3", get_test_model_nchw_quant8_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_3", get_test_model_nchw_quant8_all_tensors_as_inputs_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 0, 2, 0}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 138, 128, 128, 128, 140, 128, 130, 128, 142, 128, 132, 128, 144, 128, 134, 128, 146, 128, 136, 128, 148}),
                            .dimensions = {6, 1, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op12_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({130, 132, 134, 136, 138, 140, 142, 144, 146, 148}),
                            .dimensions = {1, 1, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder47
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_3 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_3", get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_3());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_4", get_test_model_nhwc_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder48
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_inputs_as_internal_4", get_test_model_nhwc_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_4", get_test_model_nhwc_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder49
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_1,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nhwc_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_4", get_test_model_nhwc_relaxed_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder50
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_inputs_as_internal_4", get_test_model_nhwc_relaxed_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_4", get_test_model_nhwc_relaxed_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder51
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nhwc_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_4", get_test_model_nhwc_float16_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder52
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_inputs_as_internal_4", get_test_model_nhwc_float16_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_4", get_test_model_nhwc_float16_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder53
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nhwc_float16_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_4", get_test_model_nhwc_quant8_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 4, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_inputs_as_internal_4", get_test_model_nhwc_quant8_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_4", get_test_model_nhwc_quant8_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 4, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({false}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 2, 4, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 4, 2, 1},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nhwc_quant8_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_4", get_test_model_nchw_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder56
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_inputs_as_internal_4", get_test_model_nchw_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_4", get_test_model_nchw_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder57
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nchw_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_4", get_test_model_nchw_relaxed_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder58
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_inputs_as_internal_4", get_test_model_nchw_relaxed_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_4", get_test_model_nchw_relaxed_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder59
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nchw_relaxed_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_4", get_test_model_nchw_float16_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_inputs_as_internal_4", get_test_model_nchw_float16_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_4", get_test_model_nchw_float16_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}),
                            .dimensions = {1, 1, 4, 2},
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nchw_float16_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_4", get_test_model_nchw_quant8_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder62
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_inputs_as_internal_4", get_test_model_nchw_quant8_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0, 2},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_4", get_test_model_nchw_quant8_all_tensors_as_inputs_4());

}  // namespace generated_tests::space_to_batch_v1_2

namespace generated_tests::space_to_batch_v1_2 {

const TestModel& get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_4() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {2, 5},
                .operands = {{ // op13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({3, 2}),
                            .dimensions = {2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // paddings2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1, 1, 2, 4}),
                            .dimensions = {2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // layout
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<bool8>({true}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::BOOL,
                            .zeroPoint = 0
                        }, { // op43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128, 128, 128, 128, 128, 148, 128, 128, 128, 128, 128, 128, 128, 152, 128, 128, 128, 132, 128, 128, 128, 156, 128, 128, 128, 136, 128, 128, 128, 160, 128, 128, 128, 140, 128, 128, 128, 128, 128, 128, 128, 144, 128, 128, 128, 128, 128, 128}),
                            .dimensions = {6, 1, 2, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op13_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({132, 136, 140, 144, 148, 152, 156, 160}),
                            .dimensions = {1, 1, 4, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // placeholder63
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
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
                            .inputs = {5, 6, 7},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2, 3},
                            .outputs = {4},
                            .type = TestOperationType::SPACE_TO_BATCH_ND
                        }},
                .outputIndexes = {4}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_4 = TestModelManager::get().add("space_to_batch_v1_2_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_4", get_test_model_nchw_quant8_all_tensors_as_inputs_all_inputs_as_internal_4());

}  // namespace generated_tests::space_to_batch_v1_2

