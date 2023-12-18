/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.music;

import android.app.Activity;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.media.AudioManager;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.browse.MediaBrowser;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.Gravity;
import android.widget.*;
import com.android.music.utils.LogHelper;
import com.android.music.utils.MediaIDHelper;
import com.android.music.MusicApp;

import java.util.ArrayList;
import java.util.List;
import android.os.Environment;
import java.io.File;
import android.net.Uri;
import android.content.DialogInterface;
import android.app.AlertDialog;
/*
This activity shows when there is a need for
 1. Songs tab [withtab = true]
 2. Browse songs within an album [withtab = false]
 3. Browse songs within a playlist [withtab = false]
 4. Browse songs within now playing queue [withtab = false]
 */
public class TrackBrowserActivity extends ListActivity implements MusicApp.MountCallback {
    private static final String TAG = LogHelper.makeLogTag(TrackBrowserActivity.class);
    private static final MediaBrowser.MediaItem DEFAULT_PARENT_ITEM =
            new MediaBrowser.MediaItem(new MediaDescription.Builder()
                                               .setMediaId(MediaIDHelper.MEDIA_ID_MUSICS_BY_SONG)
                                               .setTitle("Songs")
                                               .build(),
                    MediaBrowser.MediaItem.FLAG_BROWSABLE);

    // Underlining ListView of this Activity
    private ListView mTrackList;
    // The mediaId to be used for subscribing for children using the MediaBrowser.
    private MediaBrowser.MediaItem mParentItem;
    private MediaBrowser mMediaBrowser;
    private TrackBrowseAdapter mBrowseListAdapter;
    private boolean mWithTabs;
    private boolean mRefreshMetadata= true;

