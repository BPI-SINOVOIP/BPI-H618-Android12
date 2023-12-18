if(NOT trout_PROTOBUF_ROOT_DIR)
    set(trout_PROTOBUF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf)
endif()

if(EXISTS "${trout_PROTOBUF_ROOT_DIR}/cmake/CMakeLists.txt")
  set(_trout_PROTOBUF_WELLKNOWN_INCLUDE_DIR "${trout_PROTOBUF_ROOT_DIR}/src")
else()
  message(FATAL_ERROR "${trout_PROTOBUF_ROOT_DIR}/cmake/CMakeLists.txt not found")
endif()

