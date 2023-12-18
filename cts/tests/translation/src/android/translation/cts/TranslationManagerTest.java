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

package android.translation.cts;

import static android.view.translation.TranslationSpec.DATA_FORMAT_TEXT;

import static com.android.compatibility.common.util.ActivitiesWatcher.ActivityLifecycle.RESUMED;

import static com.google.common.truth.Truth.assertThat;

import android.app.Application;
import android.app.PendingIntent;
import android.content.Context;
import android.icu.util.ULocale;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.util.ArraySet;
import android.util.Log;
import android.view.translation.TranslationCapability;
import android.view.translation.TranslationContext;
import android.view.translation.TranslationManager;
import android.view.translation.TranslationRequest;
import android.view.translation.TranslationRequestValue;
import android.view.translation.TranslationResponse;
import android.view.translation.TranslationResponseValue;
import android.view.translation.TranslationSpec;
import android.view.translation.Translator;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.ActivitiesWatcher;
import com.android.compatibility.common.util.ActivitiesWatcher.ActivityWatcher;
import com.android.compatibility.common.util.RequiredServiceRule;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;

/**
 * Tests for {@link TranslationManager} related APIs.
 *
 * <p>
 * We use a non-standard {@link android.service.translation.TranslationService} for e2e CTS tests
 * that is set via shell command. This temporary service is not defined in the trusted
 * TranslationService, it should only receive queries from clients in the same package.</p>
 */
@AppModeFull(reason = "TODO(b/182330968): disable instant mode. Re-enable after we decouple the "
        + "service from the test package.")
@RunWith(AndroidJUnit4.class)
public class TranslationManagerTest {

    @Rule
    public final RequiredServiceRule mServiceRule = new RequiredServiceRule(
            android.content.Context.TRANSLATION_MANAGER_SERVICE);

    private static final String TAG = "BasicTranslationTest";

    private CtsTranslationService.ServiceWatcher mServiceWatcher;
    private ActivitiesWatcher mActivitiesWatcher;

    private static Context sContext;
    private static CtsTranslationService.TranslationReplier sTranslationReplier;

    private static String sOriginalLogTag;

    @BeforeClass
    public static void oneTimeSetup() {
        sTranslationReplier = CtsTranslationService.getTranslationReplier();
        sContext = ApplicationProvider.getApplicationContext();
        sOriginalLogTag = Helper.enableDebugLog();
    }

    @AfterClass
    public static void oneTimeReset() {
        Helper.disableDebugLog(sOriginalLogTag);
    }

    @Before
    public void setup() {
        CtsTranslationService.resetStaticState();
    }

    @After
    public void cleanup() {
        Helper.resetTemporaryTranslationService();
        if (mActivitiesWatcher != null) {
            final Application app = (Application) ApplicationProvider.getApplicationContext();
            app.unregisterActivityLifecycleCallbacks(mActivitiesWatcher);
        }
    }

    @Test
    public void testTranslationCapabilityUpdateListener() throws Exception{
        // enable cts translation service
        enableCtsTranslationService();

        final int FAKE_DATA_FORMAT_IMAGE = 2;
        // text to text capability
        final TranslationCapability updatedText2TextCapability =
                new TranslationCapability(TranslationCapability.STATE_ON_DEVICE,
                new TranslationSpec(ULocale.ENGLISH, DATA_FORMAT_TEXT),
                new TranslationSpec(ULocale.FRENCH, DATA_FORMAT_TEXT),
                true, 0);
        // text to image capability
        final TranslationCapability updatedText2ImageCapability =
                new TranslationCapability(TranslationCapability.STATE_ON_DEVICE,
                        new TranslationSpec(ULocale.ENGLISH, DATA_FORMAT_TEXT),
                        new TranslationSpec(ULocale.FRENCH, FAKE_DATA_FORMAT_IMAGE),
                        true, 0);
        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);

