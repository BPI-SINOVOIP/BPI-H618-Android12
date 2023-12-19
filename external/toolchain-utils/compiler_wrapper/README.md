# Compiler wrapper

See the comments on the top of main.go.
Build is split into 2 steps via separate commands:
- bundle: copies the sources and the `build.py` file into
  a folder.
- build: builds the actual go binary, assuming it is executed
  from the folder created by `bundle.py`.

This allows to copy the sources to a Chrome OS / Android
package, including the build script, and then
build from there without a dependency on toolchain-utils
itself.

## Testing Inside the Chroot

To test updates to the wrapper locally:

Run `install_compiler_wrapper.sh` to install the new wrapper in the chroot:
```
(chroot) ~/trunk/src/third_party/toolchain-utils/compiler_wrapper/install_compiler_wrapper.sh
```

Then perform the tests, e.g. build with the new compiler.


## Updating the Wrapper for Chrome OS

To update the wrapper for everyone, the new wrapper configuration must be copied
into chromiumos-overlay, and new revisions of the gcc and llvm ebuilds must be
created.

Copy over sources and `build.py` to chromiumos-overlay:
```
(chroot) /mnt/host/source/src/third_party/chromiumos-overlay/sys-devel/llvm/files/update_compiler_wrapper.sh
```

Rename chromiumos-overlay/sys-devel/llvm/llvm-${VERSION}.ebuild to the next
revision number. For example, if the current version is
11.0_pre394483_p20200618-r2:
```
(chroot) cd ~/trunk/src/third_party/chromiumos-overlay
(chroot) git mv llvm-11.0_pre394483_p20200618-r2.ebuild llvm-11.0_pre394483_p20200618-r3.ebuild
```

Rename chromiumos-overlay/sys-devel/gcc/gcc-${VERSION}.ebuild to the next
revision number.  For example, if the current version is 4.9.2-r254:
```
(chroot) cd ~/trunk/src/third_party/chromiumos-overlay
(chroot) git mv sys-devel/gcc/gcc-4.9.2-r254.ebuild sys-devel/gcc/gcc-4.9.2-r255.ebuild
```

Edit the new ebuild file and change the number at the end to match the new revision:
```
-# If you need to force a cros_workon uprev, change this number (you can use next
-# uprev): 254
+# If you need to force a cros_workon uprev, change this number (you can use next
+# uprev): 255
```

Commit those changes together with the changes made by
`update_compiler_wrapper.sh`.

The changes can then be reviewed and submitted through the normal process.


## Paths

`build.py` is called by these ebuilds:

- third_party/chromiumos-overlay/sys-devel/llvm/llvm-*.ebuild
- third_party/chromiumos-overlay/sys-devel/gcc/gcc-*.ebuild

Generated wrappers are stored here:

- Sysroot wrapper with ccache:
  `/usr/x86_64-pc-linux-gnu/<arch>/gcc-bin/4.9.x/sysroot_wrapper.hardened.ccache`
- Sysroot wrapper without ccache:
  `/usr/x86_64-pc-linux-gnu/<arch>/gcc-bin/4.9.x/sysroot_wrapper.hardened.noccache`
- Clang host wrapper:
  `/usr/bin/clang_host_wrapper`
- Gcc host wrapper:
  `/usr/x86_64-pc-linux-gnu/gcc-bin/4.9.x/host_wrapper`
