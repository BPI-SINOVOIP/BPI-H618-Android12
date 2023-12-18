# Codec common sources
add_subdirectory(apigen-codec-common)
add_subdirectory(compressedTextureFormats)

# Vulkan
add_subdirectory(vulkan)

# GLES translator
add_subdirectory(glestranslator)
add_subdirectory(libGLSnapshot)

# GLES decoder
add_subdirectory(gles1_dec)
add_subdirectory(gles2_dec)

# GLES dispatch based on Translator
add_subdirectory(OpenGLESDispatch)

# RenderControl decoder
add_subdirectory(renderControl_dec)

# Stream server core
set(stream-server-core-sources
    ChannelStream.cpp
    ColorBuffer.cpp
    CompositorVk.cpp
    DisplayVk.cpp
    FbConfig.cpp
    FenceSync.cpp
    GLESVersionDetector.cpp
    PostWorker.cpp
    ReadbackWorker.cpp
    ReadBuffer.cpp
    render_api.cpp
    RenderChannelImpl.cpp
    RenderThreadInfo.cpp
    RingStream.cpp
    SwapChainStateVk.cpp
    SyncThread.cpp
    TextureDraw.cpp
    TextureResize.cpp
    WindowSurface.cpp
    YUVConverter.cpp
    RenderThread.cpp
    RenderContext.cpp
    RenderControl.cpp
    RenderWindow.cpp
    RenderLibImpl.cpp
    RendererImpl.cpp
    FrameBuffer.cpp)
if (APPLE)
    set(stream-server-core-platform-sources NativeSubWindow_cocoa.m)
elseif (WIN32)
    set(stream-server-core-platform-sources NativeSubWindow_win32.cpp)
else()
    set(stream-server-core-platform-sources NativeSubWindow_x11.cpp)
endif()

add_library(
    gfxstream_backend
    SHARED
    ${stream-server-core-sources}
    ${stream-server-core-platform-sources}
    GfxStreamAgents.cpp
    GfxStreamBackend.cpp
    virtio-gpu-gfxstream-renderer.cpp)
target_link_libraries(
    gfxstream_backend
    PUBLIC
    gfxstream-host-common
    gfxstream-base
    OpenGLESDispatch
    gles1_dec
    gles2_dec
    renderControl_dec
    gfxstream-vulkan-server
    gfxstream-snapshot
    apigen-codec-common
    perfetto-tracing-only)

if (WIN32)
    target_link_libraries(gfxstream_backend PRIVATE D3d9.lib)
    target_link_options(gfxstream_backend PRIVATE /DEBUG)
endif()

target_include_directories(
    gfxstream_backend
    PUBLIC
    ${GFXSTREAM_REPO_ROOT}
    ${GFXSTREAM_REPO_ROOT}/include
    ${GFXSTREAM_REPO_ROOT}/stream-servers
    ${GFXSTREAM_REPO_ROOT}/stream-servers/apigen-codec-common
    ${GFXSTREAM_REPO_ROOT}/stream-servers/vulkan)
android_install_shared(gfxstream_backend)

# Testing libraries
add_subdirectory(testlibs)

# Backend unit tests
add_executable(
    gfxstream_backend_unittests
    gfxstream_unittest.cpp)
target_link_libraries(
    gfxstream_backend_unittests
    PRIVATE
    OSWindow
    gfxstream_backend
    gfxstream-base
    gtest_main)

# More functional tests#########################################################

# Common testing support library################################################
# This includes the server core and testing sources
add_library(
    stream-server-testing-support
    ${stream-server-core-sources}
    ${stream-server-core-platform-sources}
    tests/SampleApplication.cpp
    tests/GLSnapshotTesting.cpp
    tests/OpenGLTestContext.cpp
    tests/GLTestUtils.cpp
    tests/ShaderUtils.cpp
    tests/GLSnapshotTestDispatch.cpp
    tests/GLSnapshotTestStateUtils.cpp
    tests/HelloTriangleImp.cpp)
target_include_directories(
    stream-server-testing-support
    PRIVATE
    ${GFXSTREAM_REPO_ROOT}
    PUBLIC
    ${GFXSTREAM_REPO_ROOT}/base/testing
    ${GFXSTREAM_REPO_ROOT}
    ${GFXSTREAM_REPO_ROOT}/include
    ${GFXSTREAM_REPO_ROOT}/stream-servers
    ${GFXSTREAM_REPO_ROOT}/stream-servers/apigen-codec-common
    ${GFXSTREAM_REPO_ROOT}/stream-servers/vulkan)
target_link_libraries(
    stream-server-testing-support
    PUBLIC
    gfxstream-base
    gfxstream-host-common
    OpenGLESDispatch
    gles1_dec
    gles2_dec
    renderControl_dec
    gfxstream-vulkan-server
    gfxstream-snapshot
    apigen-codec-common
    OSWindow
    gtest)

# Basic opengl rendering tests##################################################
add_executable(
    OpenglRender_unittests
    tests/FrameBuffer_unittest.cpp
    tests/DefaultFramebufferBlit_unittest.cpp
    tests/TextureDraw_unittest.cpp
    tests/StalePtrRegistry_unittest.cpp)
target_link_libraries(
    OpenglRender_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-base-testing-support
    gfxstream-host-common-testing-support)

# Snapshot tests################################################################
add_executable(
    OpenglRender_snapshot_unittests
    tests/GLSnapshotBuffers_unittest.cpp
    tests/GLSnapshotFramebufferControl_unittest.cpp
    tests/GLSnapshotFramebuffers_unittest.cpp
    tests/GLSnapshotMultisampling_unittest.cpp
    tests/GLSnapshotPixelOperations_unittest.cpp
    tests/GLSnapshotPixels_unittest.cpp
    tests/GLSnapshotPrograms_unittest.cpp
    tests/GLSnapshotRasterization_unittest.cpp
    tests/GLSnapshotRenderbuffers_unittest.cpp
    tests/GLSnapshotRendering_unittest.cpp
    tests/GLSnapshotShaders_unittest.cpp
    tests/GLSnapshotTextures_unittest.cpp
    tests/GLSnapshotTransformation_unittest.cpp
    tests/GLSnapshotVertexAttributes_unittest.cpp
    tests/GLSnapshot_unittest.cpp)
target_link_libraries(
    OpenglRender_snapshot_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-base-testing-support
    gfxstream-host-common-testing-support)

# Vulkan tests##################################################################
add_executable(
    Vulkan_unittests
    tests/Vulkan_unittest.cpp)
target_link_libraries(
    Vulkan_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-base-testing-support
    gfxstream-host-common-testing-support)

add_executable(
    CompositorVk_unittests
    tests/CompositorVk_unittest.cpp)
target_link_libraries(
    CompositorVk_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-host-common-testing-support)

add_executable(
    SwapChainStateVk_unittests
    tests/SwapChainStateVk_unittest.cpp)
target_link_libraries(
    SwapChainStateVk_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-host-common-testing-support)

add_executable(
    DisplayVk_unittests
    tests/DisplayVk_unittest.cpp)
target_link_libraries(
    DisplayVk_unittests
    PRIVATE
    stream-server-testing-support
    gfxstream-host-common-testing-support)
