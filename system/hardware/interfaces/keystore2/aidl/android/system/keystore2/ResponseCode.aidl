/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system.keystore2;

/** @hide */
@VintfStability
@Backing(type="int")
enum ResponseCode {
    /* 1 Reserved - formerly NO_ERROR */
    /**
     * TODO Determine exact semantic of Locked and Uninitialized.
     */
    LOCKED = 2,
    UNINITIALIZED = 3,
    /**
     * Any unexpected Error such as IO or communication errors.
     * Implementations should log details to logcat.
     */
    SYSTEM_ERROR = 4,
    /* 5 Reserved - formerly "protocol error" was never used */
    /**
     * Indicates that the caller does not have the permissions for the attempted request.
     */
    PERMISSION_DENIED = 6,
    /**
     * Indicates that the requested key does not exist.
     */
    KEY_NOT_FOUND = 7,
    /**
     * Indicates a data corruption in the Keystore 2.0 database.
     */
    VALUE_CORRUPTED = 8,
    /*
     * 9 Reserved - formerly "undefined action" was never used
     * 10 Reserved - formerly WrongPassword
     * 11 - 13 Reserved - formerly password retry count indicators: obsolete
     *
     * 14 Reserved - formerly SIGNATURE_INVALID: Keystore does not perform public key operations
     *               any more
     *
     *
     * 15 Reserved - Formerly OP_AUTH_NEEDED. This is now indicated by the optional
     *               `OperationChallenge` returned by `IKeystoreSecurityLevel::create`.
     *
     * 16 Reserved
     */
    KEY_PERMANENTLY_INVALIDATED = 17,
    /**
     * May be returned by `IKeystoreSecurityLevel.create` if all Keymint operation slots
     * are currently in use and none can be pruned.
     */
    BACKEND_BUSY = 18,
    /**
     * This is a logical error on the caller's side. They are trying to advance an
     * operation, e.g., by calling `update`, that is currently processing an `update`
     * or `finish` request.
     */
    OPERATION_BUSY = 19,
    /**
     * Indicates that an invalid argument was passed to an API call.
     */
    INVALID_ARGUMENT = 20,
    /**
     * Indicates that too much data was sent in a single transaction.
     * The binder kernel mechanism cannot really diagnose this condition unambiguously.
     * So in order to force benign clients into reasonable limits, we limit the maximum
     * amount of data that we except in a single transaction to 32KiB.
     */
    TOO_MUCH_DATA = 21,

    /**
     * Indicates that the attestation key pool does not have any signed attestation keys
     * available. This can be thrown during attempts to generate a key.
     */
    OUT_OF_KEYS = 22,
}
