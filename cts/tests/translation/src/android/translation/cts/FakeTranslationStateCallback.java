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

import android.icu.util.ULocale;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.translation.UiTranslationStateCallback;

/**
 * The test implementation for {@link UiTranslationStateCallback}.
 */
public class FakeTranslationStateCallback implements UiTranslationStateCallback {

    private static final String TAG = "MockTranslationStateCallback";

    private boolean mStartCalled;
    private boolean mFinishCalled;
    private boolean mPausedCalled;
    private boolean mResumedCalled;
    private ULocale mSourceLocale;
    private ULocale mTargetLocale;

    FakeTranslationStateCallback() {
        resetStates();
    }

    void resetStates() {
        mStartCalled = false;
        mFinishCalled = false;
        mPausedCalled = false;
        mResumedCalled = false;
        mSourceLocale = null;
        mTargetLocale = null;
    }

    Pair<ULocale, ULocale> getStartedLanguagePair() {
        return new Pair<>(mSourceLocale, mTargetLocale);
    }

    boolean isOnStartedCalled() {
        return mStartCalled;
    }

    boolean isOnFinishedCalled() {
        return mFinishCalled;
    }

    boolean isOnPausedCalled() {
        return mPausedCalled;
    }

    boolean isOnResumedCalled() {
        return mResumedCalled;
    }

    @Override
    public void onStarted(ULocale sourceLocale, ULocale targetLocale) {
        UiTranslationStateCallback.super.onStarted(sourceLocale, targetLocale);
        Log.d(TAG, "onStarted, source=" + sourceLocale.getLanguage() + " targetLocale="
                + targetLocale.getLanguage());
        mStartCalled = true;
        mSourceLocale = sourceLocale;
        mTargetLocale = targetLocale;
    }

    @Override
    public void onResumed(ULocale sourceLocale, ULocale targetLocale) {
        UiTranslationStateCallback.super.onResumed(sourceLocale, targetLocale);
        Log.d(TAG, "onResumed, source=" + sourceLocale.getLanguage() + " targetLocale="
                + targetLocale.getLanguage());
        mResumedCalled = true;
    }

    @Override
    public void onPaused() {
        Log.d(TAG, "onPaused");
        mPausedCalled = true;
    }

    @Override
    public void onFinished() {
        Log.d(TAG, "onFinished");
        mFinishCalled = true;
    }
}
