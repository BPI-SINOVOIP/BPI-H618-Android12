cmake_minimum_required(VERSION 3.5)

set(PERFETTO_TRACING_ONLY_ROOT ${GFXSTREAM_REPO_ROOT}/third-party/perfetto-tracing-only)
set(PERFETTO_ROOT ${GFXSTREAM_REPO_ROOT}/third-party/perfetto)

#######################################
### perfetto-libperfettobase
#######################################

set(PERFETTO_LIBPERFETTOBASE_SRC
    ${PERFETTO_ROOT}/src/base/logging.cc
    ${PERFETTO_ROOT}/src/base/metatrace.cc
    ${PERFETTO_ROOT}/src/base/paged_memory.cc
    ${PERFETTO_ROOT}/src/base/string_splitter.cc
    ${PERFETTO_ROOT}/src/base/string_utils.cc
    ${PERFETTO_ROOT}/src/base/string_view.cc
    ${PERFETTO_ROOT}/src/base/subprocess.cc
    ${PERFETTO_ROOT}/src/base/thread_checker.cc
    ${PERFETTO_ROOT}/src/base/time.cc
    ${PERFETTO_ROOT}/src/base/uuid.cc
    ${PERFETTO_ROOT}/src/base/virtual_destructors.cc
    ${PERFETTO_ROOT}/src/base/waitable_event.cc
    ${PERFETTO_ROOT}/src/base/watchdog_posix.cc)

add_library(perfetto-libperfettobase ${PERFETTO_LIBPERFETTOBASE_SRC})
target_include_directories(
    perfetto-libperfettobase PUBLIC
    ${PERFETTO_TRACING_ONLY_ROOT}
    ${PERFETTO_ROOT}/include)

#######################################
### perfetto-libprotozero
#######################################

set(PERFETTO_LIBPROTOZERO_SRC
    ${PERFETTO_ROOT}/src/protozero/field.cc
    ${PERFETTO_ROOT}/src/protozero/message.cc
    ${PERFETTO_ROOT}/src/protozero/message_arena.cc
    ${PERFETTO_ROOT}/src/protozero/message_handle.cc
    ${PERFETTO_ROOT}/src/protozero/packed_repeated_fields.cc
    ${PERFETTO_ROOT}/src/protozero/proto_decoder.cc
    ${PERFETTO_ROOT}/src/protozero/scattered_heap_buffer.cc
    ${PERFETTO_ROOT}/src/protozero/scattered_stream_null_delegate.cc
    ${PERFETTO_ROOT}/src/protozero/scattered_stream_writer.cc
    ${PERFETTO_ROOT}/src/protozero/static_buffer.cc
    ${PERFETTO_ROOT}/src/protozero/virtual_destructors.cc)

add_library(perfetto-libprotozero ${PERFETTO_LIBPROTOZERO_SRC})
target_include_directories(
    perfetto-libprotozero PUBLIC
    ${PERFETTO_TRACING_ONLY_ROOT}
    ${PERFETTO_ROOT}/include)

target_link_libraries(perfetto-libprotozero PUBLIC perfetto-libperfettobase)

#######################################
### perfetto-trace-only
#######################################
set(PERFETTO_TRACING_ONLY_SOURCES
    perfetto-tracing-only.cpp)

set(PERFETTO_TRACING_ONLY_INCLUDE ${PERFETTO_TRACING_ONLY_ROOT} ${PERFETTO_ROOT}/include)

add_library(perfetto-tracing-only ${PERFETTO_TRACING_ONLY_SOURCES})
target_include_directories(perfetto-tracing-only PUBLIC ${PERFETTO_TRACING_ONLY_INCLUDE})
target_link_libraries(perfetto-tracing-only perfetto-libprotozero)
