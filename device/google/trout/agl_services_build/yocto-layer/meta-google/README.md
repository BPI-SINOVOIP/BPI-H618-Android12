Trout AGL Service Yocto Build Layer
===================================

This is a yocto meta-layer defining the build recipes for several Trout AGL services (e.g., vehicle HAL server). This needs to be tested with the AGL BSP source tree.

### Prerequisite

Acquire a Yocto BSP as needed for your use cases.

### Add layers

First copy this directory to `ROOT_OF_AGL_BSP/apps/apps_proc`

```
bitbake-layers add-layer meta-clang meta-google
```

### Build
`bitbake google-trout-agl-services`
