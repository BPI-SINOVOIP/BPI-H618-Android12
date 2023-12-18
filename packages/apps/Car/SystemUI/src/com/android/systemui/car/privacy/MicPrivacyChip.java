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

package com.android.systemui.car.privacy;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.constraintlayout.motion.widget.MotionLayout;

import com.android.systemui.R;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Car optimized Mic Privacy Chip View that is shown when microphone is being used.
 *
 * State flows:
 * Base state:
 * <ul>
 * <li>INVISIBLE - Start Mic Use ->> Mic Status?</li>
 * </ul>
 * Mic On:
 * <ul>
 * <li>Mic Status? - On ->> ACTIVE_INIT</li>
 * <li>ACTIVE_INIT - delay ->> ACTIVE</li>
 * <li>ACTIVE - Stop Mic Use ->> INACTIVE</li>
 * <li>INACTIVE - delay ->> INVISIBLE</li>
 * </ul>
 * Mic Off:
 * <ul>
 * <li>Mic Status? - Off ->> MICROPHONE_OFF</li>
 * </ul>
 */
public class MicPrivacyChip extends MotionLayout {
    private final static boolean DEBUG = false;
    private final static String TAG = "MicPrivacyChip";
    private final static String TYPES_TEXT_MICROPHONE = "microphone";

    private final int mDelayPillToCircle;
    private final int mDelayToNoMicUsage;

    private AnimationStates mCurrentTransitionState;
    private boolean mIsInflated;
    private boolean mIsMicrophoneEnabled;
    private ScheduledExecutorService mExecutor;

    public MicPrivacyChip(@NonNull Context context) {
        this(context, /* attrs= */ null);
    }

