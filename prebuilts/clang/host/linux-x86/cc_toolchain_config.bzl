load("@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl", "feature", "flag_group", "flag_set", "tool_path", "with_feature_set")
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

load("@soong_injection//cc_toolchain:constants.bzl", "constants")

# Clang-specific configuration.
_ClangVersionInfo = provider(fields = ["directory", "includes"])

def _clang_version_impl(ctx):
    directory = ctx.file.directory
    provider = _ClangVersionInfo(
        directory = directory,
        includes = [directory.short_path + "/" + d for d in ctx.attr.includes],
    )
    return [provider]

clang_version = rule(
    implementation = _clang_version_impl,
    attrs = {
        "directory": attr.label(allow_single_file = True, mandatory = True),
        "includes": attr.string_list(default = []),
    },
)

# These defines should only apply to targets which are not under
# @external/. This can be controlled by adding "-non_external_compiler_flags"
# to the features list for external/ packages.
# This corresponds to special-casing in Soong (see "external/" in build/soong/cc/compiler.go).
NON_EXTERNAL_DEFINES = [
    "-DANDROID_STRICT",
]

COMPILER_FLAGS = [
    "-fPIC",
]
ASM_COMPILER_FLAGS = [
    "-D__ASSEMBLY__",
]
# CStdVersion in cc/config/global.go
C_COMPILER_FLAGS = [
    "-std=gnu99",
]
# CppStdVersion in cc/config/global.go
CC_COMPILER_STANDARD_STD_FLAGS = [
    "-std=gnu++17",
]

# Should be toggled instead of CC_COMPILER_STANDARD_STD_FLAGS if
# the soong module has "cpp_std: 'experimental'". In bazel, tied
# to the feature "cpp_std_experimental".
CC_COMPILER_EXPERIMENTAL_STD_FLAGS = [
    "-std=gnu++2a",
]

# These are the linker flags for OSes that use Bionic: LinuxBionic, Android
BIONIC_LINKER_FLAGS = [
    "-nostdlib",
    "-Wl,--no-undefined",
    "-Wl,--hash-style=gnu",
    "-Wl,--gc-sections",
]
STATIC_LINKER_FLAGS = [
    "-static",
]
DYNAMIC_LINKER_FLAGS = [
    "-shared",
]

def _tool_paths(clang_version_info):
    return [
        tool_path(
            name = "gcc",
            path = clang_version_info.directory.basename + "/bin/clang",
        ),
        tool_path(
            name = "ld",
            path = clang_version_info.directory.basename + "/bin/ld.lld",
        ),
        tool_path(
            name = "ar",
            path = clang_version_info.directory.basename + "/bin/llvm-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/bin/false",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = clang_version_info.directory.basename + "/bin/llvm-nm",
        ),
        tool_path(
            name = "objdump",
            path = clang_version_info.directory.basename + "/bin/llvm-objdump",
        ),
        # Soong has a wrapper around strip.
        # https://cs.android.com/android/platform/superproject/+/master:build/soong/cc/strip.go;l=62;drc=master
        # https://cs.android.com/android/platform/superproject/+/master:build/soong/cc/builder.go;l=991-1025;drc=master
        tool_path(
            name = "strip",
            path = clang_version_info.directory.basename + "/bin/llvm-strip",
        ),
    ]

def _compiler_flag_features(flags = [], os_is_device = False):

    # Combine the toolchain's provided flags with the default ones.
    flags = flags + COMPILER_FLAGS + constants.CommonClangGlobalCflags

    if os_is_device:
        flags += constants.DeviceClangGlobalCflags
    else:
        flags += constants.HostClangGlobalCflags

    # Default assembler flags.
    asm_only_flags = ASM_COMPILER_FLAGS

    # Default C++ compile action only flags (No C)
    cpp_only_flags = []
    cpp_only_flags += constants.CommonClangGlobalCppflags
    if os_is_device:
        cpp_only_flags += constants.DeviceGlobalCppflags
    else:
        cpp_only_flags += constants.HostGlobalCppflags

    # Default C compile action only flags (No C++)
    c_only_flags = C_COMPILER_FLAGS + constants.CommonGlobalConlyflags

    # Flags that only apply in the external/ directory.
    non_external_flags = NON_EXTERNAL_DEFINES

    features = []

    features.append(feature(
        name = "non_external_compiler_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = non_external_flags,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "common_compiler_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = flags,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "asm_compiler_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = asm_only_flags,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "cpp_compiler_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = cpp_only_flags,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "c_compiler_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = c_only_flags,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "cpp_std_experimental",
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_compile,
                ],
                flag_groups = [
                    flag_group(
                        flags = CC_COMPILER_EXPERIMENTAL_STD_FLAGS,
                    ),
                ],
            ),
        ],
    ))
    features.append(feature(
        name = "cpp_std_standard",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_compile,
                ],
                with_features = [
                    with_feature_set(not_features = ["cpp_std_experimental"]),
                ],
                flag_groups = [
                    flag_group(
                        flags = CC_COMPILER_STANDARD_STD_FLAGS,
                    ),
                ],
            ),
        ],
    ))

    # The user_compile_flags feature is used by Bazel to add --copt, --conlyopt,
    # and --cxxopt values. Any features added above this call will thus appear
    # earlier in the commandline than the user opts (so users could override
    # flags set by earlier features). Anything after the user options are
    # effectively non-overridable by users.
    features.append(feature(
        name = "user_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        expand_if_available = "user_compile_flags",
                        flags = ["%{user_compile_flags}"],
                        iterate_over = "user_compile_flags",
                    ),
                ],
            ),
        ],
    ))

    # These cannot be overriden by the user.
    features.append(feature(
        name = "no_override_clang_global_copts",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                ],
                flag_groups = [
                    flag_group(
                        flags = constants.NoOverrideClangGlobalCflags,
                    ),
                ],
            ),
        ],
    ))


    return features

