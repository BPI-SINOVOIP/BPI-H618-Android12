// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Google LLC
 */

#include <bcc.h>
#include <malloc.h>
#include <u-boot/sha256.h>
#include <vm_instance.h>

#include <dice/android/bcc.h>
#include <dice/cbor_reader.h>
#include <dice/cbor_writer.h>
#include <dice/ops.h>

#include <dm/device.h>
#include <dm/device_compat.h>
#include <dm/uclass.h>
#include <dm/read.h>
#include <linux/ioport.h>

#include <openssl/evp.h>
#include <openssl/hkdf.h>

#define BCC_CONFIG_DESC_SIZE	64

struct boot_measurement {
	const uint8_t *public_key;
	size_t public_key_size;
	uint8_t digest[AVB_SHA256_DIGEST_SIZE];
};

static uint8_t *bcc_handover_buffer;
static size_t bcc_handover_buffer_size;

static const DiceMode bcc_to_dice_mode[] = {
	[BCC_MODE_NORMAL] = kDiceModeNormal,
	[BCC_MODE_MAINTENANCE] = kDiceModeMaintenance,
	[BCC_MODE_DEBUG] = kDiceModeDebug,
};

static void *find_bcc_handover(size_t *size)
{
	struct udevice *dev;
	struct resource res;

	/* Probe drivers that provide a BCC handover buffer. */
	for (uclass_first_device(UCLASS_DICE, &dev); dev; uclass_next_device(&dev)) {
		if (!dev_read_resource(dev, 0, &res)) {
			*size = resource_size(&res);
			return (void *)res.start;
		}
	}

	return NULL;
}

int bcc_init(void)
{
	/* Idempotent initialization to allow indepedent client modules. */
	if (bcc_handover_buffer && bcc_handover_buffer_size)
		return 0;
	/* If a BCC handover wasn't already set, look for a driver. */
	bcc_handover_buffer = find_bcc_handover(&bcc_handover_buffer_size);
	if (!bcc_handover_buffer || !bcc_handover_buffer_size)
		return -ENOENT;
	return 0;
}

void bcc_set_handover(void *handover, size_t handover_size)
{
	bcc_handover_buffer = handover;
	bcc_handover_buffer_size = handover_size;
}

void bcc_clear_memory(void *data, size_t size)
{
	DiceClearMemory(NULL, size, data);
}

static void sha256(const void *data, size_t data_size, uint8_t digest[AVB_SHA256_DIGEST_SIZE])
{
	sha256_context ctx;

	sha256_starts(&ctx);
	sha256_update(&ctx, data, data_size);
	sha256_finish(&ctx, digest);
}

/**
 * Format the VM instance data into a CBOR record.
 *
 * The salt is left uninitialized and a pointer to it is returned.
 *
 *  Measurement = [
 *    1: bstr,		// public key
 *    2: bstr,		// digest
 *  ]
 *
 *  InstanceData = {
 *    1: bstr .size 64,	// salt
 *    2: Measurement,	// code
 *    ? 3: Measurement,	// config
 *  }
 */
static int vm_instance_format(const struct boot_measurement *code,
			      const struct boot_measurement *config,
			      size_t buffer_size, uint8_t *buffer,
			      size_t *formatted_size, uint8_t **salt)
{
	const int64_t salt_label = 1;
	const int64_t code_label = 2;
	const int64_t config_label = 3;
	const int64_t public_key_label = 1;
	const int64_t digest_label = 2;

	struct CborOut out;

	if (!code || !formatted_size)
		return -EINVAL;

	CborOutInit(buffer, buffer_size, &out);
	CborWriteMap(/*num_pairs=*/config ? 3 : 1, &out);

	CborWriteInt(salt_label, &out);
	*salt = CborAllocBstr(VM_INSTANCE_SALT_SIZE, &out);

	CborWriteInt(code_label, &out);
	CborWriteMap(/*num_pairs=*/2, &out);
	CborWriteInt(public_key_label, &out);
	CborWriteBstr(code->public_key_size, code->public_key, &out);
	CborWriteInt(digest_label, &out);
	CborWriteBstr(AVB_SHA256_DIGEST_SIZE, code->digest, &out);

	if (config) {
		CborWriteInt(config_label, &out);
		CborWriteMap(/*num_pairs=*/2, &out);
		CborWriteInt(public_key_label, &out);
		CborWriteBstr(config->public_key_size, config->public_key, &out);
		CborWriteInt(digest_label, &out);
		CborWriteBstr(AVB_SHA256_DIGEST_SIZE, config->digest, &out);
	}

	*formatted_size = CborOutSize(&out);

	return CborOutOverflowed(&out) ? -E2BIG : 0;
}

