load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cpp_toolchain")
load(":cc_constants.bzl", "constants")

# "cc_object" module copts, taken from build/soong/cc/object.go
_CC_OBJECT_COPTS = ["-fno-addrsig"]

# partialLd module link opts, taken from build/soong/cc/builder.go
# https://cs.android.com/android/platform/superproject/+/master:build/soong/cc/builder.go;l=87;drc=f2be52c4dcc2e3d743318e106633e61de0ad2afd
_CC_OBJECT_LINKOPTS = [
    "-fuse-ld=lld",
    "-nostdlib",
    "-no-pie",
    "-Wl,-r",
]


CcObjectInfo = provider(fields = [
    # The merged compilation outputs for this cc_object and its transitive
    # dependencies.
    "objects",
])

def split_srcs_hdrs(files):
    headers = []
    non_headers = []
    for f in files:
        if f.extension in constants.hdr_exts:
            headers += [f]
        else:
            non_headers += [f]
    return non_headers, headers


def _cc_object_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features + ["linker_flags"],
    )

    compilation_contexts = []
    deps_objects = []
    for obj in ctx.attr.deps:
        compilation_contexts.append(obj[CcInfo].compilation_context)
        deps_objects.append(obj[CcObjectInfo].objects)

    product_variables = ctx.attr._android_product_variables[platform_common.TemplateVariableInfo]
    asflags = [flag.format(**product_variables.variables) for flag in ctx.attr.asflags]

    srcs, private_hdrs = split_srcs_hdrs(ctx.files.srcs)

    (compilation_context, compilation_outputs) = cc_common.compile(
        name = ctx.label.name,
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        srcs = srcs,
        includes = ctx.attr.includes,
        public_hdrs = ctx.files.hdrs,
        private_hdrs = private_hdrs,
        user_compile_flags = ctx.attr.copts + asflags,
        compilation_contexts = compilation_contexts,
    )

    objects_to_link = cc_common.merge_compilation_outputs(compilation_outputs=deps_objects + [compilation_outputs])

    # partially link if there are multiple object files
    if len(objects_to_link.objects) + len(objects_to_link.pic_objects) > 1:
        linking_output = cc_common.link(
            name = ctx.label.name + ".o",
            actions = ctx.actions,
            feature_configuration = feature_configuration,
            cc_toolchain = cc_toolchain,
            user_link_flags = _CC_OBJECT_LINKOPTS,
            compilation_outputs = objects_to_link,
        )
        files = depset([linking_output.executable])
    else:
        files = depset(objects_to_link.objects + objects_to_link.pic_objects)

    return [
        DefaultInfo(files = files),
        CcInfo(compilation_context = compilation_context),
        CcObjectInfo(objects = objects_to_link),
    ]

_cc_object = rule(
    implementation = _cc_object_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = constants.all_dot_exts),
        "hdrs": attr.label_list(allow_files = constants.hdr_dot_exts),
        "includes": attr.string_list(),
        "copts": attr.string_list(),
        "asflags": attr.string_list(),
        "deps": attr.label_list(providers=[CcInfo, CcObjectInfo]),
        "_cc_toolchain": attr.label(
            default = Label("@local_config_cc//:toolchain"),
            providers = [cc_common.CcToolchainInfo],
        ),
        "_android_product_variables": attr.label(
            default = Label("//build/bazel/product_variables:android_product_variables"),
            providers = [platform_common.TemplateVariableInfo],
        ),
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
)

def cc_object(
        name,
        copts = [],
        hdrs = [],
        asflags = [],
        srcs = [],
        deps = [],
        native_bridge_supported = False, # TODO: not supported yet.
        **kwargs):
    "Build macro to correspond with the cc_object Soong module."

    _cc_object(
        name = name,
        hdrs = hdrs,
        asflags = asflags,
        copts = _CC_OBJECT_COPTS + copts,
        srcs = srcs,
        deps = deps,
        **kwargs
    )
