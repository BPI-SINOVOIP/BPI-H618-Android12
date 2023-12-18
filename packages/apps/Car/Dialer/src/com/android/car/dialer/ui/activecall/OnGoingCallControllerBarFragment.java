/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.dialer.ui.activecall;

import android.app.AlertDialog;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.text.SpannableString;
import android.text.style.TextAppearanceSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.util.Pair;
import androidx.core.util.Preconditions;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProvider;

import com.android.car.apps.common.util.ViewUtils;
import com.android.car.dialer.R;
import com.android.car.dialer.log.L;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.telephony.common.CallDetail;
import com.android.car.ui.AlertDialogBuilder;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.recyclerview.CarUiListItemAdapter;

import com.google.common.collect.ImmutableMap;

import java.util.ArrayList;
import java.util.List;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

/** A Fragment of the bar which controls on going call. */
@AndroidEntryPoint(Fragment.class)
public class OnGoingCallControllerBarFragment extends Hilt_OnGoingCallControllerBarFragment {
    private static final String TAG = "CD.OngoingCallCtlFrg";

    private static final ImmutableMap<Integer, AudioRouteInfo> AUDIO_ROUTES =
            ImmutableMap.<Integer, AudioRouteInfo>builder()
                    .put(CallAudioState.ROUTE_WIRED_HEADSET, new AudioRouteInfo(
                            R.string.audio_route_handset,
                            R.drawable.ic_audio_route_headset,
                            R.drawable.ic_audio_route_headset_activatable))
                    .put(CallAudioState.ROUTE_EARPIECE, new AudioRouteInfo(
                            R.string.audio_route_handset,
                            R.drawable.ic_audio_route_earpiece,
                            R.drawable.ic_audio_route_earpiece_activatable))
                    .put(CallAudioState.ROUTE_BLUETOOTH, new AudioRouteInfo(
                            R.string.audio_route_vehicle,
                            R.drawable.ic_audio_route_vehicle,
                            R.drawable.ic_audio_route_vehicle_activatable))
                    .put(CallAudioState.ROUTE_SPEAKER, new AudioRouteInfo(
                            R.string.audio_route_phone_speaker,
                            R.drawable.ic_audio_route_speaker,
                            R.drawable.ic_audio_route_speaker_activatable))
                    .build();

    @Inject UiCallManager mUiCallManager;
    private InCallViewModel mInCallViewModel;

    private AlertDialog mAudioRouteSelectionDialog;
    private List<CarUiListItem> mAudioRouteListItems;
    private List<Integer> mAvailableRoutes;
    private CarUiListItemAdapter mAudioRouteAdapter;
    private View mMuteButton;
    private View mAudioRouteView;
    private ImageView mAudioRouteButton;
    private TextView mAudioRouteText;
    private View mMergeButton;
    private View mPauseButton;
    private LiveData<Call> mPrimaryCallLiveData;
    private LiveData<CallDetail> mPrimaryCallDetailLiveData;
    private LiveData<List<Call>> mOngoingCallListLiveData;
    private LiveData<Pair<Call, Call>> mOngoingCallPairLiveData;
    private MutableLiveData<Boolean> mDialpadState;
    private LiveData<List<Call>> mCallListLiveData;
    private int mPrimaryCallState;
    private int mActiveRoute;
    private LiveData<CallAudioState> mCallAudioState;
    private LiveData<List<Integer>> mAudioRoutes;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mAudioRouteListItems = new ArrayList<>();
        mAudioRouteAdapter = new CarUiListItemAdapter(mAudioRouteListItems);

        AlertDialogBuilder audioRouteSelectionDialogBuilder = new AlertDialogBuilder(getContext())
                .setAdapter(mAudioRouteAdapter)
                .setTitle(getString(R.string.audio_route_dialog_title));

        String subtitle = getString(R.string.audio_route_dialog_subtitle);
        if (!subtitle.isEmpty()) {
            audioRouteSelectionDialogBuilder.setSubtitle(subtitle);
        }

        mAudioRouteSelectionDialog = audioRouteSelectionDialogBuilder.create();

        mInCallViewModel = new ViewModelProvider(getActivity()).get(InCallViewModel.class);

        mInCallViewModel.getPrimaryCallState().observe(this, this::setCallState);
        mPrimaryCallLiveData = mInCallViewModel.getPrimaryCall();
        mPrimaryCallDetailLiveData = mInCallViewModel.getPrimaryCallDetail();
        mOngoingCallPairLiveData = mInCallViewModel.getOngoingCallPair();

