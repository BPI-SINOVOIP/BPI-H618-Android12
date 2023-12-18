// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Google LLC
 */

#ifndef _BCC_H
#define _BCC_H

#include <avb_verify.h>

struct bcc_context;

enum bcc_mode {
	BCC_MODE_NORMAL,
	BCC_MODE_DEBUG,
	BCC_MODE_MAINTENANCE,
};

int bcc_init(void);
void bcc_set_handover(void *handover, size_t handover_size) ;


enum bcc_vm_instance_result {
	BCC_VM_INSTANCE_FOUND,
	BCC_VM_INSTANCE_CREATED,
};

int bcc_vm_instance_handover(const char *iface_str, int devnum,
			     const char *instance_uuid, bool must_exist,
			     const char *component_name,
			     enum bcc_mode boot_mode,
			     const AvbSlotVerifyData *code_data,
			     const AvbSlotVerifyData *config_data,
			     const void *unverified_config,
			     size_t unverified_config_size);

int bcc_vm_instance_avf_boot_state(bool *strict_boot, bool *new_instance);

/**
 * Get a sealing key derived from the sealing CDI.
 *
 * The sealing CDI should not be used directly as a key but should have keys
 * derived from it. This functions deterministically derives keys from the
 * sealing CDI based on the info parameter. Returns zero if successful, a
 * negative error code otherwise.
 */
int bcc_get_sealing_key(const uint8_t *info, size_t info_size,
			uint8_t *out_key, size_t out_key_size);

/**
 * Zero given memory buffer and flush dcache
 */
void bcc_clear_memory(void *data, size_t size);

#endif /* _BCC_H */
