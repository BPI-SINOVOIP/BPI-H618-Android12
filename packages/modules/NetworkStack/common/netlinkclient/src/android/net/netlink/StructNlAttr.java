/*
 * Copyright (C) 2019 The Android Open Source Project
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
 * limitations under the License
 */

package android.net.netlink;

import androidx.annotation.Nullable;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * struct nlattr
 *
 * see: &lt;linux_src&gt;/include/uapi/linux/netlink.h
 *
 * @hide
 */
public class StructNlAttr {
    // Already aligned.
    public static final int NLA_HEADERLEN  = 4;
    public static final int NLA_F_NESTED   = (1 << 15);

    public static short makeNestedType(short type) {
        return (short) (type | NLA_F_NESTED);
    }

    // Return a (length, type) object only, without consuming any bytes in
    // |byteBuffer| and without copying or interpreting any value bytes.
    // This is used for scanning over a packed set of struct nlattr's,
    // looking for instances of a particular type.
    public static StructNlAttr peek(ByteBuffer byteBuffer) {
        if (byteBuffer == null || byteBuffer.remaining() < NLA_HEADERLEN) {
            return null;
        }
        final int baseOffset = byteBuffer.position();

        final StructNlAttr struct = new StructNlAttr();
        final ByteOrder originalOrder = byteBuffer.order();
        byteBuffer.order(ByteOrder.nativeOrder());
        try {
            struct.nla_len = byteBuffer.getShort();
            struct.nla_type = byteBuffer.getShort();
        } finally {
            byteBuffer.order(originalOrder);
        }

        byteBuffer.position(baseOffset);
        if (struct.nla_len < NLA_HEADERLEN) {
            // Malformed.
            return null;
        }
        return struct;
    }

    public static StructNlAttr parse(ByteBuffer byteBuffer) {
        final StructNlAttr struct = peek(byteBuffer);
        if (struct == null || byteBuffer.remaining() < struct.getAlignedLength()) {
            return null;
        }

        final int baseOffset = byteBuffer.position();
        byteBuffer.position(baseOffset + NLA_HEADERLEN);

        int valueLen = ((int) struct.nla_len) & 0xffff;
        valueLen -= NLA_HEADERLEN;
        if (valueLen > 0) {
            struct.nla_value = new byte[valueLen];
            byteBuffer.get(struct.nla_value, 0, valueLen);
            byteBuffer.position(baseOffset + struct.getAlignedLength());
        }
        return struct;
    }

    /**
     * Find next netlink attribute with a given type from {@link ByteBuffer}.
     *
     * @param attrType The given netlink attribute type is requested for.
     * @param byteBuffer The buffer from which to find the netlink attribute.
     * @return the found netlink attribute, or {@code null} if the netlink attribute could not be
     *         found or parsed successfully (for example, if it was truncated).
     */
    @Nullable
    public static StructNlAttr findNextAttrOfType(short attrType,
            @Nullable ByteBuffer byteBuffer) {
        while (byteBuffer != null && byteBuffer.remaining() > 0) {
            final StructNlAttr nlAttr = StructNlAttr.peek(byteBuffer);
            if (nlAttr == null) {
                break;
            }
            if (nlAttr.nla_type == attrType) {
                return StructNlAttr.parse(byteBuffer);
            }
            if (byteBuffer.remaining() < nlAttr.getAlignedLength()) {
                break;
            }
            byteBuffer.position(byteBuffer.position() + nlAttr.getAlignedLength());
        }
        return null;
    }

    public short nla_len = (short) NLA_HEADERLEN;
    public short nla_type;
    public byte[] nla_value;

    public StructNlAttr() {}

    public StructNlAttr(short type, byte value) {
        nla_type = type;
        setValue(new byte[1]);
        nla_value[0] = value;
    }

    public StructNlAttr(short type, short value) {
        this(type, value, ByteOrder.nativeOrder());
    }

    public StructNlAttr(short type, short value, ByteOrder order) {
        nla_type = type;
        setValue(new byte[Short.BYTES]);
        final ByteBuffer buf = getValueAsByteBuffer();
        final ByteOrder originalOrder = buf.order();
        try {
            buf.order(order);
            buf.putShort(value);
        } finally {
            buf.order(originalOrder);
        }
    }

    public StructNlAttr(short type, int value) {
        this(type, value, ByteOrder.nativeOrder());
    }

