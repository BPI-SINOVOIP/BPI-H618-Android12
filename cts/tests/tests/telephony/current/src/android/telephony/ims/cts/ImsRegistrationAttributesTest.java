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

package android.telephony.ims.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.os.Parcel;
import android.telephony.AccessNetworkConstants;
import android.telephony.ims.ImsRegistrationAttributes;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.util.ArraySet;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ImsRegistrationAttributesTest {

    @Test
    public void testRegistrationTypeToTransportAttr() {
        ArraySet<String> featureTags = new ArraySet<>();
        featureTags.add("+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.msg\"");
        featureTags.add("+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.session\"");
        featureTags.add("+g.gsma.callcomposer");

        // IWLAN
        ImsRegistrationAttributes attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN).setFeatureTags(featureTags)
                .build();
        assertEquals(ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN,
                attr.getRegistrationTechnology());
        assertEquals(AccessNetworkConstants.TRANSPORT_TYPE_WLAN,
                attr.getTransportType());
        assertEquals(0, (attr.getAttributeFlags()
                & ImsRegistrationAttributes.ATTR_EPDG_OVER_CELL_INTERNET));
        assertEquals(featureTags, attr.getFeatureTags());

        //LTE
        attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_LTE).build();
        assertEquals(ImsRegistrationImplBase.REGISTRATION_TECH_LTE,
                attr.getRegistrationTechnology());
        assertEquals(AccessNetworkConstants.TRANSPORT_TYPE_WWAN,
                attr.getTransportType());
        assertEquals(0, (attr.getAttributeFlags()
                & ImsRegistrationAttributes.ATTR_EPDG_OVER_CELL_INTERNET));
        assertNotNull(attr.getFeatureTags());
        assertEquals(0, attr.getFeatureTags().size());

        // cross sim
        attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_CROSS_SIM).build();
        assertEquals(ImsRegistrationImplBase.REGISTRATION_TECH_CROSS_SIM,
                attr.getRegistrationTechnology());
        assertEquals(AccessNetworkConstants.TRANSPORT_TYPE_WLAN,
                attr.getTransportType());
        assertEquals(ImsRegistrationAttributes.ATTR_EPDG_OVER_CELL_INTERNET,
                (attr.getAttributeFlags()
                        & ImsRegistrationAttributes.ATTR_EPDG_OVER_CELL_INTERNET));
        assertNotNull(attr.getFeatureTags());
        assertEquals(0, attr.getFeatureTags().size());

        // NR
        attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_NR).build();
        assertEquals(ImsRegistrationImplBase.REGISTRATION_TECH_NR,
                attr.getRegistrationTechnology());
        assertEquals(AccessNetworkConstants.TRANSPORT_TYPE_WWAN,
                attr.getTransportType());
        assertEquals(0,
                (attr.getAttributeFlags()
                        & ImsRegistrationAttributes.ATTR_EPDG_OVER_CELL_INTERNET));
        assertNotNull(attr.getFeatureTags());
        assertEquals(0, attr.getFeatureTags().size());
    }

    @Test
    public void testParcelUnparcel() {

        ArraySet<String> featureTags = new ArraySet<>();
        featureTags.add("+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.msg\"");
        featureTags.add("+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.session\"");
        featureTags.add("+g.gsma.callcomposer");
        ImsRegistrationAttributes attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN).setFeatureTags(featureTags)
                .build();

        Parcel parcel = Parcel.obtain();
        attr.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        ImsRegistrationAttributes unparcelledAttr =
                ImsRegistrationAttributes.CREATOR.createFromParcel(parcel);
        parcel.recycle();

        assertEquals(attr.getRegistrationTechnology(), unparcelledAttr.getRegistrationTechnology());
        assertEquals(attr.getTransportType(), unparcelledAttr.getTransportType());
        assertEquals(attr.getAttributeFlags(), unparcelledAttr.getAttributeFlags());
        assertEquals(attr.getFeatureTags(), unparcelledAttr.getFeatureTags());
    }
}
