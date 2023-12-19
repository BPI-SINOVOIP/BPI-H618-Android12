#!/usr/bin/env python

import json
import sys

# Set this to True to generate a default entry with all the flags and defines
# common to all the modules that needs to be curated by hand after it's
# generated. When set to False it prints the last curated version of the
# default, which could be incomplete.
GENERATE_FULL_DEFAULT = False
PRINT_ORIGINALS = False
GENERATE_ALL_FLAGS = False

if len(sys.argv) != 2:
    print('wrong number of arguments')
    exit()

def FormatList(l):
    return json.dumps(l)

def FilterHeaders(l):
    return [x for x in l if not x.endswith('.h')]

def PrintOriginCommentedOut(target):
    if PRINT_ORIGINALS:
        print('/* From target:')
        print(json.dumps(target, sort_keys = True, indent = 4))
        print('*/')

def MakeRelatives(l):
    return [x.split('//').pop() for x in l]

def FormatName(name):
    return 'webrtc_' + name.split('/').pop().replace(':', '__')# .replace('/', '_').replace(':', '_')

def FilterFlags(flags, to_skip = set([])):
    if GENERATE_ALL_FLAGS:
        skip = set([
            '-L',
            '-isystem',
            '-Xclang',
            '-B',
            '--sysroot',
            '-fcrash-diagnostics-dir',
            '.',
            '-fdebug-compilation-dir',
            '-instcombine-lower-dbg-declare=0',
            '-Wno-non-c-typedef-for-linkage',
            '-Werror',
            '-fcomplete-member-pointers',
            '-m64',
            '-march=x86-64'
            ]).union(to_skip)
        return [x for x in flags if not any([x.startswith(y) for y in skip])]
    else:
        return [x for x in flags if x == '-msse2']

def GenerateDefault(targets):
    in_default = {
            'cflags' : [],
            'cflags_c' : [],
            'cflags_cc' : [],
            'ldflags' : [],
            'asmflags' : [],
            'defines' : []
    }
    first = True
    for target in targets:
        typ = target['type']
        if typ == 'static_library':
            if first:
                first = False
                # Add all the flags to the default, we'll remove some later
                for flag_type in in_default.keys():
                    in_default[flag_type] = []
                    for flag in target[flag_type]:
                        in_default[flag_type].append(flag)
            else:
                for flag_type in in_default.keys():
                    flags_to_remove = []
                    for flag in in_default[flag_type]:
                        if flag not in target[flag_type]:
                            flags_to_remove.append[flag_type]
                    for flag in flags_to_remove:
                        in_default[flag_type].remove(flag)
    defines = in_default['defines']
    in_default.pop('defines')
    in_default['cflags'].extend(['-D{0}'.format(x) for x in defines])
    if GENERATE_FULL_DEFAULT:
        print('cc_defaults {')
        print('    name: "webrtc_defaults",')
        print('    local_include_dirs: ["."],')
        for typ in in_default.keys():
            print('    {0}: ['.format(typ.replace('asmflags', 'asflags')
                .replace('cflags_cc', 'cppflags')
                .replace('cflags_c', 'conlyflags')))
            for flag in FilterFlags(in_default[typ]):
                print('        "{0}",'.format(flag.replace('"', '\\"')))
            print('    ],')
        print('}')
    else:
        print('cc_defaults {')
        print('    name: "webrtc_defaults",')
        print('    local_include_dirs: [')
        print('      ".",')
        print('    ],')
        print('    cflags: [')
        print('        "-Wno-unused-parameter",')
        print('        "-Wno-missing-field-initializers",')
        print('        "-DUSE_UDEV",')
        print('        "-DUSE_AURA=1",')
        print('        "-DUSE_GLIB=1",')
        print('        "-DUSE_NSS_CERTS=1",')
        print('        "-DUSE_X11=1",')
        print('        "-D_FILE_OFFSET_BITS=64",')
        print('        "-D_LARGEFILE_SOURCE",')
        print('        "-D_LARGEFILE64_SOURCE",')
        print('        "-D_GNU_SOURCE",')
        print('        "-DWEBRTC_ENABLE_PROTOBUF=0",')
        print('        "-DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE",')
        print('        "-DRTC_ENABLE_VP9",')
        print('        "-DHAVE_SCTP",')
        print('        "-DWEBRTC_LIBRARY_IMPL",')
        print('        "-DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1",')
        print('        "-DWEBRTC_POSIX",')
        print('        "-DWEBRTC_LINUX",')
        print('    ],')
        print('    header_libs: [')
        print('      "libwebrtc_absl_headers",')
        print('    ],')
        print('    static_libs: [')
        print('        "libaom",')
        print('        "libevent",')
        print('        "libopus",')
        print('        "libsrtp2",')
        print('        "libvpx",')
        print('        "libyuv",')
        print('        "libpffft",')
        print('        "rnnoise_rnn_vad",')
        print('        "usrsctplib",')
        print('    ],')
        print('    shared_libs: [')
        print('        "libcrypto",')
        print('        "libssl",')
        print('    ],')
        print('    host_supported: true,')
        print('    device_supported: false,')
        print('    arch: {')
        print('        arm: {')
        print('            enabled: false,')
        print('        },')
        print('    },')
        print('    target: {')
        print('        darwin: {')
        print('            enabled: false,')
        print('        },')
        print('    },')
        print('}')
    in_default['cflags'].extend([
        "-Wno-unused-parameter",
        "-Wno-missing-field-initializers",
        "-DUSE_UDEV",
        "-DUSE_AURA=1",
        "-DUSE_GLIB=1",
        "-DUSE_NSS_CERTS=1",
        "-DUSE_X11=1",
        "-D_FILE_OFFSET_BITS=64",
        "-D_LARGEFILE_SOURCE",
        "-D_LARGEFILE64_SOURCE",
        "-D_GNU_SOURCE",
        "-DWEBRTC_ENABLE_PROTOBUF=0",
        "-DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE",
        "-DRTC_ENABLE_VP9",
        "-DHAVE_SCTP",
        "-DWEBRTC_LIBRARY_IMPL",
        "-DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1",
        "-DWEBRTC_POSIX",
        "-DWEBRTC_LINUX"
        ])
    return in_default

