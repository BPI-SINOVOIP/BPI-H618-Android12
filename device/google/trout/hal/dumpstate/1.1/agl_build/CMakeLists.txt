set(_dumpstate_CXX_FLAGS -Wall -Werror -std=c++17)

include(${trout_SRC_ROOT}/cmake/googletest.cmake)
include(${trout_SRC_ROOT}/cmake/grpc.cmake)
include(${trout_SRC_ROOT}/cmake/protobuf.cmake)
include(${trout_SRC_ROOT}/cmake/utils/grpc_cpp_generator.cmake)

find_package(LibXml2 REQUIRED)

get_filename_component(dumpstate_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

if (NOT trout_GENS_DIR)
  set(trout_GENS_DIR ${PROJECT_BINARY_DIR}/gens)
endif()

trout_generate_grpc_cpp_from_proto(
    SRCS
    ${dumpstate_SRC_ROOT}/proto/DumpstateServer.proto)

# ========== Protobuf generated code ==================

add_library(dumpstate_protobuf_gen
    ${trout_GENS_DIR}/DumpstateServer.pb.cc
    ${trout_GENS_DIR}/DumpstateServer.grpc.pb.cc
)

target_link_libraries(dumpstate_protobuf_gen
    ${trout_GRPCXX_LIBRARIES}
)

target_compile_options(dumpstate_protobuf_gen PRIVATE ${_dumpstate_CXX_FLAGS} -Wno-array-bounds)

# ========== dumpstate_grpc_server ==============

set(_dumpstate_sources
    ${dumpstate_SRC_ROOT}/DumpstateServer.cpp
    ${dumpstate_SRC_ROOT}/DumpstateGrpcServer.cpp
    ${dumpstate_SRC_ROOT}/ServiceDescriptor.cpp
    ${dumpstate_SRC_ROOT}/ServiceSupplier.cpp
    ${dumpstate_SRC_ROOT}/config/dumpstate_hal_configuration_V1_0.cpp
    ${dumpstate_SRC_ROOT}/config/XmlServiceSupplier.cpp
)

add_executable(dumpstate_grpc_server
    ${_dumpstate_sources}
    ${dumpstate_SRC_ROOT}/server_main.cpp
)

target_include_directories(dumpstate_grpc_server
    PRIVATE ${trout_GENS_DIR}
    ${dumpstate_SRC_ROOT}
    ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(dumpstate_grpc_server
    dumpstate_protobuf_gen
    ${LIBXML2_LIBRARIES}
)

target_compile_options(dumpstate_grpc_server PRIVATE ${_dumpstate_CXX_FLAGS})

add_executable(dumpstate_tests
    ${_dumpstate_sources}
    ${dumpstate_SRC_ROOT}/tests/test_main.cpp
    ${dumpstate_SRC_ROOT}/tests/tests.cpp
)

target_include_directories(dumpstate_tests
    PRIVATE ${trout_GENS_DIR}
    ${dumpstate_SRC_ROOT}
    ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(dumpstate_tests
    dumpstate_protobuf_gen
    ${trout_GOOGLETEST_LIBRARIES}
    ${LIBXML2_LIBRARIES}
)

target_compile_options(dumpstate_tests PRIVATE ${_dumpstate_CXX_FLAGS})