/**
 * Verify the measurements are the same as previously recorded for the VM
 * instance or create a new VM instance with the measurements.
 *
 * The salt that is saved with the instance data is added as a hidden input to
 * bcc_ctx if the measurements don't conflict with the previously recorded
 * measurements.
 */
static int vm_instance_verify(const char *iface_str, int devnum,
			      const char *instance_uuid, bool must_exist,
			      const struct boot_measurement *code,
			      const struct boot_measurement *config,
			      uint8_t out_hidden[VM_INSTANCE_SALT_SIZE])
{
	const uint8_t key_info[] = {
			'v', 'm', '-', 'i', 'n', 's', 't', 'a', 'n', 'c', 'e' };
	uint8_t *record;
	uint8_t *record_salt;
	uint8_t *saved_record;
	size_t record_size;
	struct AvbOps *ops;
	char devnum_str[3];
	struct uuid uuid;
	uint8_t sealing_key[32];
	int ret;

	if (uuid_str_to_bin(instance_uuid, (unsigned char *)&uuid,
			    UUID_STR_FORMAT_STD))
		return -EINVAL;

	/* Measure the formatted record to allocate large enough buffers. */
	ret = vm_instance_format(code, config, /*buffer_size=*/0,
				 /*buffer=*/NULL, &record_size, &record_salt);
	if (ret && ret != -E2BIG)
		return ret;
	record = malloc(record_size);
	if (!record)
		return -ENOMEM;

	saved_record = malloc(record_size);
	if (!saved_record) {
		free(record);
		return -ENOMEM;
	}

	/* Format the record into the buffer. */
	ret = vm_instance_format(code, config, record_size, record,
				 &record_size, &record_salt);
	if (ret)
		return ret;

	snprintf(devnum_str, sizeof(devnum_str), "%d", devnum);
	ops = avb_ops_alloc(iface_str, devnum_str);
	if (!ops) {
		ret = -EINVAL;
		goto out;
	}

	ret = bcc_get_sealing_key(key_info, sizeof(key_info),
				  sealing_key, sizeof(sealing_key));
	if (ret)
		goto out;

	/* Load any previous data, failing if it's a different size. */
	ret = vm_instance_load_entry(ops, &uuid,
				     sealing_key, sizeof(sealing_key),
				     saved_record, record_size);
	if (ret == 0) {
		ptrdiff_t salt_offset = record_salt - record;

		/*
		 * Copy the assumed saved salt so the records will be
		 * byte-for-byte identical if they match.
		 */
		memcpy(record_salt, saved_record + salt_offset,
		       VM_INSTANCE_SALT_SIZE);
		if (memcmp(record, saved_record, record_size) != 0) {
			log_err("VM instance data mismatch.\n");
			ret = -EINVAL;
			goto out;
		}
		ret = BCC_VM_INSTANCE_FOUND;
	} else if (ret == -ENOENT) {
		if (must_exist) {
			log_err("VM instance data not found.\n");
			goto out;
		}

		/* No previous entry so create a fresh one. */
		printf("Creating new VM instance.\n");

		ret = vm_instance_new_salt(record_salt);
		if (ret)
			goto out;

		ret = vm_instance_save_entry(ops, &uuid,
					     sealing_key, sizeof(sealing_key),
					     record, record_size);
		if (ret) {
			log_err("Failed to create VM instance.\n");
			goto out;
		}
		ret = BCC_VM_INSTANCE_CREATED;
	}

	memcpy(out_hidden, record_salt, VM_INSTANCE_SALT_SIZE);

out:
	avb_ops_free(ops);
	bcc_clear_memory(sealing_key, sizeof(sealing_key));
	bcc_clear_memory(record, record_size);
	bcc_clear_memory(saved_record, record_size);
	free(record);
	free(saved_record);
	return ret;
}