def _rpath_features():
    runtime_library_search_directories_feature = feature(
        name = "runtime_library_search_directories",
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_executable,
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ACTION_NAMES.lto_index_for_executable,
                    ACTION_NAMES.lto_index_for_dynamic_library,
                    ACTION_NAMES.lto_index_for_nodeps_dynamic_library,
                ],
                flag_groups = [
                    flag_group(
                        iterate_over = "runtime_library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$EXEC_ORIGIN/%{runtime_library_search_directories}",
                                ],
                                expand_if_true = "is_cc_test",
                            ),
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$ORIGIN/%{runtime_library_search_directories}",
                                ],
                                expand_if_false = "is_cc_test",
                            ),
                        ],
                        expand_if_available =
                            "runtime_library_search_directories",
                    ),
                ],
                with_features = [
                    with_feature_set(features = ["static_link_cpp_runtimes"]),
                ],
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_executable,
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ACTION_NAMES.lto_index_for_executable,
                    ACTION_NAMES.lto_index_for_dynamic_library,
                    ACTION_NAMES.lto_index_for_nodeps_dynamic_library,
                ],
                flag_groups = [
                    flag_group(
                        iterate_over = "runtime_library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$ORIGIN/%{runtime_library_search_directories}",
                                ],
                            ),
                        ],
                        expand_if_available =
                            "runtime_library_search_directories",
                    ),
                ],
                with_features = [
                    with_feature_set(
                        not_features = ["static_link_cpp_runtimes", "disable_rpath"],
                    ),
                ],
            ),
        ],
    )
    disable_rpath_feature = feature(
        name = "disable_rpath",
        enabled = False,
    )
    return [runtime_library_search_directories_feature, disable_rpath_feature]

def _linker_flag_feature(name, flags = [], additional_static_flags = [], additional_dynamic_flags = []):
    if not flags:
        return None
    return feature(
        name = name,
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_executable,
                ],
                flag_groups = [
                    flag_group(
                        flags = flags + additional_static_flags,
                    ),
                ],
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_dynamic_library,
                ],
                flag_groups = [
                    flag_group(
                        flags = flags + additional_dynamic_flags,
                    ),
                ],
            ),
        ],
    )

def _toolchain_include_feature(system_includes = []):
    flags = []
    for include in system_includes:
        flags.append("-isystem")
        flags.append(include)
    if not flags:
        return None
    return feature(
        name = "toolchain_include_directories",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = [
                    flag_group(
                        flags = flags,
                    ),
                ],
            ),
        ],
    )

def is_target_os_device(ctx):
    if "_host" in ctx.attr.toolchain_identifier:
        return False
    else:
        return True

