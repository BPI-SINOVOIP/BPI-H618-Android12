/* SPDX-License-Identifier: MIT */
/*
 * libfsverity API
 *
 * Copyright 2018 Google LLC
 * Copyright (C) 2020 Facebook
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#ifndef LIBFSVERITY_H
#define LIBFSVERITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define FSVERITY_UTILS_MAJOR_VERSION	1
#define FSVERITY_UTILS_MINOR_VERSION	3

#define FS_VERITY_HASH_ALG_SHA256       1
#define FS_VERITY_HASH_ALG_SHA512       2

/**
 * struct libfsverity_merkle_tree_params - properties of a file's Merkle tree
 *
 * Zero this, then fill in at least @version and @file_size.
 */
struct libfsverity_merkle_tree_params {

	/** @version: must be 1 */
	uint32_t version;

	/**
	 * @hash_algorithm: one of FS_VERITY_HASH_ALG_*, or 0 to use the default
	 * of FS_VERITY_HASH_ALG_SHA256
	 */
	uint32_t hash_algorithm;

	/** @file_size: the file size in bytes */
	uint64_t file_size;

	/**
	 * @block_size: the Merkle tree block size in bytes, or 0 to use the
	 * default of 4096 bytes
	 */
	uint32_t block_size;

	/** @salt_size: the salt size in bytes, or 0 if unsalted */
	uint32_t salt_size;

	/** @salt: pointer to the salt, or NULL if unsalted */
	const uint8_t *salt;

	/** @reserved1: must be 0 */
	uint64_t reserved1[8];

	/** @reserved2: must be 0 */
	uintptr_t reserved2[8];
};

struct libfsverity_digest {
	uint16_t digest_algorithm;	/* one of FS_VERITY_HASH_ALG_* */
	uint16_t digest_size;		/* digest size in bytes */
	uint8_t digest[];		/* the actual digest */
};

struct libfsverity_signature_params {
	const char *keyfile;		/* path to key file (PEM format) */
	const char *certfile;		/* path to certificate (PEM format) */
	uint64_t reserved1[8];		/* must be 0 */
	uintptr_t reserved2[8];		/* must be 0 */
};

/*
 * libfsverity_read_fn_t - callback that incrementally provides a file's data
 * @fd: the user-provided "file descriptor" (opaque to library)
 * @buf: buffer into which to read the next chunk of the file's data
 * @count: number of bytes to read in this chunk
 *
 * Must return 0 on success (all 'count' bytes read), or a negative errno value
 * on failure.
 */
typedef int (*libfsverity_read_fn_t)(void *fd, void *buf, size_t count);

/**
 * libfsverity_compute_digest() - Compute digest of a file
 *          A fs-verity file digest is the hash of a file's fsverity_descriptor.
 *          Not to be confused with a traditional file digest computed over the
 *          entire file, or with the bare fsverity_descriptor::root_hash.
 * @fd: context that will be passed to @read_fn
 * @read_fn: a function that will read the data of the file
 * @params: Pointer to the Merkle tree parameters
 * @digest_ret: Pointer to pointer for computed digest.
 *
 * Returns:
 * * 0 for success, -EINVAL for invalid input arguments, -ENOMEM if libfsverity
 *   failed to allocate memory, or an error returned by @read_fn.
 * * digest_ret returns a pointer to the digest on success. The digest object
 *   is allocated by libfsverity and must be freed by the caller using free().
 */
int
libfsverity_compute_digest(void *fd, libfsverity_read_fn_t read_fn,
			   const struct libfsverity_merkle_tree_params *params,
			   struct libfsverity_digest **digest_ret);

