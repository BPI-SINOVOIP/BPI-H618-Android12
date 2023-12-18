// Generated from softmax_quant8_signed.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // input
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-127, -126, -118, -108}),
                            .dimensions = {1, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // beta
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-05f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // output
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-64, -64, -64, -64}),
                            .dimensions = {1, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model = TestModelManager::get().add("softmax_quant8_signed", get_test_model());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // input
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {1, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // beta
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-05f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // output
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-64, -64, -64, -64}),
                            .dimensions = {1, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // input_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-127, -126, -118, -108}),
                            .dimensions = {1, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // placeholder
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_all_inputs_as_internal", get_test_model_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // input1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-127, -126, -125, -124, -123, 127, 126, 125, 124, 123}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // beta1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // output1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-113, -104, -88, -61, -18, -18, -61, -88, -104, -113}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_2 = TestModelManager::get().add("softmax_quant8_signed_2", get_test_model_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // input1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // beta1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // output1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-113, -104, -88, -61, -18, -18, -61, -88, -104, -113}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // input1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-127, -126, -125, -124, -123, 127, 126, 125, 124, 123}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // placeholder1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-128}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.5f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_all_inputs_as_internal_2", get_test_model_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed = TestModelManager::get().add("softmax_quant8_signed_quant8_signed", get_test_model_quant8_signed());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_all_inputs_as_internal", get_test_model_quant8_signed_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim1_axis0 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim1_axis0", get_test_model_quant8_signed_dim1_axis0());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim1_axis0_all_inputs_as_internal", get_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim3_axis2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim3_axis2", get_test_model_quant8_signed_dim3_axis2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim3_axis2_all_inputs_as_internal", get_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_2", get_test_model_quant8_signed_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder5
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {3, 4, 5},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_all_inputs_as_internal_2", get_test_model_quant8_signed_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim1_axis0_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim1_axis0_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim1_axis0_2", get_test_model_quant8_signed_dim1_axis0_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim1_axis0_all_inputs_as_internal_2", get_test_model_quant8_signed_dim1_axis0_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim3_axis2_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1},
                            .outputs = {2},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim3_axis2_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim3_axis2_2", get_test_model_quant8_signed_dim3_axis2_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = {
                .inputIndexes = {3},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // op2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder7
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {2}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_quant8_signed_dim3_axis2_all_inputs_as_internal_2", get_test_model_quant8_signed_dim3_axis2_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 68, -4, 68, -4, 64, -8, 64, -8, 64, -8, 64, -8, 60, -12, 60, -12, 60, -12, 60, -12, 56, -16, 56, -16, 56, -16, 56, -16, 4, -68, 4, -68, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, 37, 37, 37, 37, -67, -67, -67, -67, -67, -67, -67, -67, -106, -106, -106, -106, -106, -106, -106, -106, -120, -120, -120, -120, -120, -120, -120, -120, -128, -128, -128, -128, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0", get_test_model_axis_quant8_signed_dim4_axis0());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, 37, 37, 37, 37, -67, -67, -67, -67, -67, -67, -67, -67, -106, -106, -106, -106, -106, -106, -106, -106, -120, -120, -120, -120, -120, -120, -120, -120, -128, -128, -128, -128, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 68, -4, 68, -4, 64, -8, 64, -8, 64, -8, 64, -8, 60, -12, 60, -12, 60, -12, 60, -12, 56, -16, 56, -16, 56, -16, 56, -16, 4, -68, 4, -68, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder8
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 68, -4, 68, -4, 64, -8, 64, -8, 64, -8, 64, -8, 60, -12, 60, -12, 60, -12, 60, -12, 56, -16, 56, -16, 56, -16, 56, -16, 4, -68, 4, -68, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, 37, 37, 37, 37, -67, -67, -67, -67, -67, -67, -67, -67, -106, -106, -106, -106, -106, -106, -106, -106, -120, -120, -120, -120, -120, -120, -120, -120, -128, -128, -128, -128, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_neg", get_test_model_axis_quant8_signed_dim4_axis0_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, 37, 37, 37, 37, -67, -67, -67, -67, -67, -67, -67, -67, -106, -106, -106, -106, -106, -106, -106, -106, -120, -120, -120, -120, -120, -120, -120, -120, -128, -128, -128, -128, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 68, -4, 68, -4, 64, -8, 64, -8, 64, -8, 64, -8, 60, -12, 60, -12, 60, -12, 60, -12, 56, -16, 56, -16, 56, -16, 56, -16, 4, -68, 4, -68, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder9
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68, 68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128, 37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1", get_test_model_axis_quant8_signed_dim4_axis1());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128, 37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68, 68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder10
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68, 68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128, 37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_neg", get_test_model_axis_quant8_signed_dim4_axis1_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128, 37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68, 68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2", get_test_model_axis_quant8_signed_dim4_axis2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_neg", get_test_model_axis_quant8_signed_dim4_axis2_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3", get_test_model_axis_quant8_signed_dim4_axis3());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_neg", get_test_model_axis_quant8_signed_dim4_axis3_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0", get_test_model_axis_quant8_signed_dim3_axis0());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder16
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_neg", get_test_model_axis_quant8_signed_dim3_axis0_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, 37, 37, -67, -67, -67, -67, -106, -106, -106, -106, -120, -120, -120, -120, -128, -128, -128, -128}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 68, -4, 64, -8, 64, -8, 60, -12, 60, -12, 56, -16, 56, -16, 4, -68, 4, -68}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder17
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1", get_test_model_axis_quant8_signed_dim3_axis1());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder18
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_neg", get_test_model_axis_quant8_signed_dim3_axis1_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128, 37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68, 68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder19
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2", get_test_model_axis_quant8_signed_dim3_axis2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder20
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_neg", get_test_model_axis_quant8_signed_dim3_axis2_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68, 68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder21
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0", get_test_model_axis_quant8_signed_dim2_axis0());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder22
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_neg", get_test_model_axis_quant8_signed_dim2_axis0_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, 37, -67, -67, -106, -106, -120, -120, -128, -128}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, -4, 64, -8, 60, -12, 56, -16, 4, -68}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder23
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1", get_test_model_axis_quant8_signed_dim2_axis1());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder24
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_neg", get_test_model_axis_quant8_signed_dim2_axis1_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128, 37, -67, -106, -120, -128}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4, -4, -8, -12, -16, -68}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder25
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0", get_test_model_axis_quant8_signed_dim1_axis0());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder26
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_neg() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_neg = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_neg", get_test_model_axis_quant8_signed_dim1_axis0_neg());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param2
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({37, -67, -106, -120, -128}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({68, 64, 60, 56, 4}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder27
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal", get_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 4, -4, 4, -4, 8, -8, 8, -8, 8, -8, 8, -8, 12, -12, 12, -12, 12, -12, 12, -12, 16, -16, 16, -16, 16, -16, 16, -16, 20, -20, 20, -20, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_2", get_test_model_axis_quant8_signed_dim4_axis0_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 4, -4, 4, -4, 8, -8, 8, -8, 8, -8, 8, -8, 12, -12, 12, -12, 12, -12, 12, -12, 16, -16, 16, -16, 16, -16, 16, -16, 20, -20, 20, -20, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder28
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis0_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 4, -4, 4, -4, 8, -8, 8, -8, 8, -8, 8, -8, 12, -12, 12, -12, 12, -12, 12, -12, 16, -16, 16, -16, 16, -16, 16, -16, 20, -20, 20, -20, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_neg_2", get_test_model_axis_quant8_signed_dim4_axis0_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 4, -4, 4, -4, 8, -8, 8, -8, 8, -8, 8, -8, 12, -12, 12, -12, 12, -12, 12, -12, 16, -16, 16, -16, 16, -16, 16, -16, 20, -20, 20, -20, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder29
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis0_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20, 4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_2", get_test_model_axis_quant8_signed_dim4_axis1_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20, 4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder30
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis1_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20, 4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_neg_2", get_test_model_axis_quant8_signed_dim4_axis1_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20, 4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {2, 5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder31
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis1_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_2", get_test_model_axis_quant8_signed_dim4_axis2_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder32
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis2_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_neg_2", get_test_model_axis_quant8_signed_dim4_axis2_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder33
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis2_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_2", get_test_model_axis_quant8_signed_dim4_axis3_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder34
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis3_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_neg_2", get_test_model_axis_quant8_signed_dim4_axis3_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder35
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim4_axis3_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_2", get_test_model_axis_quant8_signed_dim3_axis0_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder36
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis0_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_neg_2", get_test_model_axis_quant8_signed_dim3_axis0_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 4, -4, 8, -8, 8, -8, 12, -12, 12, -12, 16, -16, 16, -16, 20, -20, 20, -20}),
                            .dimensions = {5, 2, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder37
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis0_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_2", get_test_model_axis_quant8_signed_dim3_axis1_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder38
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis1_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_neg_2", get_test_model_axis_quant8_signed_dim3_axis1_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20, 4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {2, 5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder39
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis1_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_2", get_test_model_axis_quant8_signed_dim3_axis2_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder40
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis2_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_neg_2", get_test_model_axis_quant8_signed_dim3_axis2_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20, 4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder41
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim3_axis2_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_2", get_test_model_axis_quant8_signed_dim2_axis0_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder42
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .inputs = {4, 5, 6},
                            .outputs = {0},
                            .type = TestOperationType::ADD
                        }, {
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim2_axis0_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_neg_2", get_test_model_axis_quant8_signed_dim2_axis0_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, -4, 8, -8, 12, -12, 16, -16, 20, -20}),
                            .dimensions = {5, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder43
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim2_axis0_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_2", get_test_model_axis_quant8_signed_dim2_axis1_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder44
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim2_axis1_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_neg_2", get_test_model_axis_quant8_signed_dim2_axis1_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77, -77, -77, -77, -77, -77}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20, -4, -8, -12, -16, -20}),
                            .dimensions = {2, 5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder45
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim2_axis1_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_2", get_test_model_axis_quant8_signed_dim1_axis0_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder46
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim1_axis0_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_neg_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {0},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2},
                            .outputs = {3},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_neg_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_neg_2", get_test_model_axis_quant8_signed_dim1_axis0_neg_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal_2() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // axis
                .inputIndexes = {4},
                .operands = {{ // op1
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param3
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1e-06f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
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
                            .data = TestBuffer::createFromVector<int8_t>({-77, -77, -77, -77, -77}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }, { // op1_new
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({4, 8, 12, 16, 20}),
                            .dimensions = {5},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // placeholder47
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.25f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
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
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {3}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal_2 = TestModelManager::get().add("softmax_quant8_signed_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal_2", get_test_model_axis_quant8_signed_dim1_axis0_neg_all_inputs_as_internal_2());

}  // namespace generated_tests::softmax_quant8_signed

namespace generated_tests::softmax_quant8_signed {

const TestModel& get_test_model_zero_sized_quant8_signed() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .isRelaxed = false,
        .main = { // zero_sized
                .inputIndexes = {13},
                .operands = {{ // scores
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({9, 1}),
                            .dimensions = {1, 2},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // roi
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint16_t>({8, 8, 80, 80, 0, 0, 80, 80}),
                            .dimensions = {1, 8},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.125f,
                            .type = TestOperandType::TENSOR_QUANT16_ASYMM,
                            .zeroPoint = 0
                        }, { // param4
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({0}),
                            .dimensions = {1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // param5
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.3f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // param6
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({-1}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
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
                        }, { // param8
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.4f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // param9
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // param10
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({0.3f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // scoresOut
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {0},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // roiOut
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<uint16_t>({}),
                            .dimensions = {0, 4},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.125f,
                            .type = TestOperandType::TENSOR_QUANT16_ASYMM,
                            .zeroPoint = 0
                        }, { // classesOut
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({}),
                            .dimensions = {0},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // batchSplitOut
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({}),
                            .dimensions = {0},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::TENSOR_INT32,
                            .zeroPoint = 0
                        }, { // in
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({10}),
                            .dimensions = {1, 1, 1, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param11
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // param12
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({2}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // param13
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({2.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // param14
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({2.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // param15
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
                            .zeroPoint = 0
                        }, { // param16
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int32_t>({4}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::INT32,
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
                        }, { // featureMap
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {0, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                            .numberOfConsumers = 1,
                            .scale = 0.1f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = 0
                        }, { // param17
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<float>({1.0f}),
                            .dimensions = {},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                            .numberOfConsumers = 1,
                            .scale = 0.0f,
                            .type = TestOperandType::FLOAT32,
                            .zeroPoint = 0
                        }, { // out
                            .channelQuant = {},
                            .data = TestBuffer::createFromVector<int8_t>({}),
                            .dimensions = {0, 2, 2, 1},
                            .isIgnored = false,
                            .lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT,
                            .numberOfConsumers = 0,
                            .scale = 0.00390625f,
                            .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                            .zeroPoint = -128
                        }},
                .operations = {{
                            .inputs = {0, 1, 2, 3, 4, 5, 6, 7, 8},
                            .outputs = {9, 10, 11, 12},
                            .type = TestOperationType::BOX_WITH_NMS_LIMIT
                        }, {
                            .inputs = {13, 10, 12, 14, 15, 16, 17, 18, 19, 20},
                            .outputs = {21},
                            .type = TestOperationType::ROI_ALIGN
                        }, {
                            .inputs = {21, 22},
                            .outputs = {23},
                            .type = TestOperationType::SOFTMAX
                        }},
                .outputIndexes = {9, 11, 23}
            },
        .minSupportedVersion = TestHalVersion::V1_3,
        .referenced = {}
    };
    return model;
}

const auto dummy_test_model_zero_sized_quant8_signed = TestModelManager::get().add("softmax_quant8_signed_zero_sized_quant8_signed", get_test_model_zero_sized_quant8_signed());

}  // namespace generated_tests::softmax_quant8_signed

