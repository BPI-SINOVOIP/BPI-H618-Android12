# ========== grpc proto prebuilts =================

if (NOT trout_HOST_PROGRAM_PATH)
  set(trout_HOST_PROGRAM_PATH ${trout_SRC_ROOT}/out/host_build/bin)
endif()

# find Android prebuilt programs first
find_program(trout_PROTOC aprotoc)
find_program(trout_GRPC_CPP_PLUGIN protoc-gen-grpc-cpp-plugin)

# If not found in Android prebuilts, then find in the host program path
find_program(trout_PROTOC protoc
             PATHS ${trout_HOST_PROGRAM_PATH}
             NO_DEFAULT_PATH)
find_program(trout_GRPC_CPP_PLUGIN grpc_cpp_plugin
             PATHS ${trout_HOST_PROGRAM_PATH}
             NO_DEFAULT_PATH)

if (NOT trout_GENS_DIR)
  set(trout_GENS_DIR ${PROJECT_BINARY_DIR}/gens)
endif()

if(NOT _trout_PROTOBUF_WELLKNOWN_INCLUDE_DIR)
  message(FATAL_ERROR "_trout_PROTOBUF_WELLKNOWN_INCLUDE_DIR not set")
endif()


function(trout_generate_grpc_cpp_from_proto)
  cmake_parse_arguments(
      PARSED_ARGS
      "" # no boolean args
      "" # no single value args
      "INCLUDES;SRCS" # multi value args
      ${ARGN}
  )

  if(NOT PARSED_ARGS_SRCS)
    message(SEND_ERROR "Error: trout_generate_grpc_cpp_from_proto() called without any proto files")
    return()
  endif()

  if(PARSED_ARGS_UNPARSED_ARGUMENTS)
    message(SEND_ERROR "Unknown arguments: ${PARSED_ARGS_UNPARSED_ARGUMENTS}")
    return()
  endif()

  set(_protobuf_include_path -I . -I ${_trout_PROTOBUF_WELLKNOWN_INCLUDE_DIR})
  foreach(INCLUDE_PATH ${PARSED_ARGS_INCLUDES})
      list(APPEND _protobuf_include_path -I ${INCLUDE_PATH})
  endforeach()

  foreach(FIL ${PARSED_ARGS_SRCS})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(ABS_DIR ${ABS_FIL} DIRECTORY)
    get_filename_component(FIL_NAME ${FIL} NAME)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    add_custom_command(
      OUTPUT "${trout_GENS_DIR}/${FIL_WE}.grpc.pb.cc"
             "${trout_GENS_DIR}/${FIL_WE}.grpc.pb.h"
             "${trout_GENS_DIR}/${FIL_WE}.pb.cc"
             "${trout_GENS_DIR}/${FIL_WE}.pb.h"
             COMMAND ${trout_PROTOC}
      ARGS --grpc_out=generate_mock_code=true:${trout_GENS_DIR}
           --cpp_out=${trout_GENS_DIR}
           --plugin=protoc-gen-grpc=${trout_GRPC_CPP_PLUGIN}
           ${_protobuf_include_path}
           ${FIL_NAME}
      DEPENDS ${ABS_FIL} ${trout_PROTOC} ${trout_GRPC_CPP_PLUGIN}
      WORKING_DIRECTORY ${ABS_DIR}
      COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
      VERBATIM)

      set_source_files_properties("${trout_GENS_DIR}/${FIL_WE}.grpc.pb.cc" "${trout_GENS_DIR}/${FIL_WE}.grpc.pb.h" "${trout_GENS_DIR}/${FIL_WE}.pb.cc" "${trout_GENS_DIR}/${FIL_WE}.pb.h" PROPERTIES GENERATED TRUE)
  endforeach()
endfunction()


