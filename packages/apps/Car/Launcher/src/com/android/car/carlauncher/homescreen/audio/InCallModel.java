/*
 * Copyright (C) 2020 Google Inc.
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

package com.android.car.carlauncher.homescreen.audio;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.IBinder;
import android.telecom.Call;
import android.telecom.TelecomManager;
import android.util.Log;
import android.view.View;

import androidx.core.content.ContextCompat;

import com.android.car.carlauncher.R;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.audio.telecom.InCallServiceImpl;
import com.android.car.carlauncher.homescreen.ui.CardContent;
import com.android.car.carlauncher.homescreen.ui.CardHeader;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextWithControlsView;
import com.android.car.telephony.common.CallDetail;
import com.android.car.telephony.common.TelecomUtils;
import com.android.internal.annotations.VisibleForTesting;

import java.io.FileNotFoundException;
import java.io.InputStream;
import java.time.Clock;
import java.util.concurrent.CompletableFuture;

/**
 * The {@link HomeCardInterface.Model} for ongoing phone calls.
 */
public class InCallModel implements HomeCardInterface.Model, InCallServiceImpl.InCallListener {

    private static final String TAG = "InCallModel";
    private static final boolean DEBUG = false;

    private Context mContext;
    private TelecomManager mTelecomManager;
    private final Clock mElapsedTimeClock;
    private Call mCurrentCall;
    private boolean mMuteCallToggle = true;
    private CompletableFuture<Void> mPhoneNumberInfoFuture;

    private InCallServiceImpl mInCallService;
    private HomeCardInterface.Presenter mPresenter;

    private CardHeader mCardHeader;
    private CardContent mCardContent;
    private CharSequence mOngoingCallSubtitle;
    private DescriptiveTextWithControlsView.Control mMuteButton;
    private DescriptiveTextWithControlsView.Control mEndCallButton;
    private DescriptiveTextWithControlsView.Control mDialpadButton;