        mOngoingCallListLiveData = mInCallViewModel.getOngoingCallList();
        mDialpadState = mInCallViewModel.getDialpadOpenState();
        mCallAudioState = mInCallViewModel.getCallAudioState();
        mAudioRoutes = mInCallViewModel.getSupportedAudioRoutes();
        mAudioRoutes.observe(this, audioRoutes -> {
            mAvailableRoutes = audioRoutes;
            mAudioRouteListItems.clear();
            for (Integer audioRoute : audioRoutes) {
                CarUiContentListItem item = new CarUiContentListItem(
                        CarUiContentListItem.Action.NONE);
                AudioRouteInfo routeInfo = getAudioRouteInfo(audioRoute);
                Drawable drawable = getResources().getDrawable(routeInfo.mIcon, null);
                drawable.setTintList(
                        getResources().getColorStateList(R.color.icon_accent_activatable, null));
                item.setIcon(drawable);
                item.setOnItemClickedListener(audioRouteItem -> onSetAudioRoute(audioRoute));
                String routeTitle = getString(routeInfo.mLabel);
                item.setTitle(withAccentColor(routeTitle));
                item.setActivated(mActiveRoute == audioRoute);
                mAudioRouteListItems.add(item);
            }
            mAudioRouteAdapter.notifyDataSetChanged();
        });
        mInCallViewModel.getAudioRoute().observe(this, this::updateViewBasedOnAudioRoute);

        mCallListLiveData = mInCallViewModel.getAllCallList();
        mCallListLiveData.observe(this, v -> updatePauseButtonEnabledState());

