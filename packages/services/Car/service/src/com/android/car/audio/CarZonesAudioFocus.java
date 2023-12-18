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

package com.android.car.audio;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.annotation.UserIdInt;
import android.car.media.CarAudioManager;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioFocusInfo;
import android.media.AudioManager;
import android.media.audiopolicy.AudioPolicy;
import android.os.Bundle;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.Preconditions;
import com.android.server.utils.Slogf;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * Implements {@link AudioPolicy.AudioPolicyFocusListener}
 *
 * <p><b>Note:</b> Manages audio focus on a per zone basis.
 */
final class CarZonesAudioFocus extends AudioPolicy.AudioPolicyFocusListener {
    private static final String TAG = CarLog.tagFor(CarZonesAudioFocus.class);

    private final CarFocusCallback mCarFocusCallback;
    private final boolean mDelayedFocusEnabled;
    private CarAudioService mCarAudioService; // Dynamically assigned just after construction
    private AudioPolicy mAudioPolicy; // Dynamically assigned just after construction

    private final SparseArray<CarAudioFocus> mFocusZones;

    public static CarZonesAudioFocus createCarZonesAudioFocus(@NonNull AudioManager audioManager,
            @NonNull PackageManager packageManager,
            @NonNull SparseArray<CarAudioZone> carAudioZones,
            @NonNull CarAudioSettings carAudioSettings,
            boolean enableDelayedAudioFocus,
            CarFocusCallback carFocusCallback) {
        Objects.requireNonNull(audioManager, "AudioManager cannot be null");
        Objects.requireNonNull(packageManager, "PackageManager cannot be null");
        Objects.requireNonNull(carAudioZones, "CarAudioZones cannot be null");
        Preconditions.checkArgument(carAudioZones.size() != 0,
                "There must be a minimum of one audio zone");
        Objects.requireNonNull(carAudioSettings, "CarAudioSettings cannot be null");

        SparseArray<CarAudioFocus> audioFocusPerZone = new SparseArray<>();

        //Create focus for all the zones
        for (int i = 0; i < carAudioZones.size(); i++) {
            CarAudioZone audioZone = carAudioZones.valueAt(i);
            int audioZoneId = audioZone.getId();
            Slogf.d(TAG, "Adding new zone %d", audioZoneId);
            CarAudioFocus zoneFocusListener =
                    new CarAudioFocus(audioManager, packageManager,
                            new FocusInteraction(carAudioSettings), enableDelayedAudioFocus);
            audioFocusPerZone.put(audioZoneId, zoneFocusListener);
        }
        return new CarZonesAudioFocus(audioFocusPerZone, enableDelayedAudioFocus, carFocusCallback);
    }

    @VisibleForTesting
    CarZonesAudioFocus(SparseArray<CarAudioFocus> focusZones,
            boolean enableDelayedAudioFocus,
            CarFocusCallback carFocusCallback) {
        mFocusZones = focusZones;
        mDelayedFocusEnabled = enableDelayedAudioFocus;
        mCarFocusCallback = carFocusCallback;
    }

    /**
     * Query the current list of focus loser in zoneId for uid
     * @param uid uid to query for current focus losers
     * @param zoneId zone id to query for info
     * @return list of current focus losers for uid
     */
    ArrayList<AudioFocusInfo> getAudioFocusLosersForUid(int uid, int zoneId) {
        CarAudioFocus focus = mFocusZones.get(zoneId);
        return focus.getAudioFocusLosersForUid(uid);
    }

    /**
     * Query the current list of focus holders in zoneId for uid
     * @param uid uid to query for current focus holders
     * @param zoneId zone id to query
     * @return list of current focus holders that for uid
     */
    ArrayList<AudioFocusInfo> getAudioFocusHoldersForUid(int uid, int zoneId) {
        CarAudioFocus focus = mFocusZones.get(zoneId);
        return focus.getAudioFocusHoldersForUid(uid);
    }

    /**
     * For each entry in list, transiently lose focus
     * @param afiList list of audio focus entries
     * @param zoneId zone id where focus should should be lost
     */
    void transientlyLoseInFocusInZone(@NonNull ArrayList<AudioFocusInfo> afiList, int zoneId) {
        CarAudioFocus focus = mFocusZones.get(zoneId);

        for (AudioFocusInfo info : afiList) {
            focus.removeAudioFocusInfoAndTransientlyLoseFocus(info);
        }
    }

    int reevaluateAndRegainAudioFocus(AudioFocusInfo afi) {
        int zoneId = getAudioZoneIdForAudioFocusInfo(afi);
        return getCarAudioFocusForZoneId(zoneId).reevaluateAndRegainAudioFocus(afi);
    }

    /**
     * Sets the owning policy of the audio focus
     *
     * <p><b>Note:</b> This has to happen after the construction to avoid a chicken and egg
     * problem when setting up the AudioPolicy which must depend on this object.

     * @param carAudioService owning car audio service
     * @param parentPolicy owning parent car audio policy
     */
    void setOwningPolicy(CarAudioService carAudioService, AudioPolicy parentPolicy) {
        mAudioPolicy = parentPolicy;
        mCarAudioService = carAudioService;

        for (int i = 0; i < mFocusZones.size(); i++) {
            mFocusZones.valueAt(i).setOwningPolicy(mAudioPolicy);
        }
    }

