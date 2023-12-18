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

package android.net.ipsec.ike.ike3gpp;

import android.annotation.NonNull;
import android.annotation.SuppressLint;
import android.annotation.SystemApi;

import java.util.Objects;

/**
 * Ike3gppParams is used to configure 3GPP-specific parameters to be used during an IKE Session.
 *
 * @see 3GPP ETSI TS 24.302: Access to the 3GPP Evolved Packet Core (EPC) via non-3GPP access
 *     networks
 * @hide
 */
@SystemApi
public final class Ike3gppParams {
    /** If the PDU Session ID is not set, it will be reported as 0. */
    // NoByteOrShort: using byte to be consistent with the PDU Session ID specification
    @SuppressLint("NoByteOrShort")
    public static final byte PDU_SESSION_ID_UNSET = 0;

    private final byte mPduSessionId;

    private Ike3gppParams(byte pduSessionId) {
        mPduSessionId = pduSessionId;
    }

    /**
     * Retrieves the PDU Session ID for this Ike3gppParams.
     *
     * <p>If the PDU Session ID was not set and this method is called, {@link PDU_SESSION_ID_UNSET}
     * will be returned.
     */
    // NoByteOrShort: using byte to be consistent with the PDU Session ID specification
    @SuppressLint("NoByteOrShort")
    public byte getPduSessionId() {
        return mPduSessionId;
    }

    /**
     * Returns true if the PDU Session ID is set for this instance.
     *
     * @hide
     */
    public boolean hasPduSessionId() {
        return mPduSessionId != PDU_SESSION_ID_UNSET;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mPduSessionId);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Ike3gppParams)) {
            return false;
        }

        return mPduSessionId == ((Ike3gppParams) o).mPduSessionId;
    }

    @Override
    public String toString() {
        return new StringBuilder()
                .append("Ike3gppParams={ ")
                .append("pduSessionId=")
                .append(String.format("%02X", mPduSessionId))
                .append(" }")
                .toString();
    }

    /** This class can be used to incrementally construct an {@link Ike3gppParams}. */
    public static final class Builder {
        private byte mPduSessionId = PDU_SESSION_ID_UNSET;

        /**
         * Sets the PDU Session ID to be used for the 3GPP N1_MODE_CAPABILITY payload.
         *
         * <p>Setting the PDU Session ID will configure the IKE Session to notify the server that it
         * supports N1_MODE.
         *
         * <p>{@link PDU_SESSION_ID_UNSET} will clear the previously-set PDU Session ID.
         *
         * @see TS 24.007 Section 11.2.3.1b for the definition of PDU Session ID encoding
         * @see TS 24.302 Section 7.2.2 for context on PDU Session ID usage
         * @see TS 24.302 Section 8.2.9.15 for a description of the N1_MODE_CAPABILITY payload
         * @param pduSessionId the PDU Session ID value to be used in this IKE Session
         * @return Builder this, to facilitate chaining
         */
        // NoByteOrShort: using byte to be consistent with the PDU Session ID specification
        @NonNull
        public Builder setPduSessionId(@SuppressLint("NoByteOrShort") byte pduSessionId) {
            mPduSessionId = pduSessionId;
            return this;
        }

        /**
         * Validates and builds the {@link Ike3gppParams}.
         *
         * @return Ike3gppParams the validated Ike3gppParams
         */
        @NonNull
        public Ike3gppParams build() {
            return new Ike3gppParams(mPduSessionId);
        }
    }
}
