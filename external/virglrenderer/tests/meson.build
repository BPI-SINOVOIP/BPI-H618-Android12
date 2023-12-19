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

check_dep = dependency('check')

libvrtest_sources = [
   'testvirgl.c',
   'testvirgl.h',
   'testvirgl_encode.c',
   'testvirgl_encode.h',
]

libvrtest = static_library(
   'vrtest',
   libvrtest_sources,
   dependencies : [
      libvirgl_dep,
      gallium_dep,
      check_dep
   ]
)

tests = [
   ['test_virgl_init', 'test_virgl_init.c'],
   ['test_virgl_fence', 'test_virgl_fence.c'],
   ['test_virgl_resource', 'test_virgl_resource.c'],
   ['test_virgl_transfer', 'test_virgl_transfer.c'],
   ['test_virgl_cmd', 'test_virgl_cmd.c'],
   ['test_virgl_strbuf', 'test_virgl_strbuf.c']
]

fuzzy_tests = [
   ['test_fuzzer_formats', 'test_fuzzer_formats.c'],
]

foreach t : tests
   test_virgl = executable(t[0], t[1], link_with: libvrtest,
                           dependencies : [libvirglrenderer_dep, check_dep])
   test(t[0], test_virgl)
endforeach

foreach t : fuzzy_tests
   test_virgl_fuzzy = executable(t[0], t[1], link_with: libvrtest,
                                 dependencies : [libvirglrenderer_dep, epoxy_dep])
   test(t[0], test_virgl)
endforeach


if with_valgrind
   valgrind = find_program('valgrind')
   surpression_path = join_paths(meson.current_source_dir(), 'valgrind.suppressions')
   args = ['--leak-check=full', '--quiet', '--error-exitcode=3',
           '--suppressions='+ surpression_path]
   foreach t : tests
      test('valgrind-' + t[0],
           valgrind,
           args : args + [join_paths(meson.current_build_dir(), t[0])],
           timeout : 1800)
   endforeach
endif

if with_fuzzer
   subdir('fuzzer')
endif