        // register translation capability update listener
        final AtomicReference<TranslationCapability> resultRef = new AtomicReference<>();
        final CountDownLatch latch = new CountDownLatch(3);
        final Consumer<TranslationCapability> updateListener = capability -> {
            Log.w(TAG, "text2textListener called. capability = " + capability);
            resultRef.set(capability);
            latch.countDown();
        };
        manager.addOnDeviceTranslationCapabilityUpdateListener(Executors.newSingleThreadExecutor(),
                updateListener);

        // wait service connected
        try {
            mServiceWatcher.waitOnConnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onConnected");
        }

        final CtsTranslationService service = mServiceWatcher.getService();
        // update text to text TranslationCapability
        service.updateTranslationCapability(updatedText2TextCapability);

        // Verify listener was called
        SystemClock.sleep(1_000);
        assertThat(latch.getCount()).isEqualTo(2);
        TranslationCapability capabilityText2Text = resultRef.get();
        assertTranslationCapability(capabilityText2Text, updatedText2TextCapability);

        // update text to image TranslationCapability
        service.updateTranslationCapability(updatedText2ImageCapability);

        // Verify listener was called
        SystemClock.sleep(1_000);
        assertThat(latch.getCount()).isEqualTo(1);
        TranslationCapability capabilityText2Image = resultRef.get();
        assertTranslationCapability(capabilityText2Image, updatedText2ImageCapability);

        // unregister listeners
        manager.removeOnDeviceTranslationCapabilityUpdateListener(updateListener);
        SystemClock.sleep(1_000);

