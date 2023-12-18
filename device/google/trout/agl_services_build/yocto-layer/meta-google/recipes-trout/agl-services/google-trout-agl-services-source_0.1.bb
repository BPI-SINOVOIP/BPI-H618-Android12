SUMMARY = "Source code of Trout AGL Services"

deltask do_configure
deltask do_compile
deltask do_install
deltask do_populate_lic
deltask do_populate_sysroot

inherit nopackages

WORKDIR = "${TMPDIR}/work-shared/google-trout-agl-services-source/${PV}-${PR}"

require sources.inc
