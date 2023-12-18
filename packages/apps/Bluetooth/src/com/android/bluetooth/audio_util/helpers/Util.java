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

package com.android.bluetooth.audio_util;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.browse.MediaBrowser.MediaItem;
import android.media.session.MediaSession;
import android.os.Bundle;
import android.util.Log;

import com.android.bluetooth.R;

import java.util.ArrayList;
import java.util.List;

class Util {
    public static String TAG = "audio_util.Util";
    public static boolean DEBUG = false;

    private static final String GPM_KEY = "com.google.android.music.mediasession.music_metadata";

    // TODO (apanicke): Remove this prefix later, for now it makes debugging easier.
    public static final String NOW_PLAYING_PREFIX = "NowPlayingId";

    /**
     * Get an empty set of Metadata
     */
    public static final Metadata empty_data() {
        Metadata ret = new Metadata();
        ret.mediaId = "Not Provided";
        ret.title = "Not Provided";
        ret.artist = "";
        ret.album = "";
        ret.genre = "";
        ret.trackNum = "1";
        ret.numTracks = "1";
        ret.duration = "0";
        ret.image = null;
        return ret;
    }

    /**
     * Get whether or not Bluetooth is configured to support URI images or not.
     *
     * Note that creating URI images will dramatically increase memory usage.
     */
    public static boolean areUriImagesSupported(Context context) {
        if (context == null) return false;
        return context.getResources().getBoolean(R.bool.avrcp_target_cover_art_uri_images);
    }

    /**
     * Translate a bundle of MediaMetadata keys to audio_util's Metadata
     */
    public static Metadata toMetadata(Context context, Bundle bundle) {
        Metadata.Builder builder = new Metadata.Builder();
        return builder.useContext(context).useDefaults().fromBundle(bundle).build();
    }

    /**
     * Translate a MediaDescription to audio_util's Metadata
     */
    public static Metadata toMetadata(Context context, MediaDescription desc) {
        // Find GPM_KEY data if it exists
        MediaMetadata data = null;
        Bundle extras = (desc != null ? desc.getExtras() : null);
        if (extras != null && extras.containsKey(GPM_KEY)) {
            data = (MediaMetadata) extras.get(GPM_KEY);
        }

        Metadata.Builder builder = new Metadata.Builder();
        return builder.useContext(context).useDefaults().fromMediaDescription(desc)
                .fromMediaMetadata(data).build();
    }

    /**
     * Translate a MediaItem to audio_util's Metadata
     */
    public static Metadata toMetadata(Context context, MediaItem item) {
        Metadata.Builder builder = new Metadata.Builder();
        return builder.useContext(context).useDefaults().fromMediaItem(item).build();
    }

    /**
     * Translate a MediaSession.QueueItem to audio_util's Metadata
     */
    public static Metadata toMetadata(Context context, MediaSession.QueueItem item) {
        Metadata.Builder builder = new Metadata.Builder().useDefaults().fromQueueItem(item);
        // For Queue Items, the Media Id will always be just its Queue ID
        // We don't need to use its actual ID since we don't promise UIDS being valid
        // between a file system and it's now playing list.
        if (item != null) builder.setMediaId(NOW_PLAYING_PREFIX + item.getQueueId());
        return builder.build();
    }

    /**
     * Translate a MediaMetadata to audio_util's Metadata
     */
    public static Metadata toMetadata(Context context, MediaMetadata data) {
        Metadata.Builder builder = new Metadata.Builder();
        // This will always be currsong. The AVRCP service will overwrite the mediaId if it needs to
        // TODO (apanicke): Remove when the service is ready, right now it makes debugging much more
        // convenient
        return builder.useContext(context).useDefaults().fromMediaMetadata(data)
                .setMediaId("currsong").build();
    }

    /**
     * Translate a list of MediaSession.QueueItem to a list of audio_util's Metadata
     */
    public static List<Metadata> toMetadataList(Context context,
            List<MediaSession.QueueItem> items) {
        ArrayList<Metadata> list = new ArrayList<Metadata>();

        if (items == null) return list;

        for (int i = 0; i < items.size(); i++) {
            Metadata data = toMetadata(context, items.get(i));
            data.trackNum = "" + (i + 1);
            data.numTracks = "" + items.size();
            list.add(data);
        }

        return list;
    }

    // Helper method to close a list of ListItems so that if the callee wants
    // to mutate the list they can do it without affecting any internally cached info
    public static List<ListItem> cloneList(List<ListItem> list) {
        List<ListItem> clone = new ArrayList<ListItem>(list.size());
        for (ListItem item : list) clone.add(item.clone());
        return clone;
    }

    public static String getDisplayName(Context context, String packageName) {
        try {
            PackageManager manager = context.getPackageManager();
            return manager.getApplicationLabel(manager.getApplicationInfo(packageName, 0))
                    .toString();
        } catch (Exception e) {
            Log.w(TAG, "Name Not Found using package name: " + packageName);
            return packageName;
        }
    }
}
