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

import static android.media.AudioAttributes.USAGE_ALARM;
import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_ASSISTANT;
import static android.media.AudioAttributes.USAGE_NOTIFICATION;
import static android.media.AudioAttributes.USAGE_NOTIFICATION_RINGTONE;
import static android.media.AudioAttributes.USAGE_VOICE_COMMUNICATION;

import static com.google.android.car.kitchensink.R.raw.one2six;
import static com.google.android.car.kitchensink.R.raw.turnright;

import android.media.AudioAttributes;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;

import com.android.internal.widget.LinearLayoutManager;
import com.android.internal.widget.RecyclerView;

import com.google.android.car.kitchensink.R;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class AudioTransientPlayersFragment extends Fragment {

    private static final List<Integer> TRANSIENT_SOUND_USAGES = new ArrayList<>(Arrays.asList(
            USAGE_ASSISTANCE_NAVIGATION_GUIDANCE, USAGE_ASSISTANT, USAGE_NOTIFICATION_RINGTONE,
            USAGE_VOICE_COMMUNICATION, USAGE_ALARM, USAGE_NOTIFICATION));

    private final List<AudioPlayer> mSystemPlayers =
            new ArrayList<>(TRANSIENT_SOUND_USAGES.size());

    private RecyclerView mRecyclerView;
    private PlayerAdapter mSystemPlayerAdapter;

    AudioTransientPlayersFragment() {
    }
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.audio_player_tab, container, false);
        initPlayers();

        mRecyclerView = view.findViewById(R.id.players_view);

        mRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));

        mSystemPlayerAdapter = new PlayerAdapter(mSystemPlayers);
        mRecyclerView.setAdapter(mSystemPlayerAdapter);
        mRecyclerView.scrollToPosition(0);

        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        for (int index = 0; index < mSystemPlayers.size(); index++) {
            mSystemPlayers.get(index).stop();
        }
        mSystemPlayers.clear();
    }

    private void initPlayers() {
        for (int index = 0; index < TRANSIENT_SOUND_USAGES.size(); index++) {
            int usage = TRANSIENT_SOUND_USAGES.get(index);
            mSystemPlayers.add(getCarSoundsPlayer(usage));
        }
    }

    private AudioPlayer getCarSoundsPlayer(int usage) {
        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(usage).build();
        return new AudioPlayer(getContext(), getPlayerResource(usage), attributes);
    }

    private int getPlayerResource(int usage) {
        if (usage == USAGE_ASSISTANCE_NAVIGATION_GUIDANCE) {
            return turnright;
        }
        return one2six;
    }
}