        mOngoingCallPairLiveData.observe(this, pair -> {
            boolean isPrimaryCallConference = pair.first != null
                    && pair.first.getDetails().hasProperty(Call.Details.PROPERTY_CONFERENCE);
            if (!isPrimaryCallConference && pair.second != null) {
                mPauseButton.setVisibility(View.GONE);
                mMergeButton.setVisibility(View.VISIBLE);
            } else {
                mPauseButton.setVisibility(View.VISIBLE);
                mMergeButton.setVisibility(View.GONE);
            }
        });
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.on_going_call_controller_bar_fragment,
                container, false);

        mMuteButton = fragmentView.findViewById(R.id.mute_button);
        mMuteButton.setOnClickListener((v) -> {
            if (v.isActivated()) {
                v.setActivated(false);
                onUnmuteMic();
            } else {
                v.setActivated(true);
                onMuteMic();
            }
        });

        mCallAudioState.observe(this, state -> mMuteButton.setActivated(state.isMuted()));

        View dialPadButton = fragmentView.findViewById(R.id.toggle_dialpad_button);
        dialPadButton.setOnClickListener(v -> mDialpadState.setValue(!mDialpadState.getValue()));
        mDialpadState.observe(this, activated -> dialPadButton.setActivated(activated));

        View endCallButton = fragmentView.findViewById(R.id.end_call_button);
        endCallButton.setOnClickListener(v -> onEndCall());

        mAudioRouteView = fragmentView.findViewById(R.id.voice_channel_view);
        mAudioRouteButton = fragmentView.findViewById(R.id.voice_channel_button);
        mAudioRouteText = fragmentView.findViewById(R.id.voice_channel_text);
        mPrimaryCallDetailLiveData.observe(this,
                primaryCallDetail -> mAudioRouteView.setEnabled(primaryCallDetail != null));
        mAudioRoutes.observe(this, audioRoutes -> {
            if (audioRoutes.size() > 1) {
                mAudioRouteView.setOnClickListener((v) -> {
                    mAudioRouteView.setActivated(true);
                    mAudioRouteSelectionDialog.show();
                });
            } else {
                mAudioRouteView.setClickable(false);
            }
        });

        mAudioRouteSelectionDialog.setOnDismissListener(
                (dialog) -> mAudioRouteView.setActivated(false));

        mMergeButton = fragmentView.findViewById(R.id.merge_button);
        mMergeButton.setOnClickListener((v) -> {
            mInCallViewModel.mergeConference();
        });

        mPauseButton = fragmentView.findViewById(R.id.pause_button);
        mPauseButton.setOnClickListener((v) -> {
            if (mPrimaryCallState == Call.STATE_ACTIVE) {
                onHoldCall();
            } else if (mPrimaryCallState == Call.STATE_HOLDING) {
                onUnholdCall();
            } else {
                L.i(TAG, "Pause button is clicked while call in %s state", mPrimaryCallState);
            }
        });

        updatePauseButtonEnabledState();

        return fragmentView;
    }

    @Override
    public void onPause() {
        super.onPause();
        L.i(TAG, "onPause");
        if (mAudioRouteSelectionDialog.isShowing()) {
            mAudioRouteSelectionDialog.dismiss();
        }
    }

    private void updateAudioRouteListItems() {
        for (int i = 0; i < mAvailableRoutes.size(); i++) {
            int audioRoute = mAvailableRoutes.get(i);
            CarUiContentListItem item = (CarUiContentListItem) mAudioRouteListItems.get(i);
            boolean isActiveRoute = audioRoute == mActiveRoute;
            item.setActivated(isActiveRoute);
        }
        mAudioRouteAdapter.notifyDataSetChanged();
    }

    private CharSequence withAccentColor(CharSequence routeTitle) {
        TextAppearanceSpan activeRouteSpan = new TextAppearanceSpan(null, 0, -1,
                getResources().getColorStateList(R.color.icon_accent_activatable, null), null);
        SpannableString spannableTitle = new SpannableString(routeTitle);
        spannableTitle.setSpan(activeRouteSpan, 0, routeTitle.length(), 0);
        return spannableTitle;
    }

    /** Set the call state and change the view for the pause button accordingly */
    private void setCallState(int callState) {
        L.d(TAG, "Call State: %s", callState);
        mPrimaryCallState = callState;
        updatePauseButtonEnabledState();
    }

    private void updatePauseButtonEnabledState() {
        boolean hasOnlyOneCall = mOngoingCallListLiveData.getValue() != null
                && mOngoingCallListLiveData.getValue().size() == 1;
        boolean shouldEnablePauseButton = hasOnlyOneCall && (mPrimaryCallState == Call.STATE_HOLDING
                || mPrimaryCallState == Call.STATE_ACTIVE);

        mPauseButton.setEnabled(shouldEnablePauseButton);
        mPauseButton.setActivated(mPrimaryCallState == Call.STATE_HOLDING);
    }

    private void onMuteMic() {
        mUiCallManager.setMuted(true);
    }

    private void onUnmuteMic() {
        mUiCallManager.setMuted(false);
    }

    private void onHoldCall() {
        if (mPrimaryCallLiveData.getValue() != null) {
            mPrimaryCallLiveData.getValue().hold();
        }
    }

    private void onUnholdCall() {
        if (mPrimaryCallLiveData.getValue() != null) {
            mPrimaryCallLiveData.getValue().unhold();
        }
    }

    private void onEndCall() {
        if (mPrimaryCallLiveData.getValue() != null) {
            mPrimaryCallLiveData.getValue().disconnect();
        }
    }

    private void onSetAudioRoute(int audioRoute) {
        Call primaryCall = mPrimaryCallLiveData.getValue();
        if (primaryCall == null) {
            // AudioRouteButton is disabled if it is null. Simply ignore it.
            return;
        }
        mUiCallManager.setAudioRoute(audioRoute, primaryCall);
        mActiveRoute = audioRoute;
        updateAudioRouteListItems();
        mAudioRouteSelectionDialog.dismiss();
    }

    private void updateViewBasedOnAudioRoute(@Nullable Integer audioRoute) {
        if (audioRoute == null || audioRoute.intValue() == 0) {
            return;
        }

        L.i(TAG, "Audio Route State: " + audioRoute);
        mActiveRoute = audioRoute;
        updateAudioRouteListItems();
        AudioRouteInfo audioRouteInfo = getAudioRouteInfo(audioRoute);
        if (mAudioRouteButton != null) {
            mAudioRouteButton.setImageResource(audioRouteInfo.mIconActivatable);
        }
        ViewUtils.setText(mAudioRouteText, audioRouteInfo.mLabel);

        updateMuteButtonEnabledState(audioRoute);
    }

    private void updateMuteButtonEnabledState(Integer audioRoute) {
        if (audioRoute == CallAudioState.ROUTE_BLUETOOTH) {
            mMuteButton.setEnabled(true);
            mMuteButton.setActivated(mUiCallManager.getMuted());
        } else {
            mMuteButton.setEnabled(false);
        }
    }

    @NonNull
    private static AudioRouteInfo getAudioRouteInfo(int route) {
        Preconditions.checkArgument(route != 0, "Unknown audio route: 0");
        AudioRouteInfo routeInfo = AUDIO_ROUTES.get(route);
        return routeInfo;
    }

    private static final class AudioRouteInfo {
        private final int mLabel;
        private final int mIcon;
        private final int mIconActivatable;

        private AudioRouteInfo(@StringRes int label,
                @DrawableRes int icon,
                @DrawableRes int iconActivatable) {
            mLabel = label;
            mIcon = icon;
            mIconActivatable = iconActivatable;
        }
    }
}
