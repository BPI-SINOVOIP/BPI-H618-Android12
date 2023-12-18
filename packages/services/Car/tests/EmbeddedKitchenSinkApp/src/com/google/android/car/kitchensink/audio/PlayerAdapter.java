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

package com.google.android.car.kitchensink.audio;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

import static com.google.android.car.kitchensink.audio.AudioPlayer.PLAYER_STATE_COMPLETED;
import static com.google.android.car.kitchensink.audio.AudioPlayer.PLAYER_STATE_DELAYED;
import static com.google.android.car.kitchensink.audio.AudioPlayer.PLAYER_STATE_PAUSED;
import static com.google.android.car.kitchensink.audio.AudioPlayer.PLAYER_STATE_STARTED;
import static com.google.android.car.kitchensink.audio.AudioPlayer.PLAYER_STATE_STOPPED;
import static com.google.android.car.kitchensink.audio.AudioTestFragment.getAudioLogTag;

import android.media.AudioAttributes;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.internal.widget.RecyclerView;

import com.google.android.car.kitchensink.R;
import com.google.android.car.kitchensink.audio.AudioPlayer.AudioPlayerState;

import java.util.List;

public final class PlayerAdapter extends RecyclerView.Adapter<PlayerAdapter.ViewHolder> {
    private static final String TAG = getAudioLogTag(PlayerAdapter.class);
    private static final long STOP_DELAY_TIME_MS = 3_000;

    private final List<AudioPlayer> mPlayers;
    private final Handler mHandler;

    public PlayerAdapter(List<AudioPlayer> players) {
        mHandler = new Handler(Looper.getMainLooper());
        mPlayers = players;
    }

    public final class ViewHolder extends RecyclerView.ViewHolder {
        private final TextView mUsageTextView;
        private final TextView mStatusTextView;
        private AudioPlayer mPlayer;

        private ViewHolder(View v) {
            super(v);
            mUsageTextView = v.findViewById(R.id.player_usage);
            mStatusTextView = v.findViewById(R.id.player_status);
            v.findViewById(R.id.player_start).setOnClickListener(e -> startPlayer());
            v.findViewById(R.id.player_play_once).setOnClickListener(e -> {
                startPlayer();
                mHandler.postDelayed(() -> stopPlayer(), STOP_DELAY_TIME_MS);
            });
            v.findViewById(R.id.player_stop).setOnClickListener(e -> stopPlayer());
        }

        private void setPlayer(AudioPlayer audioPlayer) {
            mPlayer = audioPlayer;
            mUsageTextView.setText(AudioAttributes.usageToString(mPlayer.getUsage()));
        }

        private void startPlayer() {
            mPlayer.start(true, true, AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK, state -> {
                handleStateChange(state);
            });
        }

        private void handleStateChange(@AudioPlayerState int state) {
            switch (state) {
                case PLAYER_STATE_STARTED:
                    mStatusTextView.setText(R.string.player_started);
                    return;
                case PLAYER_STATE_DELAYED:
                    mStatusTextView.setText(R.string.player_delayed);
                    return;
                case PLAYER_STATE_PAUSED:
                    mStatusTextView.setText(R.string.player_paused);
                    return;
                case PLAYER_STATE_STOPPED:
                case PLAYER_STATE_COMPLETED:
                default:
                    mStatusTextView.setText(R.string.player_stopped);
            }
        }

        private void stopPlayer() {
            mPlayer.stop();
            mStatusTextView.setText(R.string.player_stopped);
        }
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType) {
        View playerView = LayoutInflater.from(viewGroup.getContext())
                .inflate(R.layout.audio_player_item, viewGroup, false);

        return new ViewHolder(playerView);
    }


    @Override
    public void onBindViewHolder(ViewHolder viewHolder, final int position) {
        viewHolder.setPlayer(mPlayers.get(position));
    }

    @Override
    public int getItemCount() {
        return mPlayers.size();
    }
}
