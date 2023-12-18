/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.google.android.car.kitchensink.volume;

import static android.car.media.CarAudioManager.AUDIO_FEATURE_VOLUME_GROUP_MUTING;
import static android.media.AudioManager.FLAG_PLAY_SOUND;

import android.car.media.CarAudioManager;
import android.media.AudioAttributes;
import android.media.AudioAttributes.AttributeUsage;
import android.media.AudioManager;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import androidx.fragment.app.Fragment;

import com.android.internal.annotations.GuardedBy;

import com.google.android.car.kitchensink.R;
import com.google.android.car.kitchensink.volume.VolumeTestFragment.CarAudioZoneVolumeInfo;

public final class CarAudioZoneVolumeFragment extends Fragment {
    private static final String TAG = "CarVolumeTest."
            + CarAudioZoneVolumeFragment.class.getSimpleName();
    private static final boolean DEBUG = true;

    private static final int MSG_VOLUME_CHANGED = 0;
    private static final int MSG_REQUEST_FOCUS = 1;
    private static final int MSG_FOCUS_CHANGED = 2;
    private static final int MSG_STOP_RINGTONE = 3;
    private static final long RINGTONE_STOP_TIME_MS = 3_000;

    private final int mZoneId;
    private final Object mLock = new Object();
    private final CarAudioManager mCarAudioManager;
    private final AudioManager mAudioManager;
    private CarAudioZoneVolumeInfo[] mVolumeInfos =
            new CarAudioZoneVolumeInfo[0];
    private final Handler mHandler = new VolumeHandler();

    private CarAudioZoneVolumeAdapter mCarAudioZoneVolumeAdapter;
    private final SparseIntArray mGroupIdIndexMap = new SparseIntArray();

    @GuardedBy("mLock")
    private Ringtone mRingtone;

