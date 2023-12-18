if (APPLE)
    set(oswindow-platform-sources
        osx/OSXWindow.mm)
elseif (WIN32)
    set(oswindow-platform-sources
        windows/WindowsTimer.cpp
        windows/Windows_system_utils.cpp
        windows/win32/Win32Window.cpp)
else()
    set(oswindow-platform-sources
        x11/X11Window.cpp)
endif()

add_library(
    OSWindow
    OSWindow.cpp
    ${oswindow-platform-sources})
target_link_libraries(
    OSWindow
    PRIVATE
    gfxstream-base)
target_include_directories(
    OSWindow
    PUBLIC
    .
    PRIVATE
    ${GFXSTREAM_REPO_ROOT}/include)
