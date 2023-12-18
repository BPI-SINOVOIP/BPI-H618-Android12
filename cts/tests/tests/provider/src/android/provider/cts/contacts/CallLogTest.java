/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.provider.cts.contacts;

import static org.junit.Assert.assertArrayEquals;

import android.Manifest;
import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.OutcomeReceiver;
import android.os.ParcelFileDescriptor;
import android.os.UserHandle;
import android.provider.CallLog;
import android.provider.cts.R;
import android.test.InstrumentationTestCase;
import android.util.Pair;

import androidx.annotation.NonNull;

import com.android.compatibility.common.util.ShellIdentityUtils;
import com.android.compatibility.common.util.ShellUtils;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class CallLogTest extends InstrumentationTestCase {

    private static final String TEST_NUMBER = "5625698388";
    private static final long CONTENT_RESOLVER_TIMEOUT_MS = 5000;

    public void testGetLastOutgoingCall() {
        // Clear call log and ensure there are no outgoing calls
        Context context = getInstrumentation().getContext();
        ContentResolver resolver = context.getContentResolver();
        resolver.delete(CallLog.Calls.CONTENT_URI, null, null);

        waitUntilConditionIsTrueOrTimeout(
                new Condition() {
                    @Override
                    public Object expected() {
                        return "";
                    }

                    @Override
                    public Object actual() {
                        return CallLog.Calls.getLastOutgoingCall(context);
                    }
                },
                CONTENT_RESOLVER_TIMEOUT_MS,
                "getLastOutgoingCall did not return empty after CallLog was cleared"
        );

        // Add a single call and verify it returns as last outgoing call
        ContentValues values = new ContentValues();
        values.put(CallLog.Calls.NUMBER, TEST_NUMBER);
        values.put(CallLog.Calls.TYPE, Integer.valueOf(CallLog.Calls.OUTGOING_TYPE));
        values.put(CallLog.Calls.DATE, Long.valueOf(0 /*start time*/));
        values.put(CallLog.Calls.DURATION, Long.valueOf(5 /*call duration*/));

        resolver.insert(CallLog.Calls.CONTENT_URI, values);

        waitUntilConditionIsTrueOrTimeout(
                new Condition() {
                    @Override
                    public Object expected() {
                        return TEST_NUMBER;
                    }

                    @Override
                    public Object actual() {
                        return CallLog.Calls.getLastOutgoingCall(context);
                    }
                },
                CONTENT_RESOLVER_TIMEOUT_MS,
                "getLastOutgoingCall did not return " + TEST_NUMBER + " as expected"
        );
    }

    public void testLocationStorageAndRetrieval() {
        Context context = getInstrumentation().getContext();

        if (!context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            // This is tied to default-dialer, so don't test if the device doesn't have telephony.
            return;
        }

        UserHandle currentUser = UserHandle.of(
                ShellIdentityUtils.invokeStaticMethodWithShellPermissions(
                        () -> ActivityManager.getCurrentUser()));
        CallLog.AddCallParams.AddCallParametersBuilder builder =
                new CallLog.AddCallParams.AddCallParametersBuilder();
        builder.setAddForAllUsers(false);
        builder.setUserToBeInsertedTo(currentUser);
        // Some random spot in the North Atlantic
        double lat = 24.877323;
        double lon = -68.952545;
        builder.setLatitude(lat);
        builder.setLongitude(lon);
        ShellUtils.runShellCommand("telecom set-default-dialer %s",
                getInstrumentation().getContext().getPackageName());

        try {
            Uri uri;
            getInstrumentation().getUiAutomation()
                    .adoptShellPermissionIdentity(Manifest.permission.INTERACT_ACROSS_USERS,
                            Manifest.permission.READ_VOICEMAIL);
            try {
                uri = CallLog.Calls.addCall(context, builder.build());
            } finally {
                getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
            }
            assertNotNull(uri);

            Cursor cursor = context.getContentResolver().query(
                    uri, new String[]{CallLog.Calls.LOCATION}, null, null);
            assertEquals(1, cursor.getCount());
            cursor.moveToFirst();
            String locationUriString = cursor.getString(
                    cursor.getColumnIndex(CallLog.Calls.LOCATION));
            assertNotNull(locationUriString);

            Uri locationUri = Uri.parse(locationUriString);
            Cursor locationCursor = context.getContentResolver().query(locationUri,
                    new String[]{CallLog.Locations.LATITUDE, CallLog.Locations.LONGITUDE}, null,
                    null);
            assertEquals(1, locationCursor.getCount());
            locationCursor.moveToFirst();
            double storedLat = locationCursor.getDouble(
                    locationCursor.getColumnIndex(CallLog.Locations.LATITUDE));
            double storedLon = locationCursor.getDouble(
                    locationCursor.getColumnIndex(CallLog.Locations.LONGITUDE));
            assertEquals(lat, storedLat);
            assertEquals(lon, storedLon);
        } finally {
            ShellUtils.runShellCommand("telecom set-default-dialer default");
        }
    }

    public void testCallComposerImageStorage() throws Exception {
        Context context = getInstrumentation().getContext();
        byte[] expected = readResourceDrawable(context, R.drawable.testimage);

        CompletableFuture<Pair<Uri, CallLog.CallComposerLoggingException>> resultFuture =
                new CompletableFuture<>();
        Pair<Uri, CallLog.CallComposerLoggingException> result;
        try (InputStream inputStream =
                     context.getResources().openRawResource(R.drawable.testimage)) {
            CallLog.storeCallComposerPicture(
                    context.createContextAsUser(android.os.Process.myUserHandle(), 0),
                    inputStream,
                    Executors.newSingleThreadExecutor(),
                    new OutcomeReceiver<Uri, CallLog.CallComposerLoggingException>() {
                        @Override
                        public void onResult(@NonNull Uri result) {
                            resultFuture.complete(Pair.create(result, null));
                        }

                        @Override
                        public void onError(CallLog.CallComposerLoggingException error) {
                            resultFuture.complete(Pair.create(null, error));
                        }
                    });
           result = resultFuture.get(CONTENT_RESOLVER_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        }
        if (result.second != null) {
            fail("Got error " + result.second.getErrorCode() + " when storing image");
        }
        Uri imageLocation = result.first;

        try (ParcelFileDescriptor pfd =
                context.getContentResolver().openFileDescriptor(imageLocation, "r")) {
            byte[] remoteBytes = readBytes(new FileInputStream(pfd.getFileDescriptor()));
            assertArrayEquals(expected, remoteBytes);
        }
    }

    public void testCallComposerLoggingException() {
        CallLog.CallComposerLoggingException e =
                new CallLog.CallComposerLoggingException(
                        CallLog.CallComposerLoggingException.ERROR_STORAGE_FULL);
        assertEquals(CallLog.CallComposerLoggingException.ERROR_STORAGE_FULL,
                e.getErrorCode());
    }

    private byte[] readResourceDrawable(Context context, int id) throws Exception {
        InputStream inputStream = context.getResources().openRawResource(id);
        return readBytes(inputStream);
    }

    private byte[] readBytes(InputStream inputStream) throws Exception {
        byte[] buffer = new byte[1024];
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        int numRead;
        do {
            numRead = inputStream.read(buffer);
            if (numRead > 0) output.write(buffer, 0, numRead);
        } while (numRead > 0);
        return output.toByteArray();
    }

    private void waitUntilConditionIsTrueOrTimeout(Condition condition, long timeout,
            String description) {
        final long start = System.currentTimeMillis();
        while (!condition.expected().equals(condition.actual())
                && System.currentTimeMillis() - start < timeout) {
            sleep(50);
        }
        assertEquals(description, condition.expected(), condition.actual());
    }

    protected interface Condition {
        Object expected();
        Object actual();
    }

    private void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
        }
    }
}
