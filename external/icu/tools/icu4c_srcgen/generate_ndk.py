#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#            http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Generate ICU stable C API wrapper source.


This script parses all the header files specified by the ICU module names. For
each function in the allowlist, it generates the NDK headers, and shim functions
to shim.cpp, which in turn calls the real implementation at runtime.
The tool relies on libclang to parse header files.

Reference to ICU4C stable C APIs:
http://icu-project.org/apiref/icu4c/files.html
"""
from __future__ import absolute_import
from __future__ import print_function

import logging
import os
import re
import shutil
import subprocess

from genutil import (
    android_path,
    generate_shim,
    generate_symbol_txt,
    get_allowlisted_apis,
    AllowlistedDeclarationFilter,
    DeclaredFunctionsParser,
    StableDeclarationFilter,
)

# No suffix for ndk shim
SYMBOL_SUFFIX = ''

SECRET_PROCESSING_TOKEN = "@@@SECRET@@@"

DOC_BLOCK_COMMENT = r"\/\*\*(?:\*(?!\/)|[^*])*\*\/[ ]*\n"
TILL_CLOSE_PARENTHESIS = r"[^)^;]*\)"
STABLE_MACRO = r"(?:U_STABLE|U_CAPI)"
STABLE_FUNCTION_DECLARATION = r"^(" + DOC_BLOCK_COMMENT + STABLE_MACRO \
                              + TILL_CLOSE_PARENTHESIS + ");$"
NONSTABLE_FUNCTION_DECLARATION = r"^(" + DOC_BLOCK_COMMENT + r"(U_INTERNAL|U_DEPRECATED|U_DRAFT)" \
                                 + TILL_CLOSE_PARENTHESIS + ");$"

REGEX_STABLE_FUNCTION_DECLARATION = re.compile(STABLE_FUNCTION_DECLARATION, re.MULTILINE)
REGEX_NONSTABLE_FUNCTION_DECLARATION = re.compile(NONSTABLE_FUNCTION_DECLARATION, re.MULTILINE)

def get_allowlisted_regex_string(decl_names):
    """Return a regex in string to capture the C function declarations in the decl_names list"""
    tag = "|".join(decl_names)
    return r"(" + DOC_BLOCK_COMMENT + STABLE_MACRO + r"[^(]*(?=" + tag + r")(" + tag + ")" \
           + r"\("+ TILL_CLOSE_PARENTHESIS +");$"

def get_replacement_adding_api_level_macro(api_level):
    """Return the replacement string adding the NDK C macro
    guarding C function declaration by the api_level"""
    return r"\1 __INTRODUCED_IN({0});\n\n".format(api_level)

def modify_func_declarations(src_path, dst_path, decl_names):
    """Process the source file,
    remove the C function declarations not in the decl_names,
    add guard the functions listed in decl_names by the API level,
    and output to the dst_path """
    allowlist_regex_string = get_allowlisted_regex_string(decl_names)
    allowlist_decl_regex = re.compile('^' + allowlist_regex_string, re.MULTILINE)
    secret_allowlist_decl_regex = re.compile('^' + SECRET_PROCESSING_TOKEN
                                             + allowlist_regex_string, re.MULTILINE)
    with open(src_path, "r") as file:
        src = file.read()

    # Remove all non-stable function declarations
    modified = REGEX_NONSTABLE_FUNCTION_DECLARATION.sub('', src)

    # Insert intermediate token to all functions in the allowlist
    if decl_names:
        modified = allowlist_decl_regex.sub(SECRET_PROCESSING_TOKEN + r"\1;", modified)
    # Remove all other stable declarations not in the allowlist
    modified = REGEX_STABLE_FUNCTION_DECLARATION.sub('', modified)
    # Insert C macro and annotation to indicate the API level to each functions in the allowlist
    modified = secret_allowlist_decl_regex.sub(
        get_replacement_adding_api_level_macro(31), modified)

    with open(dst_path, "w") as out:
        out.write(modified)
def remove_ignored_includes(file_path, include_list):
    """
    Remove the included header, i.e. #include lines, listed in include_list from the file_path
    header.
    """

    # Do nothing if the list is empty
    if not include_list:
        return

    tag = "|".join(include_list)

    with open(file_path, "r") as file:
        content = file.read()

    regex = re.compile(r"^#include \"unicode\/(" + tag + ")\"\n", re.MULTILINE)
    content = regex.sub('', content)

    with open(file_path, "w") as out:
        out.write(content)

def copy_header_only_files():
    """Copy required header only files"""
    base_src_path = android_path('external/icu/icu4c/source/')
    base_dest_path = android_path('external/icu/libicu/ndk_headers/unicode/')
    with open(android_path('external/icu/tools/icu4c_srcgen/libicu_required_header_only_files.txt'),
              'r') as in_file:
        header_only_files = [
            base_src_path + line.strip() for line in in_file.readlines() if not line.startswith('#')
        ]

    for src_path in header_only_files:
        dest_path = base_dest_path + os.path.basename(src_path)
        cmd = ['sed',
               "s/U_SHOW_CPLUSPLUS_API/LIBICU_U_SHOW_CPLUSPLUS_API/g",
               src_path
               ]

        with open(dest_path, "w") as destfile:
            subprocess.check_call(cmd, stdout=destfile)

def copy_cts_headers():
    """Copy headers from common/ and i18n/ to cts_headers/ for compiling cintltst as CTS."""
    dst_folder = android_path('external/icu/libicu/cts_headers')
    if os.path.exists(dst_folder):
        shutil.rmtree(dst_folder)
    os.mkdir(dst_folder)
    os.mkdir(os.path.join(dst_folder, 'unicode'))

    shutil.copyfile(android_path('external/icu/android_icu4c/include/uconfig_local.h'),
                    android_path('external/icu/libicu/cts_headers/uconfig_local.h'))

    header_subfolders = (
        'common',
        'common/unicode',
        'i18n',
        'i18n/unicode',
    )
    for subfolder in header_subfolders:
        path = android_path('external/icu/icu4c/source', subfolder)
        files = [os.path.join(path, f) for f in os.listdir(path) if f.endswith('.h')]

        for src_path in files:
            base_header_name = os.path.basename(src_path)
            dst_path = dst_folder
            if subfolder.endswith('unicode'):
                dst_path = os.path.join(dst_path, 'unicode')
            dst_path = os.path.join(dst_path, base_header_name)

            shutil.copyfile(src_path, dst_path)

def get_rename_macro_regex(decl_names):
    """Return a regex in string to capture the C macro defining the name in the decl_names list"""
    tag = "|".join(decl_names)
    return re.compile(r"^(#define (?:" + tag + r") .*)$", re.MULTILINE)

def generate_cts_headers(decl_names):
    """Generate headers for compiling cintltst as CTS."""
    copy_cts_headers()

    # Disable all C macro renaming the NDK functions in order to test the functions in the CTS
    urename_path = android_path('external/icu/libicu/cts_headers/unicode/urename.h')
    with open(urename_path, "r") as file:
        src = file.read()

    regex = get_rename_macro_regex(decl_names)
    modified = regex.sub(r"// \1", src)

    with open(urename_path, "w") as out:
        out.write(modified)

IGNORED_INCLUDE_DEPENDENCY = {
    "ubrk.h": ["parseerr.h", ],
    "ulocdata.h": ["ures.h", "uset.h", ],
    "unorm2.h": ["uset.h", ],
    "ustring.h": ["uiter.h", ],
}

def main():
    """Parse the ICU4C headers and generate the shim libicu."""
    logging.basicConfig(level=logging.DEBUG)

    allowlisted_apis = get_allowlisted_apis('libicu_export.txt')
    decl_filters = [StableDeclarationFilter()]
    decl_filters.append(AllowlistedDeclarationFilter(allowlisted_apis))
    parser = DeclaredFunctionsParser(decl_filters, [])
    parser.set_ignored_include_dependency(IGNORED_INCLUDE_DEPENDENCY)

    parser.parse()

    includes = parser.header_includes
    functions = parser.declared_functions
    header_to_function_names = parser.header_to_function_names

    # The shim has the allowlisted functions only
    functions = [f for f in functions if f.name in allowlisted_apis]

    headers_folder = android_path('external/icu/libicu/ndk_headers/unicode')
    if os.path.exists(headers_folder):
        shutil.rmtree(headers_folder)
    os.mkdir(headers_folder)

    with open(android_path('external/icu/libicu/src/shim.cpp'),
              'w') as out_file:
        out_file.write(generate_shim(functions, includes, SYMBOL_SUFFIX, 'libicu_shim.cpp.j2')
                       .encode('utf8'))

    with open(android_path('external/icu/libicu/libicu.map.txt'), 'w') as out_file:
        out_file.write(generate_symbol_txt(functions, [], 'libicu.map.txt.j2')
                       .encode('utf8'))

    # Process the C headers and put them into the ndk folder.
    for src_path in parser.header_paths_to_copy:
        basename = os.path.basename(src_path)
        dst_path = os.path.join(headers_folder, basename)
        modify_func_declarations(src_path, dst_path, header_to_function_names[basename])
        # Remove #include lines from the header files.
        if basename in IGNORED_INCLUDE_DEPENDENCY:
            remove_ignored_includes(dst_path, IGNORED_INCLUDE_DEPENDENCY[basename])

    copy_header_only_files()

    generate_cts_headers(allowlisted_apis)

    # Apply documentation patches by the following shell script
    subprocess.check_call(
        [android_path('external/icu/tools/icu4c_srcgen/doc_patches/apply_patches.sh')])

if __name__ == '__main__':
    main()
