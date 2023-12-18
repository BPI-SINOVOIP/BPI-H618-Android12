// Generated from l2_normalization_axis.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0 = TestModelManager::get().add("l2_normalization_axis_dim4_axis0", get_test_model_dim4_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
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
                        }, { // param
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_all_inputs_as_internal", get_test_model_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_relaxed", get_test_model_dim4_axis0_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_relaxed_all_inputs_as_internal", get_test_model_dim4_axis0_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_float16", get_test_model_dim4_axis0_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_float16_all_inputs_as_internal", get_test_model_dim4_axis0_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 122, 152, 232, 192, 62, 32, 92, 32, 152, 182, 122, 182, 72, 72, 32, 122, 232, 192, 152, 152}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 174, 189, 230, 210, 205, 128, 205, 128, 189, 205, 174, 205, 230, 230, 128, 205, 230, 210, 189, 189}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_quant8", get_test_model_dim4_axis0_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 174, 189, 230, 210, 205, 128, 205, 128, 189, 205, 174, 205, 230, 230, 128, 205, 230, 210, 189, 189}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 122, 152, 232, 192, 62, 32, 92, 32, 152, 182, 122, 182, 72, 72, 32, 122, 232, 192, 152, 152}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_quant8_all_inputs_as_internal", get_test_model_dim4_axis0_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg", get_test_model_dim4_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_all_inputs_as_internal", get_test_model_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_relaxed", get_test_model_dim4_axis0_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.36f, 0.48f, 0.8f, 0.64f, 0.6f, 0.0f, 0.6f, 0.0f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.8f, 0.0f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_relaxed_all_inputs_as_internal", get_test_model_dim4_axis0_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_float16", get_test_model_dim4_axis0_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 9.0f, 12.0f, 20.0f, 16.0f, 3.0f, 0.0f, 6.0f, 0.0f, 12.0f, 15.0f, 9.0f, 15.0f, 4.0f, 4.0f, 0.0f, 9.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_float16_all_inputs_as_internal", get_test_model_dim4_axis0_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 122, 152, 232, 192, 62, 32, 92, 32, 152, 182, 122, 182, 72, 72, 32, 122, 232, 192, 152, 152}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 174, 189, 230, 210, 205, 128, 205, 128, 189, 205, 174, 205, 230, 230, 128, 205, 230, 210, 189, 189}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_quant8", get_test_model_dim4_axis0_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis0_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 174, 189, 230, 210, 205, 128, 205, 128, 189, 205, 174, 205, 230, 230, 128, 205, 230, 210, 189, 189}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 122, 152, 232, 192, 62, 32, 92, 32, 152, 182, 122, 182, 72, 72, 32, 122, 232, 192, 152, 152}),
                            .dimensions = {3, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder7
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis0_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis0_neg_quant8_all_inputs_as_internal", get_test_model_dim4_axis0_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1 = TestModelManager::get().add("l2_normalization_axis_dim4_axis1", get_test_model_dim4_axis1());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_all_inputs_as_internal", get_test_model_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_relaxed", get_test_model_dim4_axis1_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_relaxed_all_inputs_as_internal", get_test_model_dim4_axis1_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_float16", get_test_model_dim4_axis1_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder10
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_float16_all_inputs_as_internal", get_test_model_dim4_axis1_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122, 122, 152, 232, 192, 152, 182, 122, 182, 232, 192, 152, 152}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205, 174, 189, 230, 210, 189, 205, 174, 205, 230, 210, 189, 189}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_quant8", get_test_model_dim4_axis1_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205, 174, 189, 230, 210, 189, 205, 174, 205, 230, 210, 189, 189}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122, 122, 152, 232, 192, 152, 182, 122, 182, 232, 192, 152, 152}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_quant8_all_inputs_as_internal", get_test_model_dim4_axis1_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg", get_test_model_dim4_axis1_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_all_inputs_as_internal", get_test_model_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_relaxed", get_test_model_dim4_axis1_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.64f, 0.48f, 0.6f, 0.36f, 0.6f, 0.8f, 0.64f, 0.48f, 0.48f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_relaxed_all_inputs_as_internal", get_test_model_dim4_axis1_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_float16", get_test_model_dim4_axis1_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.6000000238418579f, 0.36000001430511475f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 16.0f, 12.0f, 15.0f, 9.0f, 15.0f, 20.0f, 16.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_float16_all_inputs_as_internal", get_test_model_dim4_axis1_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122, 122, 152, 232, 192, 152, 182, 122, 182, 232, 192, 152, 152}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205, 174, 189, 230, 210, 189, 205, 174, 205, 230, 210, 189, 189}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_quant8", get_test_model_dim4_axis1_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis1_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205, 174, 189, 230, 210, 189, 205, 174, 205, 230, 210, 189, 189}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122, 122, 152, 232, 192, 152, 182, 122, 182, 232, 192, 152, 152}),
                            .dimensions = {2, 3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis1_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis1_neg_quant8_all_inputs_as_internal", get_test_model_dim4_axis1_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2 = TestModelManager::get().add("l2_normalization_axis_dim4_axis2", get_test_model_dim4_axis2());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_all_inputs_as_internal", get_test_model_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_relaxed", get_test_model_dim4_axis2_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_relaxed_all_inputs_as_internal", get_test_model_dim4_axis2_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.47999998927116394f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.800000011920929f, 0.6399999856948853f, 0.36000001430511475f, 0.6000000238418579f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_float16", get_test_model_dim4_axis2_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.47999998927116394f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.800000011920929f, 0.6399999856948853f, 0.36000001430511475f, 0.6000000238418579f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder18
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_float16_all_inputs_as_internal", get_test_model_dim4_axis2_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122, 122, 152, 152, 182, 232, 192, 232, 192, 122, 182, 152, 152}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205, 174, 189, 189, 205, 230, 210, 230, 210, 174, 205, 189, 189}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_quant8", get_test_model_dim4_axis2_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205, 174, 189, 189, 205, 230, 210, 230, 210, 174, 205, 189, 189}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122, 122, 152, 152, 182, 232, 192, 232, 192, 122, 182, 152, 152}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder19
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_quant8_all_inputs_as_internal", get_test_model_dim4_axis2_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg", get_test_model_dim4_axis2_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_all_inputs_as_internal", get_test_model_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_relaxed", get_test_model_dim4_axis2_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f, 0.36f, 0.48f, 0.48f, 0.6f, 0.8f, 0.64f, 0.8f, 0.64f, 0.36f, 0.6f, 0.48f, 0.48f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_relaxed_all_inputs_as_internal", get_test_model_dim4_axis2_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.47999998927116394f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.800000011920929f, 0.6399999856948853f, 0.36000001430511475f, 0.6000000238418579f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_float16", get_test_model_dim4_axis2_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.47999998927116394f, 0.6000000238418579f, 0.800000011920929f, 0.6399999856948853f, 0.800000011920929f, 0.6399999856948853f, 0.36000001430511475f, 0.6000000238418579f, 0.47999998927116394f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f, 9.0f, 12.0f, 12.0f, 15.0f, 20.0f, 16.0f, 20.0f, 16.0f, 9.0f, 15.0f, 12.0f, 12.0f}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder22
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_float16_all_inputs_as_internal", get_test_model_dim4_axis2_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122, 122, 152, 152, 182, 232, 192, 232, 192, 122, 182, 152, 152}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205, 174, 189, 189, 205, 230, 210, 230, 210, 174, 205, 189, 189}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_quant8", get_test_model_dim4_axis2_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis2_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205, 174, 189, 189, 205, 230, 210, 230, 210, 174, 205, 189, 189}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122, 122, 152, 152, 182, 232, 192, 232, 192, 122, 182, 152, 152}),
                            .dimensions = {2, 2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder23
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis2_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis2_neg_quant8_all_inputs_as_internal", get_test_model_dim4_axis2_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3 = TestModelManager::get().add("l2_normalization_axis_dim4_axis3", get_test_model_dim4_axis3());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_all_inputs_as_internal", get_test_model_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_relaxed", get_test_model_dim4_axis3_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_relaxed_all_inputs_as_internal", get_test_model_dim4_axis3_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.47999998927116394f, 0.6000000238418579f, 0.6399999856948853f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.6399999856948853f, 0.6000000238418579f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_float16", get_test_model_dim4_axis3_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.47999998927116394f, 0.6000000238418579f, 0.6399999856948853f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.6399999856948853f, 0.6000000238418579f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder26
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_float16_all_inputs_as_internal", get_test_model_dim4_axis3_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122, 122, 152, 232, 152, 182, 192, 232, 122, 152, 192, 182, 152}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205, 174, 189, 230, 189, 205, 210, 230, 174, 189, 210, 205, 189}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_quant8", get_test_model_dim4_axis3_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205, 174, 189, 230, 189, 205, 210, 230, 174, 189, 210, 205, 189}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122, 122, 152, 232, 152, 182, 192, 232, 122, 152, 192, 182, 152}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder27
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_quant8_all_inputs_as_internal", get_test_model_dim4_axis3_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_0,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg", get_test_model_dim4_axis3_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_0,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_all_inputs_as_internal", get_test_model_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_relaxed", get_test_model_dim4_axis3_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f, 0.36f, 0.48f, 0.8f, 0.48f, 0.6f, 0.64f, 0.8f, 0.36f, 0.48f, 0.64f, 0.6f, 0.48f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_relaxed_all_inputs_as_internal", get_test_model_dim4_axis3_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.47999998927116394f, 0.6000000238418579f, 0.6399999856948853f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.6399999856948853f, 0.6000000238418579f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_float16", get_test_model_dim4_axis3_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f, 0.36000001430511475f, 0.47999998927116394f, 0.800000011920929f, 0.47999998927116394f, 0.6000000238418579f, 0.6399999856948853f, 0.800000011920929f, 0.36000001430511475f, 0.47999998927116394f, 0.6399999856948853f, 0.6000000238418579f, 0.47999998927116394f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f, 9.0f, 12.0f, 20.0f, 12.0f, 15.0f, 16.0f, 20.0f, 9.0f, 12.0f, 16.0f, 15.0f, 12.0f}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder30
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_float16_all_inputs_as_internal", get_test_model_dim4_axis3_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122, 122, 152, 232, 152, 182, 192, 232, 122, 152, 192, 182, 152}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205, 174, 189, 230, 189, 205, 210, 230, 174, 189, 210, 205, 189}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_quant8", get_test_model_dim4_axis3_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim4_axis3_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205, 174, 189, 230, 189, 205, 210, 230, 174, 189, 210, 205, 189}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122, 122, 152, 232, 152, 182, 192, 232, 122, 152, 192, 182, 152}),
                            .dimensions = {2, 2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder31
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim4_axis3_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim4_axis3_neg_quant8_all_inputs_as_internal", get_test_model_dim4_axis3_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0 = TestModelManager::get().add("l2_normalization_axis_dim3_axis0", get_test_model_dim3_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_all_inputs_as_internal", get_test_model_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_relaxed", get_test_model_dim3_axis0_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_relaxed_all_inputs_as_internal", get_test_model_dim3_axis0_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_float16", get_test_model_dim3_axis0_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder34
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_float16_all_inputs_as_internal", get_test_model_dim3_axis0_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_quant8", get_test_model_dim3_axis0_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder35
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_quant8_all_inputs_as_internal", get_test_model_dim3_axis0_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg", get_test_model_dim3_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_all_inputs_as_internal", get_test_model_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_relaxed", get_test_model_dim3_axis0_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.8f, 0.6f, 0.0f, 0.6f, 0.0f, 0.8f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_relaxed_all_inputs_as_internal", get_test_model_dim3_axis0_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_float16", get_test_model_dim3_axis0_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 8.0f, 12.0f, 3.0f, 0.0f, 6.0f, 0.0f, 4.0f, 4.0f, 0.0f, 9.0f}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder38
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_float16_all_inputs_as_internal", get_test_model_dim3_axis0_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_quant8", get_test_model_dim3_axis0_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis0_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 230, 205, 128, 205, 128, 230, 230, 128, 205}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 112, 152, 62, 32, 92, 32, 72, 72, 32, 122}),
                            .dimensions = {3, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder39
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis0_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis0_neg_quant8_all_inputs_as_internal", get_test_model_dim3_axis0_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1 = TestModelManager::get().add("l2_normalization_axis_dim3_axis1", get_test_model_dim3_axis1());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_all_inputs_as_internal", get_test_model_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_relaxed", get_test_model_dim3_axis1_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_relaxed_all_inputs_as_internal", get_test_model_dim3_axis1_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_float16", get_test_model_dim3_axis1_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_float16_all_inputs_as_internal", get_test_model_dim3_axis1_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_quant8", get_test_model_dim3_axis1_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_quant8_all_inputs_as_internal", get_test_model_dim3_axis1_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg", get_test_model_dim3_axis1_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder44
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_all_inputs_as_internal", get_test_model_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_relaxed", get_test_model_dim3_axis1_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.6f, 0.0f, 0.0f, 0.6f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder45
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_relaxed_all_inputs_as_internal", get_test_model_dim3_axis1_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_float16", get_test_model_dim3_axis1_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f, 8.0f, 12.0f, 6.0f, 0.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder46
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_float16_all_inputs_as_internal", get_test_model_dim3_axis1_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_quant8", get_test_model_dim3_axis1_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis1_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230, 230, 230, 205, 128, 128, 205}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72, 112, 152, 92, 32, 32, 122}),
                            .dimensions = {2, 3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder47
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis1_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis1_neg_quant8_all_inputs_as_internal", get_test_model_dim3_axis1_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2 = TestModelManager::get().add("l2_normalization_axis_dim3_axis2", get_test_model_dim3_axis2());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_all_inputs_as_internal", get_test_model_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_relaxed", get_test_model_dim3_axis2_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_relaxed_all_inputs_as_internal", get_test_model_dim3_axis2_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_float16", get_test_model_dim3_axis2_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder50
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_float16_all_inputs_as_internal", get_test_model_dim3_axis2_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_quant8", get_test_model_dim3_axis2_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder51
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_quant8_all_inputs_as_internal", get_test_model_dim3_axis2_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg", get_test_model_dim3_axis2_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder52
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_all_inputs_as_internal", get_test_model_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_relaxed", get_test_model_dim3_axis2_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f, 0.8f, 0.6f, 0.0f, 0.8f, 0.0f, 0.6f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder53
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_relaxed_all_inputs_as_internal", get_test_model_dim3_axis2_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_float16", get_test_model_dim3_axis2_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.0f, 0.6000000238418579f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f, 8.0f, 6.0f, 0.0f, 12.0f, 0.0f, 9.0f}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder54
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_float16_all_inputs_as_internal", get_test_model_dim3_axis2_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_quant8", get_test_model_dim3_axis2_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim3_axis2_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230, 230, 205, 128, 230, 128, 205}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72, 112, 92, 32, 152, 32, 122}),
                            .dimensions = {2, 2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder55
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim3_axis2_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim3_axis2_neg_quant8_all_inputs_as_internal", get_test_model_dim3_axis2_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0 = TestModelManager::get().add("l2_normalization_axis_dim2_axis0", get_test_model_dim2_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_all_inputs_as_internal", get_test_model_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_relaxed = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_relaxed", get_test_model_dim2_axis0_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_relaxed_all_inputs_as_internal", get_test_model_dim2_axis0_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_float16 = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_float16", get_test_model_dim2_axis0_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // placeholder58
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_float16_all_inputs_as_internal", get_test_model_dim2_axis0_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_quant8 = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_quant8", get_test_model_dim2_axis0_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder59
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_quant8_all_inputs_as_internal", get_test_model_dim2_axis0_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg", get_test_model_dim2_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder60
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_all_inputs_as_internal", get_test_model_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_relaxed", get_test_model_dim2_axis0_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.6f, 0.0f, 0.8f, 0.8f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder61
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_relaxed_all_inputs_as_internal", get_test_model_dim2_axis0_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_float16", get_test_model_dim2_axis0_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.6000000238418579f, 0.0f, 0.800000011920929f, 0.800000011920929f}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 3.0f, 0.0f, 4.0f, 4.0f}),
                            .dimensions = {3, 2},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_float16_all_inputs_as_internal", get_test_model_dim2_axis0_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_quant8", get_test_model_dim2_axis0_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis0_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 205, 128, 230, 230}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 62, 32, 72, 72}),
                            .dimensions = {3, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder63
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis0_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis0_neg_quant8_all_inputs_as_internal", get_test_model_dim2_axis0_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1 = TestModelManager::get().add("l2_normalization_axis_dim2_axis1", get_test_model_dim2_axis1());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder64
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_all_inputs_as_internal", get_test_model_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_relaxed = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_relaxed", get_test_model_dim2_axis1_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder65
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_relaxed_all_inputs_as_internal", get_test_model_dim2_axis1_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_float16 = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_float16", get_test_model_dim2_axis1_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_float16_all_inputs_as_internal", get_test_model_dim2_axis1_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_quant8 = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_quant8", get_test_model_dim2_axis1_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder67
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_quant8_all_inputs_as_internal", get_test_model_dim2_axis1_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg", get_test_model_dim2_axis1_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder68
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_all_inputs_as_internal", get_test_model_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_relaxed", get_test_model_dim2_axis1_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f, 0.6f, 0.0f, 0.8f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder69
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_relaxed_all_inputs_as_internal", get_test_model_dim2_axis1_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_float16", get_test_model_dim2_axis1_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f, 0.6000000238418579f, 0.0f, 0.800000011920929f}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f, 3.0f, 0.0f, 4.0f}),
                            .dimensions = {2, 3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_float16_all_inputs_as_internal", get_test_model_dim2_axis1_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_quant8", get_test_model_dim2_axis1_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim2_axis1_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230, 205, 128, 230}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72, 62, 32, 72}),
                            .dimensions = {2, 3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder71
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim2_axis1_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim2_axis1_neg_quant8_all_inputs_as_internal", get_test_model_dim2_axis1_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0 = TestModelManager::get().add("l2_normalization_axis_dim1_axis0", get_test_model_dim1_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder72
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_all_inputs_as_internal", get_test_model_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_relaxed = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_relaxed", get_test_model_dim1_axis0_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder73
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_relaxed_all_inputs_as_internal", get_test_model_dim1_axis0_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_float16 = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_float16", get_test_model_dim1_axis0_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_float16_all_inputs_as_internal", get_test_model_dim1_axis0_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_quant8 = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_quant8", get_test_model_dim1_axis0_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder75
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_quant8_all_inputs_as_internal", get_test_model_dim1_axis0_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg", get_test_model_dim1_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder76
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_all_inputs_as_internal", get_test_model_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_relaxed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_relaxed = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_relaxed", get_test_model_dim1_axis0_neg_relaxed());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_relaxed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = true,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .data = TestBuffer::createFromVector<float>({0.0f, 0.6f, 0.8f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
                            .zeroPoint = 0
                        }, { // placeholder77
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.0f}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT32,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::UNKNOWN,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_relaxed_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_relaxed_all_inputs_as_internal", get_test_model_dim1_axis0_neg_relaxed_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_float16() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_float16 = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_float16", get_test_model_dim1_axis0_neg_float16());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_float16_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
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
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 0.6000000238418579f, 0.800000011920929f}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_FLOAT16,
                            .zeroPoint = 0
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<_Float16>({0.0f, 3.0f, 4.0f}),
                            .dimensions = {3},
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_float16_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_float16_all_inputs_as_internal", get_test_model_dim1_axis0_neg_float16_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_quant8() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_quant8 = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_quant8", get_test_model_dim1_axis0_neg_quant8());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_dim1_axis0_neg_quant8_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .data = TestBuffer::createFromVector<uint8_t>({128, 205, 230}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32, 62, 72}),
                            .dimensions = {3},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
                        }, { // placeholder79
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({32}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 32
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_dim1_axis0_neg_quant8_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_dim1_axis0_neg_quant8_all_inputs_as_internal", get_test_model_dim1_axis0_neg_quant8_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis0 = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis0", get_test_model_corner_case_dim2_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder80
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis0_all_inputs_as_internal", get_test_model_corner_case_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis0_neg = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis0_neg", get_test_model_corner_case_dim2_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder81
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // param81
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis0_neg_all_inputs_as_internal", get_test_model_corner_case_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis1 = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis1", get_test_model_corner_case_dim2_axis1());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder82
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // param82
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis1_all_inputs_as_internal", get_test_model_corner_case_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis1_neg = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis1_neg", get_test_model_corner_case_dim2_axis1_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0, 255}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245, 247}),
                            .dimensions = {2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder83
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // param83
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim2_axis1_neg_all_inputs_as_internal", get_test_model_corner_case_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim1_axis0 = TestModelManager::get().add("l2_normalization_axis_corner_case_dim1_axis0", get_test_model_corner_case_dim1_axis0());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder84
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // param84
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim1_axis0_all_inputs_as_internal", get_test_model_corner_case_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {0},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim1_axis0_neg = TestModelManager::get().add("l2_normalization_axis_corner_case_dim1_axis0_neg", get_test_model_corner_case_dim1_axis0_neg());

}  // namespace generated_tests::l2_normalization_axis

namespace generated_tests::l2_normalization_axis {

const TestModel& get_test_model_corner_case_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // corner_case
                .inputIndexes = {3},
                .operands = {{ // op11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // axis1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // op21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0078125f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 128
                        }, { // op11_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({245}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // placeholder85
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint8_t>({246}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.904414f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                            .zeroPoint = 246
                        }, { // param85
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::L2_NORMALIZATION
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_2,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_corner_case_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("l2_normalization_axis_corner_case_dim1_axis0_neg_all_inputs_as_internal", get_test_model_corner_case_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::l2_normalization_axis

