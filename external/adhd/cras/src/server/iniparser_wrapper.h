/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef INIPARSER_WRAPPER_H_
#define INIPARSER_WRAPPER_H_

#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Allocate 63 chars + 1 for null where declared. */
#define MAX_INI_NAME_LENGTH 63
#define MAX_INI_KEY_LENGTH 63 /* names like "output_source:output_0" */

static inline dictionary *iniparser_load_wrapper(const char *ini_name)
{
	struct stat st;
	int rc = stat(ini_name, &st);
	if (rc < 0)
		return NULL;
	return iniparser_load(ini_name);
}

#endif /* INIPARSER_WRAPPER_H_ */