    public StructNlAttr(short type, int value, ByteOrder order) {
        nla_type = type;
        setValue(new byte[Integer.BYTES]);
        final ByteBuffer buf = getValueAsByteBuffer();
        final ByteOrder originalOrder = buf.order();
        try {
            buf.order(order);
            buf.putInt(value);
        } finally {
            buf.order(originalOrder);
        }
    }

    public StructNlAttr(short type, InetAddress ip) {
        nla_type = type;
        setValue(ip.getAddress());
    }

    public StructNlAttr(short type, StructNlAttr... nested) {
        this();
        nla_type = makeNestedType(type);

        int payloadLength = 0;
        for (StructNlAttr nla : nested) payloadLength += nla.getAlignedLength();
        setValue(new byte[payloadLength]);

        final ByteBuffer buf = getValueAsByteBuffer();
        for (StructNlAttr nla : nested) {
            nla.pack(buf);
        }
    }

    public int getAlignedLength() {
        return NetlinkConstants.alignedLengthOf(nla_len);
    }

    /**
     * Get attribute value as BE16.
     */
    public short getValueAsBe16(short defaultValue) {
        final ByteBuffer byteBuffer = getValueAsByteBuffer();
        if (byteBuffer == null || byteBuffer.remaining() != Short.BYTES) {
            return defaultValue;
        }
        final ByteOrder originalOrder = byteBuffer.order();
        try {
            byteBuffer.order(ByteOrder.BIG_ENDIAN);
            return byteBuffer.getShort();
        } finally {
            byteBuffer.order(originalOrder);
        }
    }

    public int getValueAsBe32(int defaultValue) {
        final ByteBuffer byteBuffer = getValueAsByteBuffer();
        if (byteBuffer == null || byteBuffer.remaining() != Integer.BYTES) {
            return defaultValue;
        }
        final ByteOrder originalOrder = byteBuffer.order();
        try {
            byteBuffer.order(ByteOrder.BIG_ENDIAN);
            return byteBuffer.getInt();
        } finally {
            byteBuffer.order(originalOrder);
        }
    }

    public ByteBuffer getValueAsByteBuffer() {
        if (nla_value == null) { return null; }
        final ByteBuffer byteBuffer = ByteBuffer.wrap(nla_value);
        // By convention, all buffers in this library are in native byte order because netlink is in
        // native byte order. It's the order that is used by NetlinkSocket.recvMessage and the only
        // order accepted by NetlinkMessage.parse.
        byteBuffer.order(ByteOrder.nativeOrder());
        return byteBuffer;
    }

    /**
     * Get attribute value as byte.
     */
    public byte getValueAsByte(byte defaultValue) {
        final ByteBuffer byteBuffer = getValueAsByteBuffer();
        if (byteBuffer == null || byteBuffer.remaining() != Byte.BYTES) {
            return defaultValue;
        }
        return getValueAsByteBuffer().get();
    }

    public int getValueAsInt(int defaultValue) {
        final ByteBuffer byteBuffer = getValueAsByteBuffer();
        if (byteBuffer == null || byteBuffer.remaining() != Integer.BYTES) {
            return defaultValue;
        }
        return getValueAsByteBuffer().getInt();
    }

    public InetAddress getValueAsInetAddress() {
        if (nla_value == null) { return null; }

        try {
            return InetAddress.getByAddress(nla_value);
        } catch (UnknownHostException ignored) {
            return null;
        }
    }

    public void pack(ByteBuffer byteBuffer) {
        final ByteOrder originalOrder = byteBuffer.order();
        final int originalPosition = byteBuffer.position();

        byteBuffer.order(ByteOrder.nativeOrder());
        try {
            byteBuffer.putShort(nla_len);
            byteBuffer.putShort(nla_type);
            if (nla_value != null) byteBuffer.put(nla_value);
        } finally {
            byteBuffer.order(originalOrder);
        }
        byteBuffer.position(originalPosition + getAlignedLength());
    }

    private void setValue(byte[] value) {
        nla_value = value;
        nla_len = (short) (NLA_HEADERLEN + ((nla_value != null) ? nla_value.length : 0));
    }

    @Override
    public String toString() {
        return "StructNlAttr{ "
                + "nla_len{" + nla_len + "}, "
                + "nla_type{" + nla_type + "}, "
                + "nla_value{" + NetlinkConstants.hexify(nla_value) + "}, "
                + "}";
    }
}