static int boot_measurement_from_avb_data(const AvbSlotVerifyData *data,
					  struct boot_measurement *measurement)
{
	const char *partition_name;
	size_t n;

	/*
	 * Use the public key of the top-level vbmeta as the authority as this
	 * cannot change. The authority of any chained partitions will be
	 * captured in the vbmeta digest.
	 */
	if (avb_find_main_pubkey(data, &measurement->public_key,
				 &measurement->public_key_size)
			== CMD_RET_FAILURE)
		return -EINVAL;

	/*
	 * Assuming we're only trying to load one thing, a top-level vbmeta with
	 * no chained partitions or a single partition that was chained from a
	 * top-level vbmeta.
	 *
	 * Calculate a hash that captures just the code we're interested in
	 * loading and will match:
	 *
	 *    avbtool calculate_vbmeta_digest --image <vbmeta|partition>.img
	 */
	if (data->num_vbmeta_images == 1)
		partition_name = data->vbmeta_images[0].partition_name;
	else if (data->num_loaded_partitions == 1)
		partition_name = data->loaded_partitions[0].partition_name;
	else
		return -EINVAL;

	for (n = 0; n < data->num_vbmeta_images; ++n) {
		AvbVBMetaData *vbmeta = &data->vbmeta_images[n];

		if (strcmp(partition_name, vbmeta->partition_name) == 0) {
			sha256(vbmeta->vbmeta_data, vbmeta->vbmeta_size,
			       measurement->digest);
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * Format a config descriptor conformaing to the BCC specification.
 *
 * BccConfigDescriptor = {
 *   ? -70002 : tstr,		; component name
 *   ? -70003 : int,		; component version
 *   ? -70004 : null,		; resettable
 *   ; Extension fields
 *   -71000: bstr .size 32;	; config digest
 * }
 */
DiceResult format_config_descriptor(const char *component_name,
				    const uint8_t config_digest[AVB_SHA256_DIGEST_SIZE],
				    size_t buffer_size, uint8_t *buffer,
				    size_t *actual_size)
{
	const int64_t component_name_label = -70002;
	const int64_t config_digest_label = -71000;
	struct CborOut out;

	CborOutInit(buffer, buffer_size, &out);
	CborWriteMap(config_digest ? 2 : 1, &out);
	CborWriteInt(component_name_label, &out);
	CborWriteTstr(component_name, &out);
	if (config_digest) {
		CborWriteInt(config_digest_label, &out);
		CborWriteBstr(AVB_SHA256_DIGEST_SIZE, config_digest, &out);
	}
	*actual_size = CborOutSize(&out);
	return CborOutOverflowed(&out) ? -E2BIG : 0;
}

int bcc_vm_instance_handover(const char *iface_str, int devnum,
			     const char *instance_uuid, bool must_exist,
			     const char *component_name,
			     enum bcc_mode bcc_mode,
			     const AvbSlotVerifyData *code_data,
			     const AvbSlotVerifyData *config_data,
			     const void *unverified_config,
			     size_t unverified_config_size)
{
	uint8_t config_desc[BCC_CONFIG_DESC_SIZE];
	struct boot_measurement code;
	struct boot_measurement config = {};
	uint8_t *new_handover = NULL;
	DiceInputValues input_vals;
	DiceResult res;
	int instance_ret, ret;

	/* Continue without the BCC if it wasn't found. */
	ret = bcc_init();
	if (ret == -ENOENT)
		return 0;
	if (ret)
		return ret;

	/* For now, only allow one config source. */
	if (config_data && unverified_config)
		return -EINVAL;

	ret = boot_measurement_from_avb_data(code_data, &code);
	if (ret)
		return ret;

	if (config_data) {
		ret = boot_measurement_from_avb_data(config_data, &config);
		if (ret)
			return ret;

		/* For now, require the same authority. */
		if (config.public_key_size != code.public_key_size ||
		    memcmp(config.public_key, code.public_key,
			   code.public_key_size) != 0)
			return -EINVAL;
	} else if (unverified_config) {
		sha256(unverified_config, unverified_config_size,
		       config.digest);
	}

	input_vals = (DiceInputValues){
		.config_type = kDiceConfigTypeDescriptor,
		.config_descriptor = config_desc,
		.mode = bcc_to_dice_mode[bcc_mode],
	};

	memcpy(input_vals.code_hash, code.digest, AVB_SHA256_DIGEST_SIZE);
	sha256(code.public_key, code.public_key_size, input_vals.authority_hash);
	ret = format_config_descriptor(component_name, config.digest,
				       sizeof(config_desc), config_desc,
				       &input_vals.config_descriptor_size);
	if (ret)
		return ret;

	instance_ret = vm_instance_verify(iface_str, devnum, instance_uuid,
					  must_exist, &code,
					  config_data ? &config : NULL,
					  input_vals.hidden);
	if (instance_ret < 0) {
		ret = instance_ret;
		log_err("Failed to validate instance.\n");
		goto out;
	}
	printf("Booting VM instance.\n");

	new_handover = calloc(1, bcc_handover_buffer_size);
	if (!new_handover)
		return -ENOMEM;

	res = BccHandoverMainFlow(/*context=*/NULL, bcc_handover_buffer,
				  bcc_handover_buffer_size, &input_vals,
				  bcc_handover_buffer_size, new_handover, NULL);
	if (res != kDiceResultOk) {
		ret = -EINVAL;
		goto out;
	}

	/* Update the handover buffer with the new data. */
	memcpy(bcc_handover_buffer, new_handover, bcc_handover_buffer_size);

out:
	if (new_handover) {
		bcc_clear_memory(new_handover, bcc_handover_buffer_size);
		free(new_handover);
	}
	bcc_clear_memory(&input_vals, sizeof(input_vals));
	return ret ? ret : instance_ret;
}

int bcc_vm_instance_avf_boot_state(bool *strict_boot, bool *new_instance)
{
	const void *fdt;
	int chosen, len;

	if (!CONFIG_IS_ENABLED(ARM64)) {
		*strict_boot = false;
		*new_instance = false;
		return 0;
	}

	fdt = (const void *)env_get_hex("fdtaddr", 0);
	if (!fdt)
		return -EINVAL;

	chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen == -FDT_ERR_NOTFOUND) {
		*strict_boot = false;
		*new_instance = false;
		return 0;
	}
	if (chosen < 0)
		return -EINVAL;

	fdt_getprop(fdt, chosen, "avf,strict-boot", &len);
	if (len >= 0)
		*strict_boot = true;
	else if (len == -FDT_ERR_NOTFOUND)
		*strict_boot = false;
	else
		return -EINVAL;

	fdt_getprop(fdt, chosen, "avf,new-instance", &len);
	if (len >= 0)
		*new_instance = true;
	else if (len == -FDT_ERR_NOTFOUND)
		*new_instance = false;
	else
		return -EINVAL;

	return 0;
}

int bcc_get_sealing_key(const uint8_t *info, size_t info_size,
			uint8_t *out_key, size_t out_key_size)
{
	const uint64_t cdi_attest_label = 1;
	const uint64_t cdi_seal_label = 2;
	struct CborIn in;
	int64_t label;
	size_t item_size;
	const uint8_t *ptr;
	int ret;

	/* Make sure initialization is complete. */
	ret = bcc_init();
	if (ret)
		return ret;

	CborInInit(bcc_handover_buffer, bcc_handover_buffer_size, &in);
	if (CborReadMap(&in, &item_size) != CBOR_READ_RESULT_OK ||
	    item_size < 3 ||
	    // Read the attestation CDI.
	    CborReadInt(&in, &label) != CBOR_READ_RESULT_OK ||
	    label != cdi_attest_label ||
	    CborReadBstr(&in, &item_size, &ptr) != CBOR_READ_RESULT_OK ||
	    item_size != DICE_CDI_SIZE ||
	    // Read the sealing CDI.
	    CborReadInt(&in, &label) != CBOR_READ_RESULT_OK ||
	    label != cdi_seal_label ||
	    CborReadBstr(&in, &item_size, &ptr) != CBOR_READ_RESULT_OK ||
	    item_size != DICE_CDI_SIZE)
		return -EINVAL;

	if (!HKDF(out_key, out_key_size, EVP_sha512(),
		  ptr, DICE_CDI_SIZE, /*salt=*/NULL, /*salt_len=*/0,
		  info, info_size)) {
		return -EIO;
	}

	return 0;
}