def _cc_toolchain_config_impl(ctx):
    clang_version_info = ctx.attr.clang_version[_ClangVersionInfo]
    os_is_device = is_target_os_device(ctx)

    builtin_include_dirs = []

    # This is so that Bazel doesn't validate .d files against the set of headers
    # declared in BUILD files (Blueprint files don't contain that data)
    builtin_include_dirs.extend(["/"])

    builtin_include_dirs.extend(clang_version_info.includes)

    # Compiler action features
    compiler_flag_features = _compiler_flag_features(ctx.attr.target_flags, os_is_device)

    # Linker action features
    linker_target_flag_feature = _linker_flag_feature(
        "linker_target_flags",
        flags = ctx.attr.target_flags,
    )

    linker_flags = []
    linker_flags += ctx.attr.linker_flags
    if os_is_device:
        linker_flags += constants.DeviceGlobalLldflags
        linker_flags += BIONIC_LINKER_FLAGS
    else:
        linker_flags += constants.HostGlobalLldflags
    linker_flag_feature = _linker_flag_feature(
        "linker_flags",
        flags = linker_flags,
        additional_static_flags = STATIC_LINKER_FLAGS,
        additional_dynamic_flags = DYNAMIC_LINKER_FLAGS,
    )

    # System include directories features
    toolchain_include_directories_feature = _toolchain_include_feature(
        system_includes = builtin_include_dirs,
    )

    # Aggregate all features
    features = compiler_flag_features + \
        _rpath_features() + \
        [
            linker_target_flag_feature,
            linker_flag_feature,
            toolchain_include_directories_feature
        ]
    features = [feature for feature in features if feature != None]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = ctx.attr.toolchain_identifier,
        host_system_name = "i686-unknown-linux-gnu",
        # TODO: replace the following placeholders with the real target values,
        # preferably declared at the toolchain.
        target_system_name = "x86_64-unknown-unknown",
        target_cpu = "x86_64",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = _tool_paths(clang_version_info),
        features = features,
        cxx_builtin_include_directories = builtin_include_dirs,
    )

_cc_toolchain_config = rule(
    implementation = _cc_toolchain_config_impl,
    attrs = {
        "toolchain_identifier": attr.string(mandatory = True),
        "clang_version": attr.label(mandatory = True, providers = [_ClangVersionInfo]),
        "target_flags": attr.string_list(default = []),
        "linker_flags": attr.string_list(default = []),
        '_android_os_constraint': attr.label(default = "//build/bazel/platforms/os:android"),
        '_linux_os_constraint': attr.label(default = "//build/bazel/platforms/os:linux"),
    },
    provides = [CcToolchainConfigInfo],
)

# Macro to set up both the toolchain and the config.
def android_cc_toolchain(
        name,
        clang_version = None,
        # This should come from the clang_version provider.
        # Instead, it's hard-coded because this is a macro, not a rule.
        clang_version_directory = None,
        target_flags = [],
        linker_flags = [],
        toolchain_identifier = None):
    # Write the toolchain config.
    _cc_toolchain_config(
        name = "%s_config" % name,
        clang_version = clang_version,
        target_flags = target_flags,
        linker_flags = linker_flags,
        toolchain_identifier = toolchain_identifier,
    )

    # Create the filegroups needed for sandboxing toolchain inputs to C++ actions.
    native.filegroup(
        name = "%s_compiler_clang_includes" % name,
        srcs =
            native.glob([clang_version_directory + "/lib64/clang/*/include/**"]),
    )

    native.filegroup(
        name = "%s_compiler_binaries" % name,
        srcs = native.glob([
            clang_version_directory + "/bin/clang*",
        ]),
    )

    native.filegroup(
        name = "%s_linker_binaries" % name,
        srcs = native.glob([
            # Linking shared libraries uses clang.
            clang_version_directory + "/bin/clang*",
        ]) + [
            clang_version_directory + "/bin/lld",
            clang_version_directory + "/bin/ld.lld",
        ],
    )

    native.filegroup(
        name = "%s_ar_files" % name,
        srcs = [clang_version_directory + "/bin/llvm-ar"],
    )

    native.filegroup(
        name = "%s_compiler_files" % name,
        srcs = [
            "%s_compiler_binaries" % name,
            "%s_compiler_clang_includes" % name,
        ],
    )
    native.filegroup(
        name = "%s_linker_files" % name,
        srcs = [
            "%s_linker_binaries" % name,
        ],
    )
    native.filegroup(
        name = "%s_all_files" % name,
        srcs = [
            "%s_compiler_files" % name,
            "%s_linker_files" % name,
            "%s_ar_files" % name,
        ],
    )

    # Create the actual cc_toolchain.
    # The dependency on //:empty is intentional; it's necessary so that Bazel
    # can parse .d files correctly (see the comment in $TOP/BUILD)
    native.cc_toolchain(
        name = name,
        all_files = "%s_all_files" % name,
        as_files = "//:empty",  # Note the "//" prefix, see comment above
        ar_files = "%s_ar_files" % name,
        compiler_files = "%s_compiler_files" % name,
        dwp_files = ":empty",
        linker_files = "%s_linker_files" % name,
        objcopy_files = ":empty",
        strip_files = ":empty",
        supports_param_files = 0,
        toolchain_config = ":%s_config" % name,
        toolchain_identifier = toolchain_identifier,
    )
