#############################################################################
#
# Copyright (C) 2019 Collabora Ltd
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#

sources_libgallium = [
   'include/c99_compat.h',
   'include/no_extern_c.h',
   'include/pipe/p_config.h',
   'include/pipe/p_defines.h',
   'include/pipe/p_context.h',
   'include/pipe/p_state.h',
   'include/pipe/p_format.h',
   'include/pipe/p_shader_tokens.h',
   'include/pipe/p_screen.h',
   'include/pipe/p_compiler.h',
   'include/pipe/p_video_enums.h',
   'include/c11/threads_win32.h',
   'include/c11/threads.h',
   'include/c11/threads_posix.h',
   'auxiliary/util/u_format.h',
   'auxiliary/util/u_memory.h',
   'auxiliary/util/u_rect.h',
   'auxiliary/util/u_surface.h',
   'auxiliary/util/u_math.h',
   'auxiliary/util/rgtc.h',
   'auxiliary/util/u_format.c',
   'auxiliary/util/u_debug.h',
   'auxiliary/util/u_inlines.h',
   'auxiliary/util/u_texture.c',
   'auxiliary/util/u_pointer.h',
   'auxiliary/util/u_hash_table.c',
   'auxiliary/util/u_debug_describe.h',
   'auxiliary/util/u_dual_blend.h',
   'auxiliary/util/u_texture.h',
   'auxiliary/util/u_hash_table.h',
   'auxiliary/util/u_box.h',
   'auxiliary/util/u_debug.c',
   'auxiliary/util/u_cpu_detect.c',
   'auxiliary/util/u_pack_color.h',
   'auxiliary/util/u_double_list.h',
   'auxiliary/util/u_debug_refcnt.h',
   'auxiliary/util/u_bitmask.c',
   'auxiliary/util/u_cpu_detect.h',
   'auxiliary/util/u_bitmask.h',
   'auxiliary/util/u_format_s3tc.h',
   'auxiliary/util/u_string.h',
   'auxiliary/util/u_surface.c',
   'auxiliary/util/u_math.c',
   'auxiliary/util/u_half.h',
   'auxiliary/util/u_prim.h',
   'auxiliary/util/u_debug_describe.c',
   'auxiliary/util/u_atomic.h',
   'auxiliary/cso_cache/cso_hash.h',
   'auxiliary/cso_cache/cso_cache.h',
   'auxiliary/cso_cache/cso_cache.c',
   'auxiliary/cso_cache/cso_hash.c',
   'auxiliary/tgsi/tgsi_opcode_tmp.h',
   'auxiliary/tgsi/tgsi_dump.c',
   'auxiliary/tgsi/tgsi_ureg.c',
   'auxiliary/tgsi/tgsi_build.c',
   'auxiliary/tgsi/tgsi_build.h',
   'auxiliary/tgsi/tgsi_util.h',
   'auxiliary/tgsi/tgsi_iterate.h',
   'auxiliary/tgsi/tgsi_scan.c',
   'auxiliary/tgsi/tgsi_info.c',
   'auxiliary/tgsi/tgsi_parse.h',
   'auxiliary/tgsi/tgsi_text.h',
   'auxiliary/tgsi/tgsi_strings.h',
   'auxiliary/tgsi/tgsi_ureg.h',
   'auxiliary/tgsi/tgsi_parse.c',
   'auxiliary/tgsi/tgsi_transform.h',
   'auxiliary/tgsi/tgsi_info.h',
   'auxiliary/tgsi/tgsi_text.c',
   'auxiliary/tgsi/tgsi_strings.c',
   'auxiliary/tgsi/tgsi_sanity.c',
   'auxiliary/tgsi/tgsi_scan.h',
   'auxiliary/tgsi/tgsi_iterate.c',
   'auxiliary/tgsi/tgsi_dump.h',
   'auxiliary/tgsi/tgsi_util.c',
   'auxiliary/tgsi/tgsi_sanity.h',
   'auxiliary/tgsi/tgsi_transform.c',
   'auxiliary/os/os_memory_aligned.h',
   'auxiliary/os/os_thread.h',
   'auxiliary/os/os_mman.h',
   'auxiliary/os/os_misc.h',
   'auxiliary/os/os_memory.h',
   'auxiliary/os/os_memory_debug.h',
   'auxiliary/os/os_memory_stdc.h',
   'auxiliary/os/os_misc.c',
]

inc_gallium = include_directories('include', 'auxiliary', 'auxiliary/util', )

u_format_table_c = custom_target(
   'u_format_table.c',
   input : ['auxiliary/util/u_format_table.py', 'auxiliary/util/u_format.csv'],
   output : 'u_format_table.c',
   command : [prog_python, '@INPUT@'],
   depend_files : files('auxiliary/util/u_format_parse.py'),
   capture : true,
)

libgallium = static_library(
   'gallium',
   [sources_libgallium, u_format_table_c],
   include_directories : [
      inc_gallium, include_directories('..')
   ],
)

gallium_dep = declare_dependency(
   link_with: libgallium,
   include_directories: inc_gallium
)
