/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.virt.fs;

/** {@hide} */
interface IVirtFdService {
    /** Error when the requesting FD is unknown. */
    const int ERROR_UNKNOWN_FD = 1;

    /**
     * Error when I/O fails. This can happen when actual I/O error happens to the backing file,
     * when the given offset or size are invalid, or any problems that can fail a read/write
     * request.
     */
    const int ERROR_IO = 2;

    /** Maximum content size that the service allows the client to request. */
    const int MAX_REQUESTING_DATA = 16384;

    /**
     * Returns the content of the given file ID, from the offset, for the amount of requested size
     * or until EOF.
     */
    byte[] readFile(int id, long offset, int size);

    /**
     * Returns the content of fs-verity compatible Merkle tree of the given file ID, from the
     * offset, for the amount of requested size or until EOF.
     */
    byte[] readFsverityMerkleTree(int id, long offset, int size);

    /** Returns the fs-verity signature of the given file ID. */
    byte[] readFsveritySignature(int id);

    /**
     * Writes the buffer to the given file ID from the file's offset. Returns the number of bytes
     * written.
     */
    int writeFile(int id, in byte[] buf, long offset);

    /** Resizes the file backed by the given file ID to the new size. */
    void resize(int id, long size);
}