    public void sendVolumeChangedMessage(int groupId, int flags) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_VOLUME_CHANGED, groupId, flags));
    }

    private class VolumeHandler extends Handler {
        private AudioFocusListener mFocusListener;

        @Override
        public void handleMessage(Message msg) {
            if (DEBUG) {
                Log.d(TAG, "zone " + mZoneId + " handleMessage : " + getMessageName(msg));
            }
            switch (msg.what) {
                case MSG_VOLUME_CHANGED:
                    initVolumeInfo();
                    playRingtoneForGroup(msg.arg1, msg.arg2);
                    break;
                case MSG_STOP_RINGTONE:
                    stopRingtone();
                    break;
                case MSG_REQUEST_FOCUS:
                    int groupId = msg.arg1;
                    if (mFocusListener != null) {
                        mAudioManager.abandonAudioFocus(mFocusListener);
                        mVolumeInfos[mGroupIdIndexMap.get(groupId)].hasAudioFocus = false;
                        mCarAudioZoneVolumeAdapter.notifyDataSetChanged();
                    }

                    mFocusListener = new AudioFocusListener(groupId);
                    mAudioManager.requestAudioFocus(mFocusListener, groupId,
                            AudioManager.AUDIOFOCUS_GAIN);
                    break;
                case MSG_FOCUS_CHANGED:
                    int focusGroupId = msg.arg1;
                    mVolumeInfos[mGroupIdIndexMap.get(focusGroupId)].hasAudioFocus = true;
                    mCarAudioZoneVolumeAdapter.refreshVolumes(mVolumeInfos);
                    break;
                default :
                    Log.wtf(TAG,"VolumeHandler handleMessage called with unknown message"
                            + msg.what);

            }
        }
    }

    public CarAudioZoneVolumeFragment(int zoneId, CarAudioManager carAudioManager,
            AudioManager audioManager) {
        mZoneId = zoneId;
        mCarAudioManager = carAudioManager;
        mAudioManager = audioManager;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if (DEBUG) {
            Log.d(TAG, "onCreateView " + mZoneId);
        }
        View v = inflater.inflate(R.layout.zone_volume_tab, container, false);
        ListView volumeListView = v.findViewById(R.id.volume_list);
        mCarAudioZoneVolumeAdapter =
                new CarAudioZoneVolumeAdapter(getContext(), R.layout.volume_item, mVolumeInfos,
                        this, mCarAudioManager.isAudioFeatureEnabled(
                        AUDIO_FEATURE_VOLUME_GROUP_MUTING));
        initVolumeInfo();
        volumeListView.setAdapter(mCarAudioZoneVolumeAdapter);
        return v;
    }

    void initVolumeInfo() {
        int volumeGroupCount = mCarAudioManager.getVolumeGroupCount(mZoneId);
        mVolumeInfos = new CarAudioZoneVolumeInfo[volumeGroupCount + 1];
        mGroupIdIndexMap.clear();
        CarAudioZoneVolumeInfo titlesInfo = new CarAudioZoneVolumeInfo();
        titlesInfo.id = "Group id";
        titlesInfo.currentGain = "Current";
        titlesInfo.maxGain = "Max";
        mVolumeInfos[0] = titlesInfo;

        int i = 1;
        for (int groupId = 0; groupId < volumeGroupCount; groupId++) {
            CarAudioZoneVolumeInfo volumeInfo = new CarAudioZoneVolumeInfo();
            mGroupIdIndexMap.put(groupId, i);
            volumeInfo.groupId = groupId;
            volumeInfo.id = String.valueOf(groupId);
            int current = mCarAudioManager.getGroupVolume(mZoneId, groupId);
            int max = mCarAudioManager.getGroupMaxVolume(mZoneId, groupId);
            volumeInfo.currentGain = String.valueOf(current);
            volumeInfo.maxGain = String.valueOf(max);
            volumeInfo.isMuted = mCarAudioManager.isVolumeGroupMuted(mZoneId, groupId);

            mVolumeInfos[i] = volumeInfo;
            if (DEBUG)
            {
                Log.d(TAG, groupId + " max: " + volumeInfo.maxGain + " current: "
                        + volumeInfo.currentGain + " is muted " + volumeInfo.isMuted);
            }
            i++;
        }
        mCarAudioZoneVolumeAdapter.refreshVolumes(mVolumeInfos);
    }

    public void adjustVolumeByOne(int groupId, boolean up) {
        if (mCarAudioManager == null) {
            Log.e(TAG, "CarAudioManager is null");
            return;
        }
        int current = mCarAudioManager.getGroupVolume(mZoneId, groupId);
        int volume = current + (up ? 1 : -1);
        mCarAudioManager.setGroupVolume(mZoneId, groupId, volume, AudioManager.FLAG_SHOW_UI);
        if (DEBUG) {
            Log.d(TAG, "Set group " + groupId + " volume " + volume + " in audio zone "
                    + mZoneId);
        }
    }

    public void toggleMute(int groupId) {
        if (mCarAudioManager == null) {
            Log.e(TAG, "CarAudioManager is null");
            return;
        }
        boolean isMuted = mCarAudioManager.isVolumeGroupMuted(mZoneId, groupId);
        mCarAudioManager.setVolumeGroupMute(mZoneId, groupId, !isMuted, AudioManager.FLAG_SHOW_UI);
        if (DEBUG) {
            Log.d(TAG, "Set group mute " + groupId + " mute " + !isMuted + " in audio zone "
                    + mZoneId);
        }
    }

    public void requestFocus(int groupId) {
        // Automatic volume change only works for primary audio zone.
        if (mZoneId == CarAudioManager.PRIMARY_AUDIO_ZONE) {
            mHandler.sendMessage(mHandler
                    .obtainMessage(MSG_REQUEST_FOCUS, groupId, /* arg2= */ 0));
        }
    }

    private void playRingtoneForGroup(int groupId, int flags) {
        if (DEBUG) {
            Log.d(TAG, "playRingtoneForGroup(" + groupId + ") in zone " + mZoneId);
        }

        if ((flags & FLAG_PLAY_SOUND) == 0) {
            return;
        }

        int usage = mCarAudioManager.getUsagesForVolumeGroupId(mZoneId, groupId)[0];
        if (isRingtoneActiveForUsage(usage)) {
            return;
        }

        mHandler.removeMessages(MSG_STOP_RINGTONE);

        stopRingtone();
        startRingtone(usage);

        mHandler.sendEmptyMessageDelayed(MSG_STOP_RINGTONE, RINGTONE_STOP_TIME_MS);
    }

    private void startRingtone(@AttributeUsage int usage) {
        if (DEBUG) {
            Log.d(TAG, "Start ringtone for zone " + mZoneId + " and usage "
                    + AudioAttributes.usageToString(usage));
        }
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(usage)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();

        Uri uri = RingtoneManager.getActualDefaultRingtoneUri(getContext(),
                AudioAttributes.toLegacyStreamType(attributes));

        Ringtone ringtone =
                RingtoneManager.getRingtone(mCarAudioZoneVolumeAdapter.getContext(), uri);
        ringtone.setAudioAttributes(attributes);
        ringtone.setLooping(true);

        ringtone.play();

        synchronized (mLock) {
            mRingtone = ringtone;
        }
    }

    private void stopRingtone() {
        synchronized (mLock) {
            if (mRingtone == null) {
                return;
            }
            if (mRingtone.isPlaying()) {
                mRingtone.stop();
            }
            mRingtone = null;
        }
    }

    boolean isRingtoneActiveForUsage(@AttributeUsage int usage) {
        synchronized (mLock) {
            return mRingtone != null && mRingtone.isPlaying()
                    && mRingtone.getAudioAttributes().getUsage() == usage;
        }
    }

    private class AudioFocusListener implements AudioManager.OnAudioFocusChangeListener {
        private final int mGroupId;
        AudioFocusListener(int groupId) {
            mGroupId = groupId;
        }
        @Override
        public void onAudioFocusChange(int focusChange) {
            if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
                mHandler.sendMessage(mHandler
                        .obtainMessage(MSG_FOCUS_CHANGED, mGroupId, /* arg2= */ 0));
            } else {
                Log.e(TAG, "Audio focus request failed");
            }
        }
    }
}
