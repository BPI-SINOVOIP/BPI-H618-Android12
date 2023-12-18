toplevel_output_directories(paths = ["out"])

load("//build/bazel/rules:lunch.bzl", "lunch")
load("//build/bazel/rules:soong_injection.bzl", "soong_injection_repository")

lunch()

register_toolchains(
    "//prebuilts/clang/host/linux-x86:all"
)

soong_injection_repository(name="soong_injection")

local_repository(
    name = "rules_cc",
    path = "build/bazel/rules_cc",
)

local_repository(
    name = "bazel_skylib",
    path = "build/bazel/bazel_skylib",
)

local_repository(
    name = "rules_android",
    path = "external/bazelbuild-rules_android",
)

register_toolchains(
  # For Starlark Android rules
  "//prebuilts/sdk:android_default_toolchain",
  "//prebuilts/sdk:android_sdk_tools",

  # For native android_binary
  "//prebuilts/sdk:android_sdk_tools_for_native_android_binary",
)

bind(
  name = "databinding_annotation_processor",
  actual = "//prebuilts/sdk:compiler_annotation_processor",
)

bind(
  name = "android/dx_jar_import",
  actual = "//prebuilts/sdk:dx_jar_import",
)