    void setRestrictFocus(boolean isFocusRestricted) {
        int[] zoneIds = new int[mFocusZones.size()];
        for (int i = 0; i < mFocusZones.size(); i++) {
            zoneIds[i] = mFocusZones.keyAt(i);
            mFocusZones.valueAt(i).setRestrictFocus(isFocusRestricted);
        }
        notifyFocusCallback(zoneIds);
    }

    @Override
    public void onAudioFocusRequest(AudioFocusInfo afi, int requestResult) {
        int zoneId = getAudioZoneIdForAudioFocusInfo(afi);
        getCarAudioFocusForZoneId(zoneId).onAudioFocusRequest(afi, requestResult);
        notifyFocusCallback(new int[]{zoneId});
    }

    /**
     * @see AudioManager#abandonAudioFocus(AudioManager.OnAudioFocusChangeListener, AudioAttributes)
     * Note that we'll get this call for a focus holder that dies while in the focus stack, so
     * we don't need to watch for death notifications directly.
     */
    @Override
    public void onAudioFocusAbandon(AudioFocusInfo afi) {
        int zoneId = getAudioZoneIdForAudioFocusInfo(afi);
        getCarAudioFocusForZoneId(zoneId).onAudioFocusAbandon(afi);
        notifyFocusCallback(new int[]{zoneId});
    }

    @NonNull
    private CarAudioFocus getCarAudioFocusForZoneId(int zoneId) {
        return mFocusZones.get(zoneId);
    }

    private int getAudioZoneIdForAudioFocusInfo(AudioFocusInfo afi) {
        int zoneId = mCarAudioService.getZoneIdForUid(afi.getClientUid());

        // If the bundle attribute for AUDIOFOCUS_EXTRA_REQUEST_ZONE_ID has been assigned
        // Use zone id from that instead.
        Bundle bundle = afi.getAttributes().getBundle();

        if (bundle != null) {
            int bundleZoneId =
                    bundle.getInt(CarAudioManager.AUDIOFOCUS_EXTRA_REQUEST_ZONE_ID,
                            -1);
            // check if the zone id is within current zones bounds
            if (mCarAudioService.isAudioZoneIdValid(bundleZoneId)) {
                Slogf.d(TAG, "getFocusForAudioFocusInfo valid zoneId %d with bundle request for"
                        + " client %s", bundleZoneId, afi.getClientId());
                zoneId = bundleZoneId;
            } else {
                Slogf.w(TAG, "getFocusForAudioFocusInfo invalid zoneId %d with bundle request for "
                                + "client %s, dispatching focus request to zoneId %d", bundleZoneId,
                        afi.getClientId(), zoneId);
            }
        }

        return zoneId;
    }

    private void notifyFocusCallback(int[] zoneIds) {
        if (mCarFocusCallback == null) {
            return;
        }
        SparseArray<List<AudioFocusInfo>> focusHoldersByZoneId = new SparseArray<>();
        for (int i = 0; i < zoneIds.length; i++) {
            int zoneId = zoneIds[i];
            List<AudioFocusInfo> focusHolders = mFocusZones.get(zoneId).getAudioFocusHolders();
            focusHoldersByZoneId.put(zoneId, focusHolders);
        }

        mCarFocusCallback.onFocusChange(zoneIds, focusHoldersByZoneId);
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    void dump(IndentingPrintWriter writer) {
        writer.println("*CarZonesAudioFocus*");
        writer.increaseIndent();
        writer.printf("Delayed Focus Enabled: %b\n", mDelayedFocusEnabled);
        writer.printf("Has Focus Callback: %b\n", mCarFocusCallback != null);
        writer.println("Car Zones Audio Focus Listeners:");
        writer.increaseIndent();
        for (int i = 0; i < mFocusZones.size(); i++) {
            writer.printf("Zone Id: %d\n", mFocusZones.keyAt(i));
            writer.increaseIndent();
            mFocusZones.valueAt(i).dump(writer);
            writer.decreaseIndent();
        }
        writer.decreaseIndent();
        writer.decreaseIndent();
    }

    public void updateUserForZoneId(int audioZoneId, @UserIdInt int userId) {
        Preconditions.checkArgument(mCarAudioService.isAudioZoneIdValid(audioZoneId),
                "Invalid zoneId %d", audioZoneId);
        mFocusZones.get(audioZoneId).getFocusInteraction().setUserIdForSettings(userId);
    }

    /**
     * Callback to get notified of the active focus holders after any focus request or abandon  call
     */
    public interface CarFocusCallback {
        /**
         * Called after a focus request or abandon call is handled.
         *
         * @param audioZoneIds IDs of the zones where the changes took place
         * @param focusHoldersByZoneId sparse array by zone ID, where each value is a list of
         * {@link AudioFocusInfo}s holding focus in specified audio zone
         */
        void onFocusChange(int[] audioZoneIds,
                @NonNull SparseArray<List<AudioFocusInfo>> focusHoldersByZoneId);
    }
}
