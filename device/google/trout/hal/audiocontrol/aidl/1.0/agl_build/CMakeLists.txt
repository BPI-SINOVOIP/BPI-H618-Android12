set(_trout_AUDIOFOCUS_CXX_FLAGS -Wall -Werror -std=c++17)

include(${trout_SRC_ROOT}/cmake/grpc.cmake)
include(${trout_SRC_ROOT}/cmake/protobuf.cmake)
include(${trout_SRC_ROOT}/cmake/utils/grpc_cpp_generator.cmake)

get_filename_component(trout_AUDIOFOCUS_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

if (NOT trout_GENS_DIR)
  set(trout_GENS_DIR ${PROJECT_BINARY_DIR}/gens)
endif()

trout_generate_grpc_cpp_from_proto(
    SRCS
    ${trout_AUDIOFOCUS_SRC_ROOT}/proto/AudioFocusControl.proto)

# ========== Protobuf generated code ==================

add_library(audiocontrol_protobuf_gen
    ${trout_GENS_DIR}/AudioFocusControl.pb.cc
    ${trout_GENS_DIR}/AudioFocusControl.grpc.pb.cc
)

target_link_libraries(audiocontrol_protobuf_gen
    ${trout_GRPCXX_LIBRARIES}
)

target_compile_options(audiocontrol_protobuf_gen PRIVATE ${_trout_AUDIOFOCUS_CXX_FLAGS} -Wno-array-bounds)

# ========== libandroid_audio_controller ==============

add_library(android_audio_controller
    ${trout_AUDIOFOCUS_SRC_ROOT}/libandroid_audio_controller/android_audio_controller.cpp
)

target_include_directories(android_audio_controller
    PRIVATE ${trout_GENS_DIR}
)

target_link_libraries(android_audio_controller
    audiocontrol_protobuf_gen
)

target_compile_options(android_audio_controller PRIVATE ${_trout_AUDIOFOCUS_CXX_FLAGS})

# ========== libandroid_audio_controller_test ==============

add_executable(android_audio_controller_test ${trout_AUDIOFOCUS_SRC_ROOT}/libandroid_audio_controller_test/main.cpp)

target_compile_options(android_audio_controller_test PRIVATE ${_trout_AUDIOFOCUS_CXX_FLAGS})

target_include_directories(android_audio_controller_test
    PRIVATE ${trout_GENS_DIR}
    PRIVATE ${trout_AUDIOFOCUS_SRC_ROOT}/libandroid_audio_controller
)

target_link_libraries(android_audio_controller_test android_audio_controller)
