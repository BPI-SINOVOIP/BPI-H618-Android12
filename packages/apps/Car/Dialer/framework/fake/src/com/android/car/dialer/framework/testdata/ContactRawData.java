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

package com.android.car.dialer.framework.testdata;

import androidx.annotation.Nullable;

/**
 * A class represents the contact raw data.
 */
public class ContactRawData extends RawData {

    private String mDisplayName;
    private String mNumber;
    private Integer mNumberType;
    private String mNumberLabel;
    private String mAddress;
    private Integer mStarred;

    /**
     * Sets display name.
     */
    public void setDisplayName(String displayName) {
        mDisplayName = displayName;
    }

    /**
     * Sets the number;
     */
    public void setNumber(String number) {
        mNumber = number;
    }

    /**
     * Sets the number type;
     */
    public void setNumberType(Integer numberType) {
        mNumberType = numberType;
    }

    /**
     * Sets phone number label.
     */
    public void setNumberLabel(String numberLabel) {
        mNumberLabel = numberLabel;
    }

    /**
     * Sets the address.
     */
    public void setAddress(String address) {
        mAddress = address;
    }

    /**
     * If this contact is also a favorite, sets this value to 1. Otherwise, it will be 0 by default.
     */
    public void setStarred(boolean starred) {
        mStarred = starred ? 1 : 0;
    }

    /**
     * Gets the display name;
     */
    public String getDisplayName() {
        return mDisplayName;
    }

    /**
     * Gets the phone number;
     */
    public String getNumber() {
        return mNumber;
    }

    /**
     * Gets the phone number type;
     */
    public Integer getNumberType() {
        return mNumberType;
    }

    /**
     * Gets the phone number label;
     */
    public String getNumberLabel() {
        return mNumberLabel;
    }

    /**
     * Return the address.
     */
    @Nullable
    public String getAddress() {
        return mAddress;
    }

    /**
     * Returns if this contact is starred. O for not starred, other values for starred.
     */
    public Integer getStarred() {
        return mStarred;
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("[id: ");
        builder.append(getId());
        builder.append(", display name: ");
        builder.append(mDisplayName);
        builder.append(", number: ");
        builder.append(mNumber);
        builder.append(", number label: ");
        builder.append(mNumberLabel);
        builder.append(", number type: ");
        builder.append(mNumberType);
        builder.append(", is starred: ");
        builder.append(mStarred);
        builder.append(", address: ");
        builder.append(mAddress);
        builder.append("]");
        return  builder.toString();
    }
}
