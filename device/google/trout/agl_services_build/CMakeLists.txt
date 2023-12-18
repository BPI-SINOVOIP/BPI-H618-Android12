cmake_minimum_required(VERSION 3.13)

project(agl_services CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    CACHE PATH "Path of static libraries.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    CACHE PATH "Path of dynamic libraries.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    CACHE PATH "Path of executables.")
mark_as_advanced(CMAKE_RUNTIME_OUTPUT_DIRECTORY CMAKE_LIBRARY_OUTPUT_DIRECTORY CMAKE_ARCHIVE_OUTPUT_DIRECTORY)

get_filename_component(trout_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# ========== grpc build settings ===================
set(BORINGSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boringssl)
set(CARES_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cares)
set(GFLAGS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gflags)
set(PROTOBUF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf)
set(ZLIB_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib)


if (CMAKE_CROSSCOMPILING)
    # ========== vehicle hal server build settings ===================
    set(trout_ANDROID_SYSCORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/android/system_core)
    set(trout_FMTLIB_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmtlib)
    set(trout_GOOGLETEST_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest)
    set(trout_GRPC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc-grpc)
    set(trout_JSONCPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsoncpp)
    set(trout_NATIVE_VHAL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/default_native_vehicle_hal)
    set(trout_PROTOBUF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dumpstate_server/agl_build)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vehicle_hal_server/agl_build)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/watchdog_test_server)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vehicle_hal_server/garage_mode_helper)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/audiocontrol/aidl/1.0/agl_build)

else()
    # ========== grpc host tools ===================
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc-grpc)
endif()