/**
 * libfsverity_sign_digest() - Sign previously computed digest of a file
 *          This signature is used by the filesystem to validate the signed file
 *          digest against a public key loaded into the .fs-verity kernel
 *          keyring, when CONFIG_FS_VERITY_BUILTIN_SIGNATURES is enabled. The
 *          signature is formatted as PKCS#7 stored in DER format. See
 *          Documentation/filesystems/fsverity.rst in the kernel source tree for
 *          further details.
 * @digest: pointer to previously computed digest
 * @sig_params: struct libfsverity_signature_params providing filenames of
 *          the keyfile and certificate file. Reserved fields must be zero.
 * @sig_ret: Pointer to pointer for signed digest
 * @sig_size_ret: Pointer to size of signed return digest
 *
 * Return:
 * * 0 for success, -EINVAL for invalid input arguments or if the cryptographic
 *   operations to sign the digest failed, -EBADMSG if the key and/or
 *   certificate file is invalid, or another negative errno value.
 * * sig_ret returns a pointer to the signed digest on success. This object
 *   is allocated by libfsverity and must be freed by the caller using free().
 * * sig_size_ret returns the size (in bytes) of the signed digest on success.
 */
int
libfsverity_sign_digest(const struct libfsverity_digest *digest,
			const struct libfsverity_signature_params *sig_params,
			uint8_t **sig_ret, size_t *sig_size_ret);

/**
 * libfsverity_enable() - Enable fs-verity on a file
 * @fd: read-only file descriptor to the file
 * @params: pointer to the Merkle tree parameters
 *
 * This is a simple wrapper around the FS_IOC_ENABLE_VERITY ioctl.
 *
 * Return: 0 on success, -EINVAL for invalid arguments, or a negative errno
 *	   value from the FS_IOC_ENABLE_VERITY ioctl.  See
 *	   Documentation/filesystems/fsverity.rst in the kernel source tree for
 *	   the possible error codes from FS_IOC_ENABLE_VERITY.
 */
int
libfsverity_enable(int fd, const struct libfsverity_merkle_tree_params *params);

/**
 * libfsverity_enable_with_sig() - Enable fs-verity on a file, with a signature
 * @fd: read-only file descriptor to the file
 * @params: pointer to the Merkle tree parameters
 * @sig: pointer to the file's signature
 * @sig_size: size of the file's signature in bytes
 *
 * Like libfsverity_enable(), but allows specifying a built-in signature (i.e. a
 * singature created with libfsverity_sign_digest()) to associate with the file.
 * This is only needed if the in-kernel signature verification support is being
 * used; it is not needed if signatures are being verified in userspace.
 *
 * If @sig is NULL and @sig_size is 0, this is the same as libfsverity_enable().
 *
 * Return: See libfsverity_enable().
 */
int
libfsverity_enable_with_sig(int fd,
			    const struct libfsverity_merkle_tree_params *params,
			    const uint8_t *sig, size_t sig_size);

/**
 * libfsverity_find_hash_alg_by_name() - Find hash algorithm by name
 * @name: Pointer to name of hash algorithm
 *
 * Return: The hash algorithm number, or zero if not found.
 */
uint32_t libfsverity_find_hash_alg_by_name(const char *name);

/**
 * libfsverity_get_digest_size() - Get size of digest for a given algorithm
 * @alg_num: Number of hash algorithm
 *
 * Return: size of digest in bytes, or -1 if algorithm is unknown.
 */
int libfsverity_get_digest_size(uint32_t alg_num);

/**
 * libfsverity_get_hash_name() - Get name of hash algorithm by number
 * @alg_num: Number of hash algorithm
 *
 * Return: The name of the hash algorithm, or NULL if algorithm is unknown.
 */
const char *libfsverity_get_hash_name(uint32_t alg_num);

/**
 * libfsverity_set_error_callback() - Set callback to handle error messages
 * @cb: the callback function.
 *
 * If a callback is already set, it is replaced.  @cb may be NULL in order to
 * remove the existing callback.
 */
void libfsverity_set_error_callback(void (*cb)(const char *msg));

#ifdef __cplusplus
}
#endif

#endif /* LIBFSVERITY_H */
