// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

#ifndef __ABG_INTERNAL_H__
#define __ABG_INTERNAL_H__
#include "config.h"

#ifdef HAS_GCC_VISIBILITY_ATTRIBUTE

/// This macro makes a declaration be hidden at the binary level.
///
/// On ELF systems, this means that the symbol for the declaration
/// (function or variable) is going to be local to the file.  External
/// ELF files won't be able to link against the symbol.
#define ABG_HIDDEN  __attribute__((visibility("hidden")))

/// This macro makes a declaration be exported at the binary level.
///
/// On ELF systems, this means that the symbol for the declaration
///(function or variable) is going to be global.  External ELF files
///will be able to link against the symbol.
#define ABG_EXPORTED __attribute__((visibility("default")))
#define ABG_BEGIN_EXPORT_DECLARATIONS _Pragma("GCC visibility push(default)")
#define ABG_END_EXPORT_DECLARATIONS _Pragma("GCC visibility pop")
#else
#define ABG_HIDDEN
#define ABG_EXPORTED
#define ABG_BEGIN_EXPORT_DECLARATIONS
#define ABG_END_EXPORT_DECLARATIONS
#endif
#endif // __ABG_INTERNAL_H__
