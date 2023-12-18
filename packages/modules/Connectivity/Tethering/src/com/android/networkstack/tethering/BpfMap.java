/*
 * Copyright (C) 2020 The Android Open Source Project
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
package com.android.networkstack.tethering;

import static android.system.OsConstants.EEXIST;
import static android.system.OsConstants.ENOENT;

import android.system.ErrnoException;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.internal.annotations.VisibleForTesting;
import com.android.net.module.util.Struct;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.function.BiConsumer;

/**
 * BpfMap is a key -> value mapping structure that is designed to maintained the bpf map entries.
 * This is a wrapper class of in-kernel data structure. The in-kernel data can be read/written by
 * passing syscalls with map file descriptor.
 *
 * @param <K> the key of the map.
 * @param <V> the value of the map.
 */
public class BpfMap<K extends Struct, V extends Struct> implements AutoCloseable {
    static {
        System.loadLibrary("tetherutilsjni");
    }

    // Following definitions from kernel include/uapi/linux/bpf.h
    public static final int BPF_F_RDWR = 0;
    public static final int BPF_F_RDONLY = 1 << 3;
    public static final int BPF_F_WRONLY = 1 << 4;

    public static final int BPF_MAP_TYPE_HASH = 1;

    private static final int BPF_F_NO_PREALLOC = 1;

    private static final int BPF_ANY = 0;
    private static final int BPF_NOEXIST = 1;
    private static final int BPF_EXIST = 2;

    private final int mMapFd;
    private final Class<K> mKeyClass;
    private final Class<V> mValueClass;
    private final int mKeySize;
    private final int mValueSize;

    /**
     * Create a BpfMap map wrapper with "path" of filesystem.
     *
     * @param flag the access mode, one of BPF_F_RDWR, BPF_F_RDONLY, or BPF_F_WRONLY.
     * @throws ErrnoException if the BPF map associated with {@code path} cannot be retrieved.
     * @throws NullPointerException if {@code path} is null.
     */
    public BpfMap(@NonNull final String path, final int flag, final Class<K> key,
            final Class<V> value) throws ErrnoException, NullPointerException {
        mMapFd = bpfFdGet(path, flag);

        mKeyClass = key;
        mValueClass = value;
        mKeySize = Struct.getSize(key);
        mValueSize = Struct.getSize(value);
    }

     /**
     * Constructor for testing only.
     * The derived class implements an internal mocked map. It need to implement all functions
     * which are related with the native BPF map because the BPF map handler is not initialized.
     * See BpfCoordinatorTest#TestBpfMap.
     */
    @VisibleForTesting
    protected BpfMap(final Class<K> key, final Class<V> value) {
        mMapFd = -1;
        mKeyClass = key;
        mValueClass = value;
        mKeySize = Struct.getSize(key);
        mValueSize = Struct.getSize(value);
    }

    /**
     * Update an existing or create a new key -> value entry in an eBbpf map.
     * (use insertOrReplaceEntry() if you need to know whether insert or replace happened)
     */
    public void updateEntry(K key, V value) throws ErrnoException {
        writeToMapEntry(mMapFd, key.writeToBytes(), value.writeToBytes(), BPF_ANY);
    }

    /**
     * If the key does not exist in the map, insert key -> value entry into eBpf map.
     * Otherwise IllegalStateException will be thrown.
     */
    public void insertEntry(K key, V value)
            throws ErrnoException, IllegalStateException {
        try {
            writeToMapEntry(mMapFd, key.writeToBytes(), value.writeToBytes(), BPF_NOEXIST);
        } catch (ErrnoException e) {
            if (e.errno == EEXIST) throw new IllegalStateException(key + " already exists");

            throw e;
        }
    }

    /**
     * If the key already exists in the map, replace its value. Otherwise NoSuchElementException
     * will be thrown.
     */
    public void replaceEntry(K key, V value)
            throws ErrnoException, NoSuchElementException {
        try {
            writeToMapEntry(mMapFd, key.writeToBytes(), value.writeToBytes(), BPF_EXIST);
        } catch (ErrnoException e) {
            if (e.errno == ENOENT) throw new NoSuchElementException(key + " not found");

            throw e;
        }
    }

    /**
     * Update an existing or create a new key -> value entry in an eBbpf map.
     * Returns true if inserted, false if replaced.
     * (use updateEntry() if you don't care whether insert or replace happened)
     * Note: see inline comment below if running concurrently with delete operations.
     */
    public boolean insertOrReplaceEntry(K key, V value)
            throws ErrnoException {
        try {
            writeToMapEntry(mMapFd, key.writeToBytes(), value.writeToBytes(), BPF_NOEXIST);
            return true;   /* insert succeeded */
        } catch (ErrnoException e) {
            if (e.errno != EEXIST) throw e;
        }
        try {
            writeToMapEntry(mMapFd, key.writeToBytes(), value.writeToBytes(), BPF_EXIST);
            return false;   /* replace succeeded */
        } catch (ErrnoException e) {
            if (e.errno != ENOENT) throw e;
        }
        /* If we reach here somebody deleted after our insert attempt and before our replace:
         * this implies a race happened.  The kernel bpf delete interface only takes a key,
         * and not the value, so we can safely pretend the replace actually succeeded and
         * was immediately followed by the other thread's delete, since the delete cannot
         * observe the potential change to the value.
         */
        return false;   /* pretend replace succeeded */
    }

