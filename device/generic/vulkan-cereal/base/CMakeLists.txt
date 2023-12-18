if (WIN32)
endif()

set(gfxstream-base-common-sources
    AlignedBuf.cpp
    CompressingStream.cpp
    CpuTime.cpp
    DecompressingStream.cpp
    FileUtils.cpp
    FunctorThread.cpp
    GLObjectCounter.cpp
    LayoutResolver.cpp
    MemStream.cpp
    StdioStream.cpp
    MemoryTracker.cpp
    MessageChannel.cpp
    PathUtils.cpp
    ring_buffer.cpp
    SharedLibrary.cpp
    StringFormat.cpp
    Stream.cpp
    StreamSerializing.cpp
    SubAllocator.cpp
    System.cpp
    Tracing.cpp)
set(gfxstream-base-posix-sources
    SharedMemory_posix.cpp
    Thread_pthread.cpp)
set(gfxstream-base-windows-sources
    msvc.cpp
    SharedMemory_win32.cpp
    Thread_win32.cpp
    Win32UnicodeString.cpp)

if (APPLE)
    set(gfxstream-platform-sources
        ${gfxstream-base-posix-sources}
        system-native-mac.mm)
elseif (WIN32)
    set(gfxstream-platform-sources
        ${gfxstream-base-windows-sources})
else()
    set(gfxstream-platform-sources
        ${gfxstream-base-posix-sources})
endif()

add_library(
    gfxstream-base
    ${gfxstream-base-common-sources}
    ${gfxstream-platform-sources})

target_link_libraries(
    gfxstream-base PRIVATE lz4 perfetto-tracing-only)
target_include_directories(
    gfxstream-base PUBLIC ${GFXSTREAM_REPO_ROOT})
if (NOT MSVC)
    target_compile_options(
        gfxstream-base PRIVATE -fvisibility=default)
endif()

if (APPLE)
    set(gfxstream-base-platform-test-sources "")
elseif (WIN32)
    set(gfxstream-base-platform-test-sources Win32UnicodeString_unittest.cpp)
else()
    set(gfxstream-base-platform-test-sources "")
endif()

# Tests
add_library(
    gfxstream-base-testing-support
    testing/file_io.cpp)
target_include_directories(
    gfxstream-base-testing-support
    PRIVATE
    ${GFXSTREAM_REPO_ROOT}
    PUBLIC
    ${GFXSTREAM_REPO_ROOT}/base/testing)
target_link_libraries(
    gfxstream-base-testing-support
    PRIVATE
    gfxstream-base
    gtest
    gmock)
add_executable(
    gfxstream-base_unittests
    AlignedBuf_unittest.cpp
    ArraySize_unittest.cpp
    LayoutResolver_unittest.cpp
    Optional_unittest.cpp
    ring_buffer_unittest.cpp
    StringFormat_unittest.cpp
    SubAllocator_unittest.cpp
    TypeTraits_unittest.cpp
    ${gfxstream-base-platform-test-sources})
target_link_libraries(
    gfxstream-base_unittests
    PRIVATE
    gfxstream-base
    gfxstream-base-testing-support
    gtest_main)