def GenerateGroup(target):
    PrintOriginCommentedOut(target)

def GenerateSourceSet(target):
    PrintOriginCommentedOut(target)
    if target.has_key('sources'):
        name = FormatName(target['name'])
        print('filegroup {')
        print('    name: "{0}",'.format(name))
        print('    srcs: {0},'.format(FormatList(MakeRelatives(FilterHeaders(target['sources'])))))
        print('}')
        return name
    return ""

def GenerateStaticLib(target, targets, flags_in_default):
    PrintOriginCommentedOut(target)
    name = FormatName(target['name'])
    print('cc_library_static {')
    print('    name: "{0}",'.format(name))
    print('    defaults: ["webrtc_defaults"],')
    sources = []
    sources.extend(MakeRelatives(FilterHeaders(target['sources'])))
    for trg in targets:
        if trg['type'] == 'source_set' and trg['name'] in target['deps']:
            sources.append(':' + FormatName(trg['name']))
    print('    srcs: {0},'.format(FormatList(sources)))
    print('    host_supported: true,')
    if target.has_key('asmflags'):
        asmflags = FilterFlags(target['asmflags'], set(flags_in_default['asmflags']))
        if len(asmflags) > 0:
            print('    asflags: {0},'.format(FormatList(asmflags)))
    cflags = []
    if target.has_key('cflags'):
        cflags.extend(target['cflags'])
    cflags = FilterFlags(cflags, set(flags_in_default['cflags']))
    if target.has_key('defines'):
        cflags.extend(['-D' + x for x in target['defines']])
    cflags = [x for x in cflags if x not in flags_in_default['cflags']]
    if len(cflags) > 0:
        print('    cflags: {0},'.format(FormatList(cflags)))
    if target.has_key('cflags_c'):
        cflags_c = FilterFlags(target['cflags_c'], set(flags_in_default['cflags_c']))
        if len(cflags_c) > 0:
            print('    conlyflags: {0},'.format(FormatList(cflags_c)))
    if target.has_key('cflags_cc'):
        cflags_cc = FilterFlags(target['cflags_cc'], set(flags_in_default['cflags_cc']))
        if len(cflags_cc) > 0:
            print('    cppflags: {0},'.format(FormatList(cflags_cc)))
    if target.has_key('ldflags'):
        ldflags = FilterFlags(target['ldflags'], set(flags_in_default['ldflags']))
        if len(ldflags) > 0:
            print('    ldflags: {0},'.format(FormatList(ldflags)))
    static_libs = []
    for trg in targets:
        if trg['type'] == 'static_library' and target['deps'].count(trg['name']) > 0:
            static_libs.append(FormatName(trg['name']))
    if len(static_libs) > 0:
        print('    static_libs: {0},'.format(FormatList(static_libs)))
    for dep in target['deps']:
        if FormatName(dep) not in static_libs:
            #print('    // omitted dep: {0}'.format(dep))
            pass
    print('}')
    return name

json_file = open(sys.argv[1], "r")
targets = json.load(json_file)

flags_in_default = GenerateDefault(targets)
print("\n\n")

static_libs = []
file_groups = []

for target in targets:
    typ = target['type']
    if typ == 'static_library':
        lib_name = GenerateStaticLib(target, targets, flags_in_default)
        static_libs.append(lib_name)
    elif typ == 'source_set':
        group_name = GenerateSourceSet(target)
        if len(group_name) > 0:
          file_groups.append(group_name)
    elif typ == 'group':
        GenerateGroup(target)
    else:
        print('Unknown type: {0}'.format(typ))
    print("\n\n")

print('cc_library_static {')
print('    name: "libwebrtc",')
print('    defaults: ["webrtc_defaults"],')
print('    export_include_dirs: ["."],')
print('    whole_static_libs: {0},'.format(FormatList(static_libs + ['libpffft', 'rnnoise_rnn_vad', 'usrsctplib'])))
print('    srcs: {0},').format(FormatList([':{0}'.format(x) for x in file_groups]))
print('}')
