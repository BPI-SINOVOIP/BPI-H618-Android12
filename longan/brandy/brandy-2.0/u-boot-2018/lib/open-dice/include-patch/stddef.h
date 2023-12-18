// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021 Google LLC
 */

#ifndef _ALT_STDDEF_H
#define _ALT_STDDEF_H
#include <compiler.h>
#include <linux/kernel.h>
#undef __UINTPTR_TYPE__

#define INT64_MAX 0x7fffffffffffffffLL
#endif