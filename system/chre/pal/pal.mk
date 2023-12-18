#
# PAL Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Ipal/include

# GoogleTest Source Files ######################################################

GOOGLETEST_CFLAGS += -Ipal/tests/include
GOOGLETEST_SRCS += pal/tests/src/version_test.cc
GOOGLETEST_SRCS += pal/tests/src/wwan_test.cc
GOOGLETEST_PAL_IMPL_SRCS += pal/tests/src/gnss_pal_impl_test.cc
GOOGLETEST_PAL_IMPL_SRCS += pal/tests/src/wifi_pal_impl_test.cc