    /** Remove existing key from eBpf map. Return false if map was not modified. */
    public boolean deleteEntry(K key) throws ErrnoException {
        return deleteMapEntry(mMapFd, key.writeToBytes());
    }

    /** Returns {@code true} if this map contains no elements. */
    public boolean isEmpty() throws ErrnoException {
        return getFirstKey() == null;
    }

    private K getNextKeyInternal(@Nullable K key) throws ErrnoException {
        final byte[] rawKey = getNextRawKey(
                key == null ? null : key.writeToBytes());
        if (rawKey == null) return null;

        final ByteBuffer buffer = ByteBuffer.wrap(rawKey);
        buffer.order(ByteOrder.nativeOrder());
        return Struct.parse(mKeyClass, buffer);
    }

    /**
     * Get the next key of the passed-in key. If the passed-in key is not found, return the first
     * key. If the passed-in key is the last one, return null.
     *
     * TODO: consider allowing null passed-in key.
     */
    public K getNextKey(@NonNull K key) throws ErrnoException {
        Objects.requireNonNull(key);
        return getNextKeyInternal(key);
    }

    private byte[] getNextRawKey(@Nullable final byte[] key) throws ErrnoException {
        byte[] nextKey = new byte[mKeySize];
        if (getNextMapKey(mMapFd, key, nextKey)) return nextKey;

        return null;
    }

    /** Get the first key of eBpf map. */
    public K getFirstKey() throws ErrnoException {
        return getNextKeyInternal(null);
    }

    /** Check whether a key exists in the map. */
    public boolean containsKey(@NonNull K key) throws ErrnoException {
        Objects.requireNonNull(key);

        final byte[] rawValue = getRawValue(key.writeToBytes());
        return rawValue != null;
    }

    /** Retrieve a value from the map. Return null if there is no such key. */
    public V getValue(@NonNull K key) throws ErrnoException {
        Objects.requireNonNull(key);
        final byte[] rawValue = getRawValue(key.writeToBytes());

        if (rawValue == null) return null;

        final ByteBuffer buffer = ByteBuffer.wrap(rawValue);
        buffer.order(ByteOrder.nativeOrder());
        return Struct.parse(mValueClass, buffer);
    }

    private byte[] getRawValue(final byte[] key) throws ErrnoException {
        byte[] value = new byte[mValueSize];
        if (findMapEntry(mMapFd, key, value)) return value;

        return null;
    }

    /**
     * Iterate through the map and handle each key -> value retrieved base on the given BiConsumer.
     * The given BiConsumer may to delete the passed-in entry, but is not allowed to perform any
     * other structural modifications to the map, such as adding entries or deleting other entries.
     * Otherwise, iteration will result in undefined behaviour.
     */
    public void forEach(BiConsumer<K, V> action) throws ErrnoException {
        @Nullable K nextKey = getFirstKey();

        while (nextKey != null) {
            @NonNull final K curKey = nextKey;
            @NonNull final V value = getValue(curKey);

            nextKey = getNextKey(curKey);
            action.accept(curKey, value);
        }
    }

    @Override
    public void close() throws ErrnoException {
        closeMap(mMapFd);
    }

    /**
     * Clears the map. The map may already be empty.
     *
     * @throws ErrnoException if the map is already closed, if an error occurred during iteration,
     *                        or if a non-ENOENT error occurred when deleting a key.
     */
    public void clear() throws ErrnoException {
        K key = getFirstKey();
        while (key != null) {
            deleteEntry(key);  // ignores ENOENT.
            key = getFirstKey();
        }
    }

    private static native int closeMap(int fd) throws ErrnoException;

    private native int bpfFdGet(String path, int mode) throws ErrnoException, NullPointerException;

    private native void writeToMapEntry(int fd, byte[] key, byte[] value, int flags)
            throws ErrnoException;

    private native boolean deleteMapEntry(int fd, byte[] key) throws ErrnoException;

    // If key is found, the operation returns true and the nextKey would reference to the next
    // element.  If key is not found, the operation returns true and the nextKey would reference to
    // the first element.  If key is the last element, false is returned.
    private native boolean getNextMapKey(int fd, byte[] key, byte[] nextKey) throws ErrnoException;

    private native boolean findMapEntry(int fd, byte[] key, byte[] value) throws ErrnoException;
}
