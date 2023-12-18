/*
 * Copyright 2020 The Android Open Source Project
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

package com.google.android.iwlan;

import android.net.ipsec.ike.exceptions.IkeException;
import android.net.ipsec.ike.exceptions.IkeInternalException;
import android.net.ipsec.ike.exceptions.IkeProtocolException;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class IwlanError {

    // error types
    public static final int NO_ERROR = 0;

    // IKE lib related
    public static final int IKE_PROTOCOL_EXCEPTION = 1;
    public static final int IKE_INTERNAL_IO_EXCEPTION = 2;
    public static final int IKE_GENERIC_EXCEPTION = 3; // catch all

    // Known internal types
    public static final int EPDG_SELECTOR_SERVER_SELECTION_FAILED = 4;
    public static final int TUNNEL_TRANSFORM_FAILED = 5;

    // Catch all exception
    public static final int UNKNOWN_EXCEPTION = 6; // catch all

    @IntDef({
        NO_ERROR,
        IKE_PROTOCOL_EXCEPTION,
        IKE_INTERNAL_IO_EXCEPTION,
        IKE_GENERIC_EXCEPTION,
        EPDG_SELECTOR_SERVER_SELECTION_FAILED,
        TUNNEL_TRANSFORM_FAILED,
        UNKNOWN_EXCEPTION
    })
    @interface IwlanErrorType {};

    private static final Map<Integer, String> sErrorTypeStrings =
            new ConcurrentHashMap<>() {
                {
                    put(NO_ERROR, "IWLAN_NO_ERROR");
                    put(IKE_PROTOCOL_EXCEPTION, "IWLAN_IKE_PROTOCOL_EXCEPTION");
                    put(IKE_INTERNAL_IO_EXCEPTION, "IWLAN_IKE_INTERNAL_IO_EXCEPTION");
                    put(IKE_GENERIC_EXCEPTION, "IWLAN_IKE_GENERIC_EXCEPTION");
                    put(
                            EPDG_SELECTOR_SERVER_SELECTION_FAILED,
                            "IWLAN_EPDG_SELECTOR_SERVER_SELECTION_FAILED");
                    put(TUNNEL_TRANSFORM_FAILED, "IWLAN_TUNNEL_TRANSFORM_FAILED");
                    put(UNKNOWN_EXCEPTION, "IWLAN_UNKNOWN_EXCEPTION");
                }
            };

    private int mErrorType;
    private Exception mException;

    public IwlanError(@IwlanErrorType int err) {
        mErrorType = err;
    }

    /**
     * Sets the IwlanError based on the Exception: 1. IkeException is base the class for all IKE
     * exception ErrorType: IKE_GENERIC_EXCEPTION. 2. IkeProtocolException is for specific protocol
     * errors (like IKE notify error codes) ErrorType: IKE_PROTOCOL_EXCEPTION 3.
     * IkeInternalException is just a wrapper for various exeptions that IKE lib may encounter
     * ErrorType: IKE_INTERNAL_IO_EXCEPTION if the Exception is instance of IOException ErrorType:
     * IKE_GENERIC_EXCEPTION for all the other.
     */
    public IwlanError(@NonNull Exception exception) {
        // resolve into specific types if possible
        if (exception instanceof IkeProtocolException) {
            IwlanErrorIkeProtocolException((IkeProtocolException) exception);
        } else if (exception instanceof IkeInternalException) {
            IwlanErrorIkeInternalException((IkeInternalException) exception);
        } else if (exception instanceof IkeException) {
            mErrorType = IKE_GENERIC_EXCEPTION;
            mException = exception;
        } else {
            mErrorType = UNKNOWN_EXCEPTION;
            mException = exception;
        }
    }

    private void IwlanErrorIkeProtocolException(@NonNull IkeProtocolException exception) {
        mErrorType = IKE_PROTOCOL_EXCEPTION;
        mException = exception;
    }

    private void IwlanErrorIkeInternalException(@NonNull IkeInternalException exception) {
        if (exception.getCause() instanceof IOException) {
            mErrorType = IKE_INTERNAL_IO_EXCEPTION;
        } else {
            mErrorType = IKE_GENERIC_EXCEPTION;
        }
        mException = exception;
    }

    public @IwlanErrorType int getErrorType() {
        return mErrorType;
    }

    public Exception getException() {
        return mException;
    }

    private static @NonNull String getErrorTypeString(@IwlanErrorType int error) {
        String s = sErrorTypeStrings.get(error);
        return (s == null ? "IWLAN_UNKNOWN_ERROR_TYPE" : s);
    }

    @Override
    public String toString() {
        return ("TYPE: " + getErrorTypeString(mErrorType) + " " + errorDetailsString());
    }

    private String errorDetailsString() {
        StringBuilder sb = new StringBuilder();

        if (mException == null) {
            return "";
        }

        switch (mErrorType) {
            case IKE_GENERIC_EXCEPTION:
                sb.append("MSG: " + mException.getMessage() + "\n CAUSE: ");
                sb.append(mException.getCause());
                break;
            case UNKNOWN_EXCEPTION:
                sb.append(mException.toString());
                break;
            case IKE_PROTOCOL_EXCEPTION:
                sb.append("ERR: " + ((IkeProtocolException) mException).getErrorType() + "\nDATA:");
                for (byte b : ((IkeProtocolException) mException).getErrorData()) {
                    sb.append(String.format("%02x ", b));
                }
                break;
            default:
                sb.append("-No Details-");
        }
        return sb.toString();
    }

    /**
     * Returns the error of the String. String that matches the name of the Error
     *
     * @param errorType string form of errorType
     * @return IwlanErrorType
     */
    public static int getErrorType(String errorType) {
        int ret = IwlanError.UNKNOWN_EXCEPTION;

        // TODO: Add representation for Global error
        switch (errorType) {
            case "IKE_PROTOCOL_EXCEPTION":
                ret = IwlanError.IKE_PROTOCOL_EXCEPTION;
                break;
            case "IKE_INTERNAL_IO_EXCEPTION":
                ret = IwlanError.IKE_INTERNAL_IO_EXCEPTION;
                break;
            case "IKE_GENERIC_EXCEPTION":
                ret = IwlanError.IKE_GENERIC_EXCEPTION;
                break;
            case "EPDG_SELECTOR_SERVER_SELECTION_FAILED":
                ret = IwlanError.EPDG_SELECTOR_SERVER_SELECTION_FAILED;
                break;
            case "TUNNEL_TRANSFORM_FAILED":
                ret = IwlanError.TUNNEL_TRANSFORM_FAILED;
                break;
        }
        return ret;
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof IwlanError)) {
            return false;
        }
        IwlanError error = (IwlanError) o;
        boolean ret = false;
        if (mErrorType == error.getErrorType()) {
            if (mException != null && error.getException() != null) {
                ret = mException.getClass().equals(error.getException().getClass());
                if (ret && mException instanceof IkeProtocolException) {
                    ret =
                            (((IkeProtocolException) mException).getErrorType()
                                    == ((IkeProtocolException) error.getException())
                                            .getErrorType());
                }
            } else if (mException == null && error.getException() == null) {
                ret = true;
            }
        }
        return ret;
    }
}
;
