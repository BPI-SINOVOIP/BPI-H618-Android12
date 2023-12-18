project(gfx-streaming-kit)
cmake_minimum_required(VERSION 3.11)

if (WIN32)
    add_definitions("-DUNICODE -D_UNICODE -DNOMINMAX -DEMUGL_BUILD -DVK_USE_PLATFORM_WIN32_KHR -DBUILDING_EMUGL_COMMON_SHARED")
    add_definitions(/MP)
endif()

find_package(Threads)
include(ExternalProject)
enable_testing()

# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/distribution)
if (WIN32)
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O3")
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if (APPLE)
    add_compile_definitions(VK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX)
    # TODO(kaiyili, b/179477624): Add Linux specific Vulkan platform macro definitions
elseif(WIN32)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif()

add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)

if (MSVC)
    # ask msvc not to warn not secure C ISO functions
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # ask msvc not to warn non C ISO POSIX functions
    add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
endif()

# Uncomment for ASAN support
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
# set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

# GoogleTest support: uses CMake ExternalProject to pull in external repo
# without manual steps (during CMake build script generation phase)
include(gtest.cmake)

set(GFXSTREAM_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(android.cmake)

# Third party dependencies
add_subdirectory(third-party)

# Common base libraries for host################################################

add_subdirectory(base)
add_subdirectory(snapshot)
add_subdirectory(host-common)

# Backends######################################################################

add_subdirectory(stream-servers)

# Protocols and associated code generators######################################

add_subdirectory(protocols)

# Fake Android guest#########################3##################################

if (NOT WIN32)
    add_subdirectory(fake-android-guest)
endif()

# Frontends#####################################################################

# add_subdirectory(stream-clients)

# Test environments#############################################################

# add_subdirectory(testenvs)

# Target: Host backend to consume real guest traffic############################

# Target: Host front+back drivers for test environment##########################

# Target: Host front+back drivers for app isolation#############################

# Tests#################3#######################################################
add_test(NAME gfxstream-base_unittests COMMAND gfxstream-base_unittests)
add_test(NAME gfxstream-host-common_unittests COMMAND gfxstream-host-common_unittests)
add_test(NAME gfxstream_backend_unittests COMMAND gfxstream_backend_unittests)
add_test(NAME OpenglRender_unittests COMMAND OpenglRender_unittests)
add_test(NAME OpenglRender_snapshot_unittests COMMAND OpenglRender_snapshot_unittests)
add_test(NAME Vulkan_unittests COMMAND Vulkan_unittests)