    // init media scanner
    private Button scannerBtn;
    private String SCANNERPATH = "/sdcard/";
    private long updateTime = 0;
    private long UPDATETIME = 3 * 60 * 1000;
    private long toastShowTime = 0;
    private long TOASTUPDATETIME = 2 * 1000;

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle icicle) {
        Log.d(TAG, "onCreate()");
        super.onCreate(icicle);
        // Process past states
        Intent intent = getIntent();
        if (icicle != null) {
            LogHelper.d(TAG, "Launch by saved instance state");
            mParentItem = icicle.getParcelable(MusicUtils.TAG_PARENT_ITEM);
            mWithTabs = icicle.getBoolean(MusicUtils.TAG_WITH_TABS);
            //MusicUtils.updateNowPlaying(this);
        } else if (intent != null) {
            LogHelper.d(TAG, "Launch by intent");
            mParentItem = intent.getParcelableExtra(MusicUtils.TAG_PARENT_ITEM);
            mWithTabs = intent.getBooleanExtra(MusicUtils.TAG_WITH_TABS, false);
        }
        if (mParentItem == null) {
            LogHelper.d(TAG, "Launch by default parameters");
            mParentItem = DEFAULT_PARENT_ITEM;
            mWithTabs = true;
        }
        if (mWithTabs) {
            requestWindowFeature(Window.FEATURE_NO_TITLE);
        }
        setTitle(mParentItem.getDescription().getTitle());
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        // Init layout
        LogHelper.d(TAG, "init layout");
        setContentView(R.layout.media_picker_activity);
        MusicUtils.updateButtonBar(this, R.id.songtab);

        // init the refresh button
        scannerBtn = (Button) findViewById(R.id.refresh_btn);
        scannerBtn.setVisibility(View.VISIBLE);
        scannerBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (updateTime == 0) {
                   updateTime = System.currentTimeMillis();
                   // scannerBtn.setEnabled();
                } else {
                    long diffTime = System.currentTimeMillis() - updateTime;
                    if (!(diffTime > UPDATETIME)) {
                        showRefreshDialog(diffTime);
                        return;
                    }
                }
                String sdPath = Environment.getExternalStorageDirectory().getAbsolutePath();
                if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                    sdPath = SCANNERPATH;
                }
                startMediaScanner(getApplicationContext(), sdPath);
            }
        });

        // Init the ListView
        Log.d(TAG, "Creating ListView");
        mTrackList = getListView();
        mTrackList.setCacheColorHint(0);
        mTrackList.setTextFilterEnabled(true);
        mBrowseListAdapter = (TrackBrowseAdapter) getLastNonConfigurationInstance();
        if (mBrowseListAdapter == null) {
            mBrowseListAdapter = new TrackBrowseAdapter(this, R.layout.track_list_item);
        }
        setListAdapter(mBrowseListAdapter);
        // don't set the album art until after the view has been layed out
        mTrackList.post(new Runnable() {
            public void run() {
                mTrackList.setBackgroundColor(Color.WHITE);
                mTrackList.setCacheColorHint(0);
            }
        });

        // Create media browser
        Log.d(TAG, "Creating MediaBrowser");
        mMediaBrowser = new MediaBrowser(this, new ComponentName(this, MediaPlaybackService.class),
                mConnectionCallback, null);
        // update playing delay, otherwise when rotate,title will be show
        if (icicle != null)
            MusicUtils.updateNowPlaying(this);
    }

    // send the scanner broadcast
    private void startMediaScanner(Context context, String filePath) {
        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        intent.setData(Uri.fromFile(new File(filePath)));
        context.sendBroadcast(intent);
    }

    private void showRefreshDialog(long diffTime) {
        String showMessage = String.format(getResources().getString(R.string.refresh_dialog_title));
        /*
        new AlertDialog.Builder(this)
        .setTitle(showMessage)
        .setPositiveButton(getString(R.string.refresh_dialog_y), new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        })
        .create()
        .show();
        */

        if (toastShowTime > diffTime - TOASTUPDATETIME) {
            return;
        }
        toastShowTime = diffTime;
        Toast toast = Toast.makeText(this, showMessage, Toast.LENGTH_SHORT);
        toast.setGravity(Gravity.BOTTOM, 0, 0);
        toast.show();
    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart()");
        super.onStart();
        Log.e(TAG," onStart mMediaBrowser.isConnected():"+mMediaBrowser.isConnected());
        if(!mMediaBrowser.isConnected()){
            mMediaBrowser.connect();
        }
        mRefreshMetadata = true;
        ((MusicApp)getApplication()).addMountCallback(this);
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop()");
        super.onStop();
        ((MusicApp)getApplication()).removeMountCallback(this);
        if (null != getMediaController()) {
            // when MediaBrowser disconnect, we should unregisterCallback
            getMediaController().unregisterCallback(mMediaControllerCallback);
        }
        Log.d(TAG," onStop mRefreshMetadata:"+mRefreshMetadata);
        if(mRefreshMetadata){
            mMediaBrowser.disconnect();
        }
       // mMediaBrowser.disconnect();

    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy()");
        ListView lv = getListView();
        // Because we pass the adapter to the next activity, we need to make
        // sure it doesn't keep a reference to this activity. We can do this
        // by clearing its DatasetObservers, which setListAdapter(null) does.
        setListAdapter(null);
        mBrowseListAdapter = null;
        super.onDestroy();
    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume()");
        if (mBrowseListAdapter != null) {
            mBrowseListAdapter.notifyDataSetChanged();
        }
        super.onResume();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause()");
        super.onPause();
    }

    @Override
    public void onSaveInstanceState(Bundle outcicle) {
        outcicle.putParcelable(MusicUtils.TAG_PARENT_ITEM, mParentItem);
        outcicle.putBoolean(MusicUtils.TAG_WITH_TABS, mWithTabs);
        super.onSaveInstanceState(outcicle);
    }

    private MediaBrowser.SubscriptionCallback mSubscriptionCallback =
            new MediaBrowser.SubscriptionCallback() {

                @Override
                public void onChildrenLoaded(
                        String parentId, List<MediaBrowser.MediaItem> children) {
                    mBrowseListAdapter.clear();
                    mBrowseListAdapter.notifyDataSetInvalidated();
                    for (MediaBrowser.MediaItem item : children) {
                        mBrowseListAdapter.add(item);
                    }
                    mBrowseListAdapter.notifyDataSetChanged();
                }

                @Override
                public void onError(String id) {
                    Toast.makeText(getApplicationContext(), R.string.error_loading_media,
                                 Toast.LENGTH_LONG)
                            .show();
                }
            };

    private MediaBrowser.ConnectionCallback mConnectionCallback =
            new MediaBrowser.ConnectionCallback() {
                @Override
                public void onConnected() {
                    Log.d(TAG, "onConnected: session token " + mMediaBrowser.getSessionToken());
                    mMediaBrowser.subscribe(mParentItem.getMediaId(), mSubscriptionCallback);
                    if (mMediaBrowser.getSessionToken() == null) {
                        throw new IllegalArgumentException("No Session token");
                    }
                    MediaController mediaController = new MediaController(
                            TrackBrowserActivity.this, mMediaBrowser.getSessionToken());
                    mediaController.registerCallback(mMediaControllerCallback);
                    TrackBrowserActivity.this.setMediaController(mediaController);
                    // no metadata should let playing view gone, otherwise enter to MediaPlaybackActivity content will be none.
                    //if (mediaController.getMetadata() != null && mWithTabs) {
                    if (mWithTabs) {
                        MusicUtils.updateNowPlaying(TrackBrowserActivity.this);
                    }
                }

                @Override
                public void onConnectionFailed() {
                    Log.d(TAG, "onConnectionFailed");
                }

                @Override
                public void onConnectionSuspended() {
                    Log.d(TAG, "onConnectionSuspended");
                    TrackBrowserActivity.this.setMediaController(null);
                }
            };

    private MediaController.Callback mMediaControllerCallback = new MediaController.Callback() {
        @Override
        public void onSessionDestroyed() {
            // when mount or unmount changed disconnect make MediaPlaybackService destory, we re-connect it
            if (!mMediaBrowser.isConnected()) {
                LogHelper.d(TAG, "Session destroyed. MediaBrower: re-conncet");
                mMediaBrowser.connect();
            }
        }

        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            super.onMetadataChanged(metadata);
            if (mWithTabs) {
                MusicUtils.updateNowPlaying(TrackBrowserActivity.this);
            }
            if (mBrowseListAdapter != null) {
                mBrowseListAdapter.notifyDataSetChanged();
            }
        }
    };

    // An adapter for showing the list of browsed MediaItem's
    private static class TrackBrowseAdapter extends ArrayAdapter<MediaBrowser.MediaItem> {
        private int mLayoutId;
        private final Drawable mNowPlayingOverlay;
        private Activity mActivity;

        static class ViewHolder {
            TextView line1;
            TextView line2;
            TextView duration;
            ImageView play_indicator;
        }

        TrackBrowseAdapter(Activity activity, int layout) {
            super(activity, layout, new ArrayList<>());
            mLayoutId = layout;
            mNowPlayingOverlay = activity.getResources().getDrawable(
                    R.drawable.indicator_ic_mp_playing_list, activity.getTheme());
            mActivity = activity;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            Log.d(TAG, "getView()");
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(mLayoutId, parent, false);
                ImageView iv = (ImageView) convertView.findViewById(R.id.icon);
                iv.setVisibility(View.GONE);
                ViewHolder vh = new ViewHolder();
                vh.line1 = (TextView) convertView.findViewById(R.id.line1);
                vh.line2 = (TextView) convertView.findViewById(R.id.line2);
                vh.duration = (TextView) convertView.findViewById(R.id.duration);
                vh.play_indicator = (ImageView) convertView.findViewById(R.id.play_indicator);
                convertView.setTag(vh);
            }
            ViewHolder holder = (ViewHolder) convertView.getTag();
            MediaBrowser.MediaItem item = getItem(position);
            Log.d(TAG, "title: " + item.getDescription().getTitle());
            holder.line1.setText(item.getDescription().getTitle());
            Log.d(TAG, "artist: " + item.getDescription().getSubtitle());
            holder.line2.setText(item.getDescription().getSubtitle());
            long duration =
                    item.getDescription().getExtras().getLong(MediaMetadata.METADATA_KEY_DURATION);
            LogHelper.d(TAG, "duration: ", duration);
            holder.duration.setText(MusicUtils.makeTimeString(getContext(), duration / 1000));
            MediaController mediaController = mActivity.getMediaController();
            if (mediaController == null) {
                holder.play_indicator.setImageDrawable(null);
                return convertView;
            }
            MediaMetadata metadata = mediaController.getMetadata();
            if (metadata == null) {
                holder.play_indicator.setImageDrawable(null);
                return convertView;
            }
            if (item.getDescription().getMediaId().endsWith(
                        metadata.getString(MediaMetadata.METADATA_KEY_MEDIA_ID))) {
                holder.play_indicator.setImageDrawable(mNowPlayingOverlay);
            } else {
                holder.play_indicator.setImageDrawable(null);
            }
            return convertView;
        }
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        TrackBrowseAdapter a = mBrowseListAdapter;
        return a;
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        Log.d(TAG, "onListItemClick at position " + position + ", id " + id);
        MediaBrowser.MediaItem item = mBrowseListAdapter.getItem(position);
        if (item.isPlayable()) {
            mRefreshMetadata = false;
            getMediaController().getTransportControls().playFromMediaId(item.getMediaId(), null);
            Intent intent = new Intent(this, MediaPlaybackActivity.class);
            startActivity(intent);
        }
    }

    @Override
    public void onAppMountChanged(boolean isMounted) {
        if (mMediaBrowser.isConnected()) {
            LogHelper.d(TAG, "mount changed: isMounted: " + isMounted + ", MediaBrowser: disconnect");
            int resId = R.string.removeable_mounted;
            if (!isMounted) {
                resId = R.string.removeable_unmounted;
            }
            Toast.makeText(this, resId, Toast.LENGTH_SHORT).show();

            // when mount or unmount we:
            // 1. stop music play
            // 2. disconnect media brower service which will make session detroyed
            //   and the media controller will re-connect it
            /*
            if (null != getMediaController()) {
                getMediaController().getTransportControls().stop();
            }
            mMediaBrowser.disconnect();
            setMediaController(null);
            MusicUtils.updateNowPlaying(TrackBrowserActivity.this);
            */
        }
    }
}
