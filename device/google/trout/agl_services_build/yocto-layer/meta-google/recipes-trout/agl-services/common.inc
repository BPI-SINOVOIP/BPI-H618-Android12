require sources.inc

do_fetch() {
	:
}
do_fetch[noexec] = "1"
deltask do_unpack
deltask do_patch

S = "${TMPDIR}/work-shared/google-trout-agl-services-source/${PV}-${PR}/${FETCH_CODE_PREFIX}"

do_configure[depends] += "google-trout-agl-services-source:do_patch"
do_populate_lic[depends] += "google-trout-agl-services-source:do_unpack"

def trout_compile_targets(d):
    install_entries = d.getVar('TROUT_target_install').split()
    compile_targets = map(lambda x: x.split(':')[0], install_entries)
    return ' '.join(compile_targets)

def trout_do_install(d):
    install_entries = d.getVar('TROUT_target_install').split()
    os.chdir(d.getVar('B') + '/bin')
    install_dir = '{}{}'.format(d.getVar('D'), d.getVar('bindir'))
    os.system('install -d {}'.format(install_dir))
    for entry in install_entries:
        if ':' in entry:
            src, dst = entry.split(':')
            force_create_symlink(src, dst)
        else:
            dst = entry
        os.system('install -m 0755 {} {}'.format(dst, install_dir))

OECMAKE_TARGET_COMPILE = "${@trout_compile_targets(d)}"

inherit pkgconfig cmake

python do_install() {
    trout_do_install(d)
}
