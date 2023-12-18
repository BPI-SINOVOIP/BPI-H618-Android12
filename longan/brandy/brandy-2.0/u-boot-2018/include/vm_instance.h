// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Google LLC
 */

#ifndef _VM_INSTANCE_H
#define _VM_INSTANCE_H

#include <avb_verify.h>

/**
 * Load the VM instance file partition with the given UUID.
 *
 * The contents of the partition are decrypted with the sealing key before
 * being returned via the data argument if the function is successful. Returns
 * zero if successful, -NOENT if the partition was not found or a negative
 * error code otherwise.
 */
int vm_instance_load_entry(struct AvbOps *ops, const struct uuid *uuid,
			   const uint8_t *sealing_key, size_t sealing_key_size,
			   void *data, size_t data_size);

/**
 * Save to the VM instance partition with the given UUID.
 *
 * The data is encrypted with the sealing key before being written to the
 * partition. Returns zero if successful, a negative error code otherwise.
 */
int vm_instance_save_entry(struct AvbOps *ops, const struct uuid *uuid,
			   const uint8_t *sealing_key, size_t sealing_key_size,
			   const void *data, size_t data_size);

/**
 * Generate a new salt to bind to the VM instance and store in the VM instance
 * file entry.
 */
#define VM_INSTANCE_SALT_SIZE	64
int vm_instance_new_salt(uint8_t salt[VM_INSTANCE_SALT_SIZE]);

#endif /* _VM_INSTANCE_H */
