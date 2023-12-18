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

package android.car.telemetry;

import android.annotation.NonNull;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * A parcelable that wraps around the Manifest name and version.
 *
 * @hide
 */
public final class ManifestKey implements Parcelable {

    @NonNull
    private String mName;
    private int mVersion;

    @NonNull
    public String getName() {
        return mName;
    }

    public int getVersion() {
        return mVersion;
    }

    @Override
    public void writeToParcel(@NonNull Parcel out, int flags) {
        out.writeString(mName);
        out.writeInt(mVersion);
    }

    private ManifestKey(Parcel in) {
        mName = in.readString();
        mVersion = in.readInt();
    }

    public ManifestKey(@NonNull String name, int version) {
        mName = name;
        mVersion = version;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final @NonNull Parcelable.Creator<ManifestKey> CREATOR =
            new Parcelable.Creator<ManifestKey>() {
                @Override
                public ManifestKey createFromParcel(Parcel in) {
                    return new ManifestKey(in);
                }

                @Override
                public ManifestKey[] newArray(int size) {
                    return new ManifestKey[size];
                }
            };
}
