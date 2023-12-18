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

package com.android.car.audio;

import static com.android.car.audio.CarAudioContext.ALARM;
import static com.android.car.audio.CarAudioContext.ANNOUNCEMENT;
import static com.android.car.audio.CarAudioContext.CALL;
import static com.android.car.audio.CarAudioContext.CALL_RING;
import static com.android.car.audio.CarAudioContext.EMERGENCY;
import static com.android.car.audio.CarAudioContext.INVALID;
import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;
import static com.android.car.audio.CarAudioContext.NOTIFICATION;
import static com.android.car.audio.CarAudioContext.SAFETY;
import static com.android.car.audio.CarAudioContext.SYSTEM_SOUND;
import static com.android.car.audio.CarAudioContext.VEHICLE_STATUS;
import static com.android.car.audio.CarAudioContext.VOICE_COMMAND;

import android.media.AudioFocusInfo;
import android.util.SparseArray;

import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

final class CarDuckingUtils {
    @VisibleForTesting
    static final SparseArray<int[]> sContextsToDuck = new SparseArray<>();

    static {
        // INVALID ducks nothing
        sContextsToDuck.append(INVALID, new int[0]);
        // MUSIC ducks nothing
        sContextsToDuck.append(MUSIC, new int[0]);
        sContextsToDuck.append(NAVIGATION, new int[]{
                MUSIC,
                CALL_RING,
                CALL,
                ALARM,
                NOTIFICATION,
                SYSTEM_SOUND,
                VEHICLE_STATUS,
                ANNOUNCEMENT
        });
        sContextsToDuck.append(VOICE_COMMAND, new int[]{
                CALL_RING
        });
        sContextsToDuck.append(CALL_RING, new int[0]);
        sContextsToDuck.append(CALL, new int[]{
                CALL_RING,
                ALARM,
                NOTIFICATION,
                VEHICLE_STATUS
        });
        sContextsToDuck.append(ALARM, new int[]{
                MUSIC
        });
        sContextsToDuck.append(NOTIFICATION, new int[]{
                MUSIC,
                ALARM,
                ANNOUNCEMENT
        });
        sContextsToDuck.append(SYSTEM_SOUND, new int[]{
                MUSIC,
                ALARM,
                ANNOUNCEMENT
        });
        sContextsToDuck.append(EMERGENCY, new int[]{
                CALL
        });
        sContextsToDuck.append(SAFETY, new int[]{
                MUSIC,
                NAVIGATION,
                VOICE_COMMAND,
                CALL_RING,
                CALL,
                ALARM,
                NOTIFICATION,
                SYSTEM_SOUND,
                VEHICLE_STATUS,
                ANNOUNCEMENT
        });
        sContextsToDuck.append(VEHICLE_STATUS, new int[]{
                MUSIC,
                CALL_RING,
                ANNOUNCEMENT
        });
        // ANNOUNCEMENT ducks nothing
        sContextsToDuck.append(ANNOUNCEMENT, new int[0]);
    }

    private CarDuckingUtils() {
    }

    static int[] getUsagesHoldingFocus(List<AudioFocusInfo> focusHolders) {
        Set<Integer> uniqueUsages = new HashSet<>();
        for (AudioFocusInfo focusInfo : focusHolders) {
            uniqueUsages.add(focusInfo.getAttributes().getSystemUsage());
        }

        int index = 0;
        int[] usagesHoldingFocus = new int[uniqueUsages.size()];
        for (int usage : uniqueUsages) {
            usagesHoldingFocus[index] = usage;
            index++;
        }
        return usagesHoldingFocus;
    }

    static List<String> getAddressesToDuck(int[] usages, CarAudioZone zone) {
        Set<Integer> uniqueContexts = CarAudioContext.getUniqueContextsForUsages(usages);
        Set<Integer> contextsToDuck = getContextsToDuck(uniqueContexts);
        Set<String> addressesToDuck = getAddressesForContexts(contextsToDuck, zone);

        Set<Integer> unduckedContexts = getUnduckedContexts(uniqueContexts, contextsToDuck);
        Set<String> unduckedAddresses = getAddressesForContexts(unduckedContexts, zone);

        // We should not duck any device that's associated with an unducked context holding focus.
        addressesToDuck.removeAll(unduckedAddresses);
        return new ArrayList<>(addressesToDuck);
    }

    static List<String> getAddressesToUnduck(List<String> addressesToDuck,
            List<String> oldAddressesToDuck) {
        List<String> addressesToUnduck = new ArrayList<>(oldAddressesToDuck);
        addressesToUnduck.removeAll(addressesToDuck);
        return addressesToUnduck;
    }

    private static Set<Integer> getUnduckedContexts(Set<Integer> contexts,
            Set<Integer> duckedContexts) {
        Set<Integer> unduckedContexts = new HashSet<>(contexts);
        unduckedContexts.removeAll(duckedContexts);
        return unduckedContexts;
    }

    private static Set<String> getAddressesForContexts(Set<Integer> contexts, CarAudioZone zone) {
        Set<String> addresses = new HashSet<>();
        for (Integer context : contexts) {
            addresses.add(zone.getAddressForContext(context));
        }
        return addresses;
    }

    private static Set<Integer> getContextsToDuck(Set<Integer> contexts) {
        Set<Integer> contextsToDuck = new HashSet<>();

        for (Integer context : contexts) {
            int[] duckedContexts = sContextsToDuck.get(context);
            for (int i = 0; i < duckedContexts.length; i++) {
                contextsToDuck.add(duckedContexts[i]);
            }
        }

        // Reduce contextsToDuck down to subset of contexts currently holding focus
        contextsToDuck.retainAll(contexts);
        return contextsToDuck;
    }
}
