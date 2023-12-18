add_library(
    GLES_CM_translator_static
    CoreProfileEngine.cpp GLEScmContext.cpp GLEScmImp.cpp GLEScmUtils.cpp
    GLEScmValidate.cpp)
if (NOT MSVC)
    target_compile_options(GLES_CM_translator_static PRIVATE -fvisibility=hidden)
endif()
target_compile_options(GLES_CM_translator_static PRIVATE -Wno-macro-redefined)
target_link_libraries(GLES_CM_translator_static PRIVATE apigen-codec-common gfxstream-base)
target_include_directories(
    GLES_CM_translator_static PRIVATE
    ${GFXSTREAM_REPO_ROOT}/include
    ${GFXSTREAM_REPO_ROOT}/stream-servers/
    ${GFXSTREAM_REPO_ROOT}/stream-servers/glestranslator/include)
