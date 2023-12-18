add_library(GLSnapshot GLSnapshot.cpp)
target_link_libraries(GLSnapshot PUBLIC apigen-codec-common)
target_include_directories(GLSnapshot PUBLIC
    ${GFXSTREAM_REPO_ROOT}/stream-servers/libGLSnapshot
    ${GFXSTREAM_REPO_ROOT}/include
    ${GFXSTREAM_REPO_ROOT}/stream-servers
    ${GFXSTREAM_REPO_ROOT}/stream-servers/OpenGLESDispatch)
