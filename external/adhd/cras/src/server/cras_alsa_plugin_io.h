/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_ALSA_PLUGIN_IO_H_
#define CRAS_ALSA_PLUGIN_IO_H_

/*
 * Disclaimer:
 * The ALSA plugin path in CRAS is intended to be used for development or
 * testing. CrOS audio team is not responsible for nor provides hot-fix to
 * any breakage if it’s used in production code.
 */

void alsa_pluigin_io_destroy_all();

void cras_alsa_plugin_io_init(const char *device_config_dir);

#endif /* CRAS_ALSA_PLUGIN_IO_H_ */
