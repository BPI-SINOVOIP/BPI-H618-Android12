# When inherited by an image recipe, this class provide simple way to
# put configuration files into rootfs.

ROOTFS_POSTPROCESS_COMMAND_append = "install_google_overlay ; "

python install_google_overlay() {
    """Fetch and install overlays into rootfs

       Fetch overlays from GOOGLE_OVERLAY_SRC_URI and copy the files
       inside the folders listed in GOOGLE_OVERLAY_ROOT_DIRS to rootfs.

       GOOGLE_OVERLAY_ROOT_DIRS contains only names of folders, no path.

       GOOGLE_OVERLAY_SRC_URI must be used to get dirs in GOOGLE_OVERLAY_ROOT_DIRS
       imported to the workdir, it's the same as SRC_URI, so it's
       possible to use distant repository, tarballs, etc.
    """
    workdir = d.getVar('WORKDIR', True)
    rootfs = d.getVar('IMAGE_ROOTFS', True)
    srcs = (d.getVar("GOOGLE_OVERLAY_SRC_URI", True) or "").split()
    overlay_root_dirs = (d.getVar('GOOGLE_OVERLAY_ROOT_DIRS', True) or '').split()

    fetcher = bb.fetch2.Fetch(srcs, d)
    fetcher.download()
    fetcher.unpack(workdir)

    for i in range(len(overlay_root_dirs)):
        fullpath = os.path.join(workdir, overlay_root_dirs[i])

        if not os.path.isdir(fullpath):
            bb.fatal(fullpath +
                     " : directory not found, please check your"\
                     " GOOGLE_OVERLAY_ROOT_DIRS and GOOGLE_OVERLAY_SRC_URI variables.")
            break
        os.system("cp -dr " + os.path.join(fullpath, "*") + " " + rootfs)
}

do_rootfs[vardeps] += "GOOGLE_OVERLAY_ROOT_DIRS GOOGLE_OVERLAY_SRC_URI"

# Copied from https://patchwork.openembedded.org/patch/138100/