    public MicPrivacyChip(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, /* defStyleAttrs= */ 0);
    }

    public MicPrivacyChip(@NonNull Context context,
            @Nullable AttributeSet attrs, int defStyleAttrs) {
        super(context, attrs, defStyleAttrs);

        mDelayPillToCircle = getResources().getInteger(R.integer.privacy_chip_pill_to_circle_delay);
        mDelayToNoMicUsage = getResources().getInteger(R.integer.privacy_chip_no_mic_usage_delay);

        mExecutor = Executors.newSingleThreadScheduledExecutor();
        mIsInflated = false;

        // Microphone is enabled by default (invisible state).
        mIsMicrophoneEnabled = true;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mCurrentTransitionState = AnimationStates.INVISIBLE;
        mIsInflated = true;
    }

    @Override
    public void setOnClickListener(View.OnClickListener onClickListener) {
        // required for CTS tests.
        super.setOnClickListener(onClickListener);
        // required for rotary.
        requireViewById(R.id.focus_view).setOnClickListener(onClickListener);
    }

    /**
     * Sets whether microphone is enabled or disabled.
     * If enabled, animates to {@link AnimationStates#INVISIBLE}.
     * Otherwise, animates to {@link AnimationStates#MICROPHONE_OFF}.
     */
    @UiThread
    public void setMicrophoneEnabled(boolean isMicrophoneEnabled) {
        if (DEBUG) Log.d(TAG, "Microphone enabled: " + isMicrophoneEnabled);

        if (mIsMicrophoneEnabled == isMicrophoneEnabled) {
            if (isMicrophoneEnabled) {
                switch (mCurrentTransitionState) {
                    case INVISIBLE:
                    case ACTIVE:
                    case INACTIVE:
                    case ACTIVE_INIT:
                        return;
                }
            } else {
                if (mCurrentTransitionState == AnimationStates.MICROPHONE_OFF) return;
            }
        }

        mIsMicrophoneEnabled = isMicrophoneEnabled;

        if (!mIsInflated) {
            if (DEBUG) Log.d(TAG, "Layout not inflated");

            return;
        }

        if (mIsMicrophoneEnabled) {
            if (DEBUG) Log.d(TAG, "setTransition: invisibleFromMicOff");
            setTransition(R.id.invisibleFromMicOff);
        } else {
            switch (mCurrentTransitionState) {
                case INVISIBLE:
                    if (DEBUG) Log.d(TAG, "setTransition: micOffFromInvisible");
                    setTransition(R.id.micOffFromInvisible);
                    break;
                case ACTIVE_INIT:
                    if (DEBUG) Log.d(TAG, "setTransition: micOffFromActiveInit");
                    setTransition(R.id.micOffFromActiveInit);
                    break;
                case ACTIVE:
                    if (DEBUG) Log.d(TAG, "setTransition: micOffFromActive");
                    setTransition(R.id.micOffFromActive);
                    break;
                case INACTIVE:
                    if (DEBUG) Log.d(TAG, "setTransition: micOffFromInactive");
                    setTransition(R.id.micOffFromInactive);
                    break;
                default:
                    return;
            }
        }

        mExecutor.shutdownNow();
        mExecutor = Executors.newSingleThreadScheduledExecutor();

        // TODO(182938429): Use Transition Listeners once ConstraintLayout 2.0.0 is being used.

        // When microphone is off, mic privacy chip is always visible.
        if (!mIsMicrophoneEnabled) setVisibility(View.VISIBLE);
        setContentDescription(!mIsMicrophoneEnabled);
        mCurrentTransitionState = mIsMicrophoneEnabled ? MicPrivacyChip.AnimationStates.INVISIBLE
                : MicPrivacyChip.AnimationStates.MICROPHONE_OFF;
        transitionToEnd();
        // When microphone is on, after animation we hide mic privacy chip until mic is next used.
        if (mIsMicrophoneEnabled) setVisibility(View.GONE);
    }

    private void setContentDescription(boolean isMicOff) {
        String contentDescription;
        if (isMicOff) {
            contentDescription = getResources().getString(R.string.mic_privacy_chip_off_content);
        } else {
            contentDescription = getResources().getString(
                    R.string.ongoing_privacy_chip_content_multiple_apps, TYPES_TEXT_MICROPHONE);
        }

        setContentDescription(contentDescription);
    }

    /**
     * Starts reveal animation for Mic Privacy Chip.
     */
    @UiThread
    public void animateIn() {
        if (!mIsInflated) {
            if (DEBUG) Log.d(TAG, "Layout not inflated");

            return;
        }

        if (mCurrentTransitionState == null) {
            if (DEBUG) Log.d(TAG, "Current transition state is null or empty.");

            return;
        }

        switch (mCurrentTransitionState) {
            case INVISIBLE:
                if (DEBUG) {
                    Log.d(TAG, mIsMicrophoneEnabled ? "setTransition: activeInitFromInvisible"
                            : "setTransition: micOffFromInvisible");
                }
                setTransition(mIsMicrophoneEnabled ? R.id.activeInitFromInvisible
                        : R.id.micOffFromInvisible);
                break;
            case INACTIVE:
                if (DEBUG) {
                    Log.d(TAG, mIsMicrophoneEnabled ? "setTransition: activeInitFromInactive"
                            : "setTransition: micOffFromInactive");
                }

                setTransition(mIsMicrophoneEnabled ? R.id.activeInitFromInactive
                        : R.id.micOffFromInactive);
                break;
            case MICROPHONE_OFF:
                if (DEBUG) {
                    Log.d(TAG, mIsMicrophoneEnabled ? "setTransition: activeInitFromMicOff"
                            : "No Transition.");
                }

                if (!mIsMicrophoneEnabled) {
                    return;
                }

                setTransition(R.id.activeInitFromMicOff);
                break;
            default:
                if (DEBUG) {
                    Log.d(TAG, "Early exit, mCurrentTransitionState= "
                            + mCurrentTransitionState);
                }

                return;
        }

        mExecutor.shutdownNow();
        mExecutor = Executors.newSingleThreadScheduledExecutor();

        // TODO(182938429): Use Transition Listeners once ConstraintLayout 2.0.0 is being used.
        setContentDescription(false);
        setVisibility(View.VISIBLE);
        transitionToEnd();
        mCurrentTransitionState = AnimationStates.ACTIVE_INIT;
        if (mIsMicrophoneEnabled) {
            mExecutor.schedule(MicPrivacyChip.this::animateToOrangeCircle, mDelayPillToCircle,
                    TimeUnit.MILLISECONDS);
        }
    }

    // TODO(182938429): Use Transition Listeners once ConstraintLayout 2.0.0 is being used.
    private void animateToOrangeCircle() {
        setTransition(R.id.activeFromActiveInit);

        // Since this is launched using a {@link ScheduledExecutorService}, its UI based elements
        // need to execute on main executor.
        getContext().getMainExecutor().execute(() -> {
            mCurrentTransitionState = AnimationStates.ACTIVE;
            transitionToEnd();
        });
    }

    /**
     * Starts conceal animation for Mic Privacy Chip.
     */
    @UiThread
    public void animateOut() {
        if (!mIsInflated) {
            if (DEBUG) Log.d(TAG, "Layout not inflated");

            return;
        }

        switch (mCurrentTransitionState) {
            case ACTIVE_INIT:
                if (DEBUG) Log.d(TAG, "setTransition: inactiveFromActiveInit");

                setTransition(R.id.inactiveFromActiveInit);
                break;
            case ACTIVE:
                if (DEBUG) Log.d(TAG, "setTransition: inactiveFromActive");

                setTransition(R.id.inactiveFromActive);
                break;
            default:
                if (DEBUG) {
                    Log.d(TAG, "Early exit, mCurrentTransitionState= "
                            + mCurrentTransitionState);
                }

                return;
        }

        mExecutor.shutdownNow();
        mExecutor = Executors.newSingleThreadScheduledExecutor();

        if (mCurrentTransitionState.equals(AnimationStates.MICROPHONE_OFF)) {
            mCurrentTransitionState = AnimationStates.INACTIVE;
            mExecutor.schedule(MicPrivacyChip.this::reset, mDelayToNoMicUsage,
                    TimeUnit.MILLISECONDS);
            return;
        }

        // TODO(182938429): Use Transition Listeners once ConstraintLayout 2.0.0 is being used.
        mCurrentTransitionState = AnimationStates.INACTIVE;
        transitionToEnd();
        mExecutor.schedule(MicPrivacyChip.this::reset, mDelayToNoMicUsage,
                TimeUnit.MILLISECONDS);
    }

    // TODO(182938429): Use Transition Listeners once ConstraintLayout 2.0.0 is being used.
    private void reset() {
        if (mIsMicrophoneEnabled) {
            if (DEBUG) Log.d(TAG, "setTransition: invisibleFromInactive");

            setTransition(R.id.invisibleFromInactive);
        } else {
            if (DEBUG) Log.d(TAG, "setTransition: invisibleFromMicOff");

            setTransition(R.id.invisibleFromMicOff);
        }

        // Since this is launched using a {@link ScheduledExecutorService}, its UI based elements
        // need to execute on main executor.
        getContext().getMainExecutor().execute(() -> {
            mCurrentTransitionState = AnimationStates.INVISIBLE;
            transitionToEnd();
            setVisibility(View.GONE);
        });
    }

    private enum AnimationStates {
        INVISIBLE,
        ACTIVE_INIT,
        ACTIVE,
        INACTIVE,
        MICROPHONE_OFF,
    }
}
