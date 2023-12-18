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

package android.telephony.cts;

import static com.google.common.truth.Truth.assertThat;

import android.os.Parcel;
import android.telephony.data.TrafficDescriptor;

import org.junit.Test;

public class TrafficDescriptorTest {
    private static final String DNN = "DNN";
    private static final byte[] OS_APP_ID = {1, 2, 3, 4};

    @Test
    public void testConstructorAndGetters() {
        TrafficDescriptor td = new TrafficDescriptor(DNN, OS_APP_ID);
        assertThat(td.getDataNetworkName()).isEqualTo(DNN);
        assertThat(td.getOsAppId()).isEqualTo(OS_APP_ID);
    }

    @Test
    public void testEquals() {
        TrafficDescriptor td = new TrafficDescriptor(DNN, OS_APP_ID);
        TrafficDescriptor equalsTd = new TrafficDescriptor(DNN, OS_APP_ID);
        assertThat(td).isEqualTo(equalsTd);
    }

    @Test
    public void testNotEquals() {
        TrafficDescriptor td = new TrafficDescriptor(DNN, OS_APP_ID);
        byte[] notOsAppId = {5, 6, 7, 8};
        TrafficDescriptor notEqualsTd = new TrafficDescriptor("NOT_DNN", notOsAppId);
        assertThat(td).isNotEqualTo(notEqualsTd);
        assertThat(td).isNotEqualTo(null);
    }

    @Test
    public void testParcel() {
        TrafficDescriptor td = new TrafficDescriptor(DNN, OS_APP_ID);

        Parcel parcel = Parcel.obtain();
        td.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);

        TrafficDescriptor parcelTd = TrafficDescriptor.CREATOR.createFromParcel(parcel);
        assertThat(td).isEqualTo(parcelTd);
    }

    @Test
    public void testBuilder() {
        TrafficDescriptor td = new TrafficDescriptor.Builder()
                .setDataNetworkName(DNN)
                .setOsAppId(OS_APP_ID)
                .build();
        assertThat(td.getDataNetworkName()).isEqualTo(DNN);
        assertThat(td.getOsAppId()).isEqualTo(OS_APP_ID);
    }
}