        // update text to text TranslationCapability
        service.updateTranslationCapability(updatedText2TextCapability);
        // Verify listener was not called after unregister
        SystemClock.sleep(1_000);
        assertThat(latch.getCount()).isEqualTo(1);
    }

    private void assertTranslationCapability(TranslationCapability source,
            TranslationCapability expected) {
        assertThat(source.getState()).isEqualTo(expected.getState());
        assertThat(source.getSourceSpec()).isEqualTo(expected.getSourceSpec());
        assertThat(source.getTargetSpec()).isEqualTo(expected.getTargetSpec());
        assertThat(source.isUiTranslationEnabled()).isEqualTo(expected.isUiTranslationEnabled());
        assertThat(source.getSupportedTranslationFlags()).isEqualTo(
                expected.getSupportedTranslationFlags());
    }

    @Test
    public void testSingleTranslation() throws Exception{
        enableCtsTranslationService();

        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);

        sTranslationReplier.addResponse(
                new TranslationResponse.Builder(TranslationResponse.TRANSLATION_STATUS_SUCCESS)
                        .setTranslationResponseValue(0, new TranslationResponseValue
                                .Builder(TranslationResponseValue.STATUS_SUCCESS)
                                .setText("success")
                                .build())
                        .build());

        final TranslationContext translationContext = new TranslationContext.Builder(
                new TranslationSpec(ULocale.ENGLISH,
                        TranslationSpec.DATA_FORMAT_TEXT),
                new TranslationSpec(ULocale.FRENCH,
                        TranslationSpec.DATA_FORMAT_TEXT))
                .build();

        final CountDownLatch createTranslatorLatch = new CountDownLatch(1);
        final AtomicReference<Translator> translatorRef = new AtomicReference<>();
        manager.createOnDeviceTranslator(translationContext, r -> r.run(),
                new Consumer<Translator>() {
                    @Override
                    public void accept(Translator translator) {
                        createTranslatorLatch.countDown();
                        translatorRef.set(translator);
                    }
                });

        createTranslatorLatch.await(5_000, TimeUnit.MILLISECONDS);
        final Translator translator = translatorRef.get();

        try {
            mServiceWatcher.waitOnConnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onConnected");
        }

        assertThat(translator.isDestroyed()).isFalse();

        final CountDownLatch translationLatch = new CountDownLatch(1);
        final AtomicReference<TranslationResponse> responseRef = new AtomicReference<>();
        final ArrayList<TranslationRequestValue> values = new ArrayList<>();
        values.add(TranslationRequestValue.forText("hello world"));
        translator.translate(new TranslationRequest.Builder()
                        .setTranslationRequestValues(values)
                        .build(), new CancellationSignal(), (r) -> r.run(),
                new Consumer<TranslationResponse>() {
                    @Override
                    public void accept(TranslationResponse translationResponse) {
                        responseRef.set(translationResponse);
                        translationLatch.countDown();
                    }
                });

        sTranslationReplier.getNextTranslationRequest();

        translator.destroy();
        assertThat(translator.isDestroyed()).isTrue();
        try {
            mServiceWatcher.waitOnDisconnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onDisconnected");
        }

        // Wait for translation to finish
        translationLatch.await();
        sTranslationReplier.assertNoUnhandledTranslationRequests();

        final TranslationResponse response = responseRef.get();
        Log.v(TAG, "TranslationResponse=" + response);

        assertThat(response).isNotNull();
        assertThat(response.getTranslationStatus())
                .isEqualTo(TranslationResponse.TRANSLATION_STATUS_SUCCESS);
        assertThat(response.isFinalResponse()).isTrue();
        assertThat(response.getTranslationResponseValues().size()).isEqualTo(1);
        assertThat(response.getViewTranslationResponses().size()).isEqualTo(0);

        final TranslationResponseValue value = response.getTranslationResponseValues().get(0);
        assertThat(value.getStatusCode()).isEqualTo(TranslationResponseValue.STATUS_SUCCESS);
        assertThat(value.getText().toString()).isEqualTo("success");
        assertThat(value.getTransliteration()).isNull();
        assertThat(value.getExtras()).isEqualTo(Bundle.EMPTY);
    }

    @Test
    public void testTranslation_partialResponses() throws Exception{
        enableCtsTranslationService();

        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);

        sTranslationReplier.addResponse(
                new TranslationResponse.Builder(TranslationResponse.TRANSLATION_STATUS_SUCCESS)
                        .setTranslationResponseValue(0, new TranslationResponseValue
                                .Builder(TranslationResponseValue.STATUS_SUCCESS)
                                .setText("success")
                                .build())
                        .setFinalResponse(false)
                        .build()).addResponse(
                new TranslationResponse.Builder(TranslationResponse.TRANSLATION_STATUS_SUCCESS)
                        .setTranslationResponseValue(1, new TranslationResponseValue
                                .Builder(TranslationResponseValue.STATUS_SUCCESS)
                                .setText("success 2")
                                .build())
                        .setFinalResponse(true)
                        .build());

        final TranslationContext translationContext = new TranslationContext.Builder(
                new TranslationSpec(ULocale.ENGLISH,
                        TranslationSpec.DATA_FORMAT_TEXT),
                new TranslationSpec(ULocale.FRENCH,
                        TranslationSpec.DATA_FORMAT_TEXT))
                .build();

        final CountDownLatch createTranslatorLatch = new CountDownLatch(1);
        final AtomicReference<Translator> translatorRef = new AtomicReference<>();
        manager.createOnDeviceTranslator(translationContext, r -> r.run(),
                translator -> {
                    createTranslatorLatch.countDown();
                    translatorRef.set(translator);
                });

        createTranslatorLatch.await(5_000, TimeUnit.MILLISECONDS);
        final Translator translator = translatorRef.get();

        try {
            mServiceWatcher.waitOnConnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onConnected");
        }

        assertThat(translator).isNotNull();
        assertThat(translator.isDestroyed()).isFalse();

        final CountDownLatch translationLatch = new CountDownLatch(2);
        final AtomicReference<List<TranslationResponse>> responsesRef = new AtomicReference<>();
        responsesRef.set(new ArrayList<TranslationResponse>());
        final ArrayList<TranslationRequestValue> values = new ArrayList<>();
        values.add(TranslationRequestValue.forText("hello world"));
        translator.translate(new TranslationRequest.Builder()
                        .setTranslationRequestValues(values)
                        .setFlags(TranslationRequest.FLAG_PARTIAL_RESPONSES)
                        .build(), new CancellationSignal(), (r) -> r.run(),
                translationResponse -> {
                    responsesRef.getAndUpdate(responses -> {
                        responses.add(translationResponse);
                        return responses;
                    });
                    translationLatch.countDown();
                });

        sTranslationReplier.getNextTranslationRequest();

        translator.destroy();
        assertThat(translator.isDestroyed()).isTrue();
        try {
            mServiceWatcher.waitOnDisconnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onDisconnected");
        }

        // Wait for translation to finish
        translationLatch.await();
        sTranslationReplier.assertNoUnhandledTranslationRequests();

        final List<TranslationResponse> responses = responsesRef.get();
        Log.v(TAG, "TranslationResponses=" + responses);
        assertThat(responses.size()).isEqualTo(2);

        final TranslationResponse response1 = responses.get(0);
        assertThat(response1).isNotNull();
        assertThat(response1.getTranslationStatus())
                .isEqualTo(TranslationResponse.TRANSLATION_STATUS_SUCCESS);
        assertThat(response1.isFinalResponse()).isFalse();
        assertThat(response1.getTranslationResponseValues().size()).isEqualTo(1);
        assertThat(response1.getViewTranslationResponses().size()).isEqualTo(0);

        final TranslationResponseValue value1 = response1.getTranslationResponseValues().get(0);
        assertThat(value1.getStatusCode()).isEqualTo(TranslationResponseValue.STATUS_SUCCESS);
        assertThat(value1.getText().toString()).isEqualTo("success");
        assertThat(value1.getTransliteration()).isNull();
        assertThat(value1.getExtras()).isEqualTo(Bundle.EMPTY);

        final TranslationResponse response2 = responses.get(1);
        assertThat(response2).isNotNull();
        assertThat(response2.getTranslationStatus())
                .isEqualTo(TranslationResponse.TRANSLATION_STATUS_SUCCESS);
        assertThat(response2.isFinalResponse()).isTrue();
        assertThat(response2.getTranslationResponseValues().size()).isEqualTo(1);
        assertThat(response2.getViewTranslationResponses().size()).isEqualTo(0);

        final TranslationResponseValue value2 = response2.getTranslationResponseValues().get(1);
        assertThat(value2.getStatusCode()).isEqualTo(TranslationResponseValue.STATUS_SUCCESS);
        assertThat(value2.getText().toString()).isEqualTo("success 2");
        assertThat(value2.getTransliteration()).isNull();
        assertThat(value2.getExtras()).isEqualTo(Bundle.EMPTY);
    }

    @Test
    public void testTranslationCancelled() throws Exception{
        enableCtsTranslationService();

        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);

        sTranslationReplier.addResponse(
                new TranslationResponse.Builder(TranslationResponse.TRANSLATION_STATUS_SUCCESS)
                        .setTranslationResponseValue(0, new TranslationResponseValue
                                .Builder(TranslationResponseValue.STATUS_SUCCESS)
                                .setText("success")
                                .build())
                        .build());

        final CountDownLatch translationLatch = new CountDownLatch(1);
        final AtomicReference<TranslationResponse> responseRef = new AtomicReference<>();

        final TranslationContext translationContext = new TranslationContext.Builder(
                new TranslationSpec(ULocale.ENGLISH, TranslationSpec.DATA_FORMAT_TEXT),
                new TranslationSpec(ULocale.FRENCH, TranslationSpec.DATA_FORMAT_TEXT))
                .build();

        final CountDownLatch createTranslatorLatch = new CountDownLatch(1);
        final AtomicReference<Translator> translatorRef = new AtomicReference<>();
        manager.createOnDeviceTranslator(translationContext, r -> r.run(),
                new Consumer<Translator>() {
                    @Override
                    public void accept(Translator translator) {
                        createTranslatorLatch.countDown();
                        translatorRef.set(translator);
                    }
                });

        createTranslatorLatch.await(5_000, TimeUnit.MILLISECONDS);
        final Translator translator = translatorRef.get();

        try {
            mServiceWatcher.waitOnConnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onConnected");
        }

        assertThat(translator.isDestroyed()).isFalse();

        final Consumer<TranslationResponse> callback = new Consumer<TranslationResponse>() {
            @Override
            public void accept(TranslationResponse translationResponse) {
                responseRef.set(translationResponse);
                translationLatch.countDown();
            }
        };

        final CancellationSignal cancellationSignal = new CancellationSignal();

        final ArrayList<TranslationRequestValue> values = new ArrayList<>();
        values.add(TranslationRequestValue.forText("hello world"));
        translator.translate(new TranslationRequest.Builder()
                .setTranslationRequestValues(values)
                .build(), cancellationSignal, (r) -> r.run(), callback);

        // TODO: implement with cancellation signal listener
        // cancel translation request
        cancellationSignal.cancel();

        sTranslationReplier.assertNoUnhandledTranslationRequests();

        translator.destroy();
        assertThat(translator.isDestroyed()).isTrue();
        try {
            mServiceWatcher.waitOnDisconnected();
        } catch (InterruptedException e) {
            Log.w(TAG, "Exception waiting for onDisconnected");
        }
    }

    @Test
    public void testGetTranslationCapabilities() throws Exception{
        enableCtsTranslationService();

        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);
        final CountDownLatch latch = new CountDownLatch(1);
        final AtomicReference<Set<TranslationCapability>> resultRef =
                new AtomicReference<>();

        final Thread th = new Thread(() -> {
            final Set<TranslationCapability> capabilities =
                    manager.getOnDeviceTranslationCapabilities(DATA_FORMAT_TEXT, DATA_FORMAT_TEXT);
            resultRef.set(capabilities);
            latch.countDown();
        });
        th.start();
        latch.await();

        final ArraySet<TranslationCapability> capabilities = new ArraySet<>(resultRef.get());
        assertThat(capabilities.size()).isEqualTo(1);
        capabilities.forEach((capability) -> {
            assertThat(capability.getState()).isEqualTo(TranslationCapability.STATE_ON_DEVICE);

            assertThat(capability.getSupportedTranslationFlags()).isEqualTo(0);
            assertThat(capability.isUiTranslationEnabled()).isTrue();
            assertThat(capability.getSourceSpec().getLocale()).isEqualTo(ULocale.ENGLISH);
            assertThat(capability.getSourceSpec().getDataFormat()).isEqualTo(DATA_FORMAT_TEXT);
            assertThat(capability.getTargetSpec().getLocale()).isEqualTo(ULocale.FRENCH);
            assertThat(capability.getSourceSpec().getDataFormat()).isEqualTo(DATA_FORMAT_TEXT);
        });
    }

    @Test
    public void testGetTranslationSettingsActivityIntent() throws Exception{
        enableCtsTranslationService();

        final TranslationManager manager = sContext.getSystemService(TranslationManager.class);
        final PendingIntent pendingIntent = manager.getOnDeviceTranslationSettingsActivityIntent();

        assertThat(pendingIntent).isNotNull();
        assertThat(pendingIntent.isImmutable()).isTrue();

        // Start Settings Activity and verify if the expected Activity resumed
        mActivitiesWatcher = new ActivitiesWatcher(5_000);
        final Application app = (Application) ApplicationProvider.getApplicationContext();
        app.registerActivityLifecycleCallbacks(mActivitiesWatcher);
        final ActivityWatcher watcher = mActivitiesWatcher.watch(SimpleActivity.class);

        pendingIntent.send();

        watcher.waitFor(RESUMED);
    }

    //TODO(183605243): add test for cancelling translation.

    protected void enableCtsTranslationService() {
        mServiceWatcher = CtsTranslationService.setServiceWatcher();
        Helper.setTemporaryTranslationService(CtsTranslationService.SERVICE_NAME);
    }
}
