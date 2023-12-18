# This test requires bp2build to run and the generated BUILD files in the source tree.
sh_test(
    name = "verify_bionic_outputs",
    srcs = ["verify_bionic_outputs.sh"],
    data = [
        "//bionic/linker:ld-android",
        "//bionic/libdl:libdl_android",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
