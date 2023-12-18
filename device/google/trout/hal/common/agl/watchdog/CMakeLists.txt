cmake_minimum_required(VERSION 3.13)

project(agl_watchdog CXX)

set(_trout_WATCHDOG_CXX_FLAGS_ -Wall -Werror -std=c++17)

add_executable(watchdog_test_service watchdog.cpp test/service.cpp)
target_compile_options(watchdog_test_service PRIVATE ${_trout_WATCHDOG_CXX_FLAGS_})
target_include_directories(watchdog_test_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(watchdog_test_service systemd pthread)
