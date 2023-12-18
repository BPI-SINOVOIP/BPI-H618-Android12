#include "X11Support.h"

#include "base/SharedLibrary.h"

#define DEFINE_DUMMY_IMPL(rettype, funcname, args) \
    static rettype dummy_##funcname args { \
        return (rettype)0; \
    } \

LIST_XLIB_FUNCTYPES(DEFINE_DUMMY_IMPL)
LIST_GLX_FUNCTYPES(DEFINE_DUMMY_IMPL)

class X11FunctionGetter {
    public:
        X11FunctionGetter() :
            mX11Lib(android::base::SharedLibrary::open("libX11")),
            mXextLib(android::base::SharedLibrary::open("libXext")) {

#define X11_ASSIGN_DUMMY_IMPL(funcname) mApi.funcname = dummy_##funcname;

                LIST_XLIB_FUNCS(X11_ASSIGN_DUMMY_IMPL)

            if (!mX11Lib) return;
            if (!mXextLib) return;

#define X11_GET_FUNC(funcname) \
            { \
                auto f = mX11Lib->findSymbol(#funcname); \
                if (f) mApi.funcname = (funcname##_t)f; \
            } \
           
                LIST_XLIB_FUNCS(X11_GET_FUNC);

            }

        X11Api* getApi() { return &mApi; }
    private:
        android::base::SharedLibrary* mX11Lib;
        android::base::SharedLibrary* mXextLib;

        X11Api mApi;
};

class GlxFunctionGetter {
    public:
        GlxFunctionGetter() :
            mGlxLib(android::base::SharedLibrary::open("libGL")) {

#define GLX_ASSIGN_DUMMY_IMPL(funcname) mApi.funcname = dummy_##funcname;

                LIST_GLX_FUNCS(GLX_ASSIGN_DUMMY_IMPL)

            if (!mGlxLib) return;

#define GLX_GET_FUNC(funcname) \
                { \
                auto f = mGlxLib->findSymbol(#funcname); \
                if (f) mApi.funcname = (funcname##_t)f; \
                } \
           
                LIST_GLX_FUNCS(GLX_GET_FUNC);

            }

        GlxApi* getApi() { return &mApi; }

    private:
        android::base::SharedLibrary* mGlxLib;

        GlxApi mApi;
};

AEMU_EXPORT struct X11Api* getX11Api() {
    static X11FunctionGetter* g = new X11FunctionGetter;
    return g->getApi();
}

AEMU_EXPORT struct GlxApi* getGlxApi() {
    static GlxFunctionGetter* g = new GlxFunctionGetter;
    return g->getApi();
}