    private final ServiceConnection mInCallServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            if (DEBUG) Log.d(TAG, "onServiceConnected: " + name + ", service: " + service);
            mInCallService = ((InCallServiceImpl.LocalBinder) service).getService();
            mInCallService.addListener(InCallModel.this);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (DEBUG) Log.d(TAG, "onServiceDisconnected: " + name);
            mInCallService = null;
        }
    };

    private Call.Callback mCallback = new Call.Callback() {
        @Override
        public void onStateChanged(Call call, int state) {
            super.onStateChanged(call, state);
            if (state == Call.STATE_ACTIVE) {
                mCurrentCall = call;
                mMuteCallToggle = true;
                CallDetail callDetails = CallDetail.fromTelecomCallDetail(call.getDetails());
                // If the home app does not have permission to read contacts, just display the
                // phone number
                if (ContextCompat.checkSelfPermission(mContext, Manifest.permission.READ_CONTACTS)
                        != PackageManager.PERMISSION_GRANTED) {
                    updateModelWithPhoneNumber(callDetails.getNumber());
                    return;
                }
                if (mPhoneNumberInfoFuture != null) {
                    mPhoneNumberInfoFuture.cancel(/* mayInterruptIfRunning= */ true);
                }
                mPhoneNumberInfoFuture = TelecomUtils.getPhoneNumberInfo(mContext,
                        callDetails.getNumber())
                        .thenAcceptAsync(x -> updateModelWithContact(x),
                                mContext.getMainExecutor());
            }
        }
    };

    public InCallModel(Clock elapsedTimeClock) {
        mElapsedTimeClock = elapsedTimeClock;
    }

    @Override
    public void onCreate(Context context) {
        mContext = context;
        mTelecomManager = context.getSystemService(TelecomManager.class);
        mOngoingCallSubtitle = context.getResources().getString(R.string.ongoing_call_text);
        initializeAudioControls();
        try {
            PackageManager pm = context.getPackageManager();
            Drawable appIcon = pm.getApplicationIcon(mTelecomManager.getDefaultDialerPackage());
            CharSequence appName = pm.getApplicationLabel(
                    pm.getApplicationInfo(mTelecomManager.getDefaultDialerPackage(), /* flags = */
                            0));
            mCardHeader = new CardHeader(appName, appIcon);
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "No default dialer package found", e);
        }

        Intent intent = new Intent(context, InCallServiceImpl.class);
        intent.setAction(InCallServiceImpl.ACTION_LOCAL_BIND);
        context.getApplicationContext().bindService(intent, mInCallServiceConnection,
                Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onDestroy(Context context) {
        if (mInCallService != null) {
            context.getApplicationContext().unbindService(mInCallServiceConnection);
            mInCallService = null;
        }
        if (mPhoneNumberInfoFuture != null) {
            mPhoneNumberInfoFuture.cancel(/* mayInterruptIfRunning= */true);
        }
    }

    @Override
    public void setPresenter(HomeCardInterface.Presenter presenter) {
        mPresenter = presenter;
    }

    @Override
    public CardHeader getCardHeader() {
        return mCardContent == null ? null : mCardHeader;
    }

    @Override
    public CardContent getCardContent() {
        return mCardContent;
    }

    /**
     * Clicking the card opens the default dialer application that fills the role of {@link
     * android.app.role.RoleManager#ROLE_DIALER}. This application will have an appropriate UI to
     * display as one of the requirements to fill this role is to provide an ongoing call UI.
     */
    @Override
    public void onClick(View view) {
        PackageManager pm = mContext.getPackageManager();
        Intent intent = pm.getLaunchIntentForPackage(mTelecomManager.getDefaultDialerPackage());
        if (intent != null) {
            mContext.startActivity(intent);
        } else {
            if (DEBUG) {
                Log.d(TAG, "No launch intent found for dialer package: "
                        + mTelecomManager.getDefaultDialerPackage());
            }
        }
    }

    /**
     * When a {@link Call} is added, notify the {@link HomeCardInterface.Presenter} to update the
     * card to display content on the ongoing phone call.
     */
    @Override
    public void onCallAdded(Call call) {
        if (call != null) {
            call.registerCallback(mCallback);
        }
    }

    /**
     * When a {@link Call} is removed, notify the {@link HomeCardInterface.Presenter} to update the
     * card to remove the content on the no longer ongoing phone call.
     */
    @Override
    public void onCallRemoved(Call call) {
        mCurrentCall = null;
        mCardContent = null;
        mPresenter.onModelUpdated(this);
        if (call != null) {
            call.unregisterCallback(mCallback);
        }
    }

    /**
     * Updates the model's content using the given phone number.
     */
    @VisibleForTesting
    void updateModelWithPhoneNumber(String number) {
        String formattedNumber = TelecomUtils.getFormattedNumber(mContext, number);
        mCardContent = new DescriptiveTextWithControlsView(null, formattedNumber,
                mOngoingCallSubtitle, mElapsedTimeClock.millis(), mMuteButton, mEndCallButton,
                mDialpadButton);
        mPresenter.onModelUpdated(this);
    }

    /**
     * Updates the model's content using the given {@link TelecomUtils.PhoneNumberInfo}. If there is
     * a corresponding contact, use the contact's name and avatar. If the contact doesn't have an
     * avatar, use an icon with their first initial.
     */
    @VisibleForTesting
    void updateModelWithContact(TelecomUtils.PhoneNumberInfo phoneNumberInfo) {
        String contactName = phoneNumberInfo.getDisplayName();
        Drawable contactImage = null;
        if (phoneNumberInfo.getAvatarUri() != null) {
            try {
                InputStream inputStream = mContext.getContentResolver().openInputStream(
                        phoneNumberInfo.getAvatarUri());
                contactImage = Drawable.createFromStream(inputStream,
                        phoneNumberInfo.getAvatarUri().toString());
            } catch (FileNotFoundException e) {
                // If no file is found for the contact's avatar URI, the icon will be set to a
                // LetterTile below.
                if (DEBUG) {
                    Log.d(TAG, "Unable to find contact avatar from Uri: "
                            + phoneNumberInfo.getAvatarUri(), e);
                }
            }
        }
        if (contactImage == null) {
            contactImage = TelecomUtils.createLetterTile(mContext,
                    phoneNumberInfo.getInitials(), phoneNumberInfo.getDisplayName());
        }
        mCardContent = new DescriptiveTextWithControlsView(contactImage, contactName,
                mOngoingCallSubtitle, mElapsedTimeClock.millis(), mMuteButton, mEndCallButton,
                mDialpadButton);
        mPresenter.onModelUpdated(this);
    }

    private void initializeAudioControls() {
        mMuteButton = new DescriptiveTextWithControlsView.Control(
                mContext.getDrawable(R.drawable.ic_mic_off),
                v -> {
                    mInCallService.setMuted(mMuteCallToggle);
                    mMuteCallToggle = !mMuteCallToggle;
                });
        mEndCallButton = new DescriptiveTextWithControlsView.Control(
                mContext.getDrawable(R.drawable.ic_call_end_button),
                v -> mCurrentCall.disconnect());
        mDialpadButton = new DescriptiveTextWithControlsView.Control(
                mContext.getDrawable(R.drawable.ic_dialpad), this::onClick);
    }
}
