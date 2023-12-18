/*
 * Copyright (C) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.aidl.tests;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;
import static org.hamcrest.core.Is.is;
import static org.hamcrest.core.IsNull.nullValue;
import static org.hamcrest.core.IsNull.notNullValue;

import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import java.util.ArrayList;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runners.JUnit4;
import org.junit.runner.RunWith;

@RunWith(JUnit4.class)
public class NullableTests {
    private ITestService mService;
    private ICppJavaTests mCppJavaTests;

    @Before
    public void setUp() throws RemoteException {
        IBinder binder = ServiceManager.waitForService(ITestService.class.getName());
        assertNotNull(binder);
        mService = ITestService.Stub.asInterface(binder);
        assertNotNull(mService);

        IBinder binder2 = mService.GetCppJavaTests();
        if (binder2 != null) {
            mCppJavaTests = ICppJavaTests.Stub.asInterface(binder2);
        }
    }

    @Test
    public void testExpectNpeWithNullString() throws RemoteException {
        try {
            String response = mService.RepeatString(null);
        } catch (NullPointerException ex) {
            return;
        }
        if (mService.getBackendType() != BackendType.JAVA) {
          fail("NullPointerException was expected, but wasn't thrown");
        }
    }

    @Test
    public void testExpectNpeWithNullBinder() throws RemoteException {
        try {
            mService.TakesAnIBinder(null);
        } catch (NullPointerException ex) {
            return;
        }

        if (mService.getBackendType() != BackendType.JAVA) {
          fail("NullPointerException was expected, but wasn't thrown");
        }
    }

    @Test
    public void testExpectNpeWithNullBinderList() throws RemoteException {
        assumeTrue(mCppJavaTests != null);

        List<IBinder> listWithNulls = new ArrayList<IBinder>();
        listWithNulls.add(null);
        try {
            mCppJavaTests.TakesAnIBinderList(listWithNulls);
        } catch (NullPointerException ex) {
            return;
        }
        if (mService.getBackendType() != BackendType.JAVA) {
          fail("NullPointerException was expected, but wasn't thrown");
        }
    }

    @Test
    public void testNullableIInterface() throws RemoteException {
        INamedCallback callback  = mService.GetCallback(false);
        assertThat(callback, is(notNullValue()));

        callback = mService.GetCallback(true);
        assertThat(callback, is(nullValue()));
    }
}
