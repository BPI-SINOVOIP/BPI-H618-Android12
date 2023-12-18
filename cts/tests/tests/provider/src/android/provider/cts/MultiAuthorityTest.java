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

package android.provider.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import com.android.compatibility.common.util.SystemUtil;

import android.app.ActivityManager;
import android.app.Instrumentation;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.provider.SearchIndexableResource;

import androidx.test.filters.SmallTest;
import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class MultiAuthorityTest {

    private static final String PROVIDER1 = "android.provider.apps.cts.multi1";
    private static final String PROVIDER2 = "android.provider.apps.cts.multi2";
    private static final String PROVIDER_NONEXISTENT = "android.provider.apps.cts.multi3";

    private Context mContext;
    private ActivityManager mAm;
    private ContentResolver mContentResolver;

    @Before
    public void setUp() {
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mContentResolver = mContext.getContentResolver();
        mAm = mContext.getSystemService(ActivityManager.class);
    }

    @After
    public void shutDown() {
        killProviderProcess();
    }

    private void killProviderProcess() {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            mAm.forceStopPackage("android.provider.apps.cts.multiauthority");
        });
    }

    @Test
    public void testAuthority1() {
        ContentProviderClient cpc = mContentResolver.acquireContentProviderClient(PROVIDER1);
        assertNotNull("Couldn't acquire provider", cpc);
        cpc.release();
    }

    @Test
    public void testAuthorityBoth() {
        ContentProviderClient cpc = mContentResolver.acquireContentProviderClient(PROVIDER2);
        assertNotNull("Couldn't acquire provider 2", cpc);
        cpc.release();
        cpc = mContentResolver.acquireContentProviderClient(PROVIDER1);
        assertNotNull("Couldn't acquire provider 1", cpc);
        cpc.release();
    }

    @Test
    public void testAuthorityNonExistent() {
        ContentProviderClient cpc =
                mContentResolver.acquireContentProviderClient(PROVIDER_NONEXISTENT);
        assertNull("Could acquire non-existent provider 3!", cpc);
    }
}
