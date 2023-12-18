def _impl(rctx):
    rctx.file("WORKSPACE", "")
    build_dir = str(rctx.path(Label("//:BUILD")).dirname.dirname)
    soong_injection_dir = build_dir + "/soong_injection"
    rctx.symlink(soong_injection_dir + "/mixed_builds", "mixed_builds")
    rctx.symlink(soong_injection_dir + "/cc_toolchain", "cc_toolchain")

soong_injection_repository = repository_rule(
    implementation = _impl,
)
