"""Hard-coded rules for product variables until product config files are
converted and available."""

def _product_variables_providing_rule_impl(ctx):
    return [
        platform_common.TemplateVariableInfo({
            "Platform_version_name": ctx.attr.platform_version_name,
            "Platform_sdk_version": str(ctx.attr.platform_sdk_version),
            "Platform_sdk_codename": ctx.attr.platform_sdk_codename,
            "Platform_sdk_final": str(1 if ctx.attr.platform_sdk_final else 0),
            "Platform_version_active_codenames": ",".join(ctx.attr.platform_version_active_codenames),
            "Platform_vndk_version": ctx.attr.platform_vndk_version,
        }),
    ]

# Values taken from https://cs.android.com/android/platform/superproject/+/master:build/soong/android/variable.go;l=412;drc=7b85eeb41ef3e6d2cf44558d3f54f9ed1b247036
product_variables_providing_rule = rule(
    implementation = _product_variables_providing_rule_impl,
    attrs = {
        "platform_version_name": attr.string(default="S"),
        "platform_sdk_version": attr.int(default=30),
        "platform_sdk_codename": attr.string(default="S"),
        "platform_sdk_final": attr.bool(default=False),
        "platform_version_active_codenames": attr.string_list(default=["S"]),
        "platform_vndk_version": attr.string(default="S"),
    },
)


