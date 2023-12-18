set(_trout_VHAL_CXX_FLAGS -Wall -Werror -std=c++17)

include(${trout_SRC_ROOT}/cmake/fmtlib.cmake)
include(${trout_SRC_ROOT}/cmake/android.cmake)
include(${trout_SRC_ROOT}/cmake/default_native_vhal.cmake)
include(${trout_SRC_ROOT}/cmake/grpc.cmake)
include(${trout_SRC_ROOT}/cmake/jsoncpp.cmake)
include(${trout_SRC_ROOT}/cmake/protobuf.cmake)
include(${trout_SRC_ROOT}/cmake/utils/grpc_cpp_generator.cmake)

get_filename_component(trout_VHAL_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
set(trout_VHAL_PREBUILT_DIR ${trout_VHAL_SRC_ROOT}/agl_build/prebuilt)

if (NOT trout_GENS_DIR)
  set(trout_GENS_DIR ${PROJECT_BINARY_DIR}/gens)
endif()


trout_generate_grpc_cpp_from_proto(
    INCLUDES
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/proto
    SRCS
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/proto/VehicleHalProto.proto
    ${trout_VHAL_SRC_ROOT}/proto/VehicleServer.proto)

# ========== Protobuf generated code ==================

add_library(vehicle_hal_protobuf_gen
    ${trout_GENS_DIR}/VehicleHalProto.pb.cc
    ${trout_GENS_DIR}/VehicleHalProto.grpc.pb.cc
    ${trout_GENS_DIR}/VehicleServer.pb.cc
    ${trout_GENS_DIR}/VehicleServer.grpc.pb.cc
)

target_link_libraries(vehicle_hal_protobuf_gen
    ${trout_GRPCXX_LIBRARIES}
)

target_compile_options(vehicle_hal_protobuf_gen PRIVATE ${_trout_VHAL_CXX_FLAGS} -Wno-array-bounds)

# ========== vehicle_hal_grpc_server ==============

add_executable(vehicle_hal_grpc_server
    ${trout_VHAL_SRC_ROOT}/VirtualizationGrpcServer.cpp
    ${trout_VHAL_SRC_ROOT}/GarageModeServerSideHandler.cpp
    ${trout_VHAL_SRC_ROOT}/GrpcVehicleServer.cpp
    ${trout_VHAL_SRC_ROOT}/PowerStateListener.cpp
    ${trout_VHAL_SRC_ROOT}/Utils.cpp

    ${trout_NATIVE_VHAL_ROOT_DIR}/common/src/Obd2SensorStore.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/common/src/VehicleObjectPool.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/common/src/VehicleUtils.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/common/src/VehiclePropertyStore.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/VehicleHalServer.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/GeneratorHub.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/JsonFakeValueGenerator.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/LinearFakeValueGenerator.cpp
    ${trout_NATIVE_VHAL_ROOT_DIR}/impl/vhal_v2_0/ProtoMessageConverter.cpp
)

target_include_directories(vehicle_hal_grpc_server
    PRIVATE ${trout_ANDROID_INCLUDE_DIRS}
    PRIVATE ${trout_GENS_DIR}
    PRIVATE ${trout_JSONCPP_INCLUDE_DIRS}
    PRIVATE ${trout_NAVTIVE_VHAL_COMMON_INCLUDE_DIRS}
    PRIVATE ${trout_NAVTIVE_VHAL_IMPL_INCLUDE_DIRS}
    PRIVATE ${trout_VHAL_PREBUILT_DIR}/include
)

target_link_libraries(vehicle_hal_grpc_server
    vehicle_hal_protobuf_gen
    ${trout_ANDROID_LIBRARIES}
    ${trout_JSONCPP_LIBRARIES}
)

target_compile_options(vehicle_hal_grpc_server PRIVATE ${_trout_VHAL_CXX_FLAGS})
