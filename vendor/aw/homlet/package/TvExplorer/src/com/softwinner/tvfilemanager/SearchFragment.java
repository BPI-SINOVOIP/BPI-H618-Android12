/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.softwinner.tvfilemanager;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.HeaderItem;
import android.support.v17.leanback.widget.ImageCardView;
import android.support.v17.leanback.widget.ListRow;
import android.support.v17.leanback.widget.ListRowPresenter;
import android.support.v17.leanback.widget.ObjectAdapter;
import android.support.v17.leanback.widget.OnItemViewClickedListener;
import android.support.v17.leanback.widget.Presenter;
import android.support.v17.leanback.widget.Row;
import android.support.v17.leanback.widget.RowPresenter;
import android.support.v4.app.ActivityOptionsCompat;
import android.text.TextUtils;
import android.util.Log;

import com.softwinner.tvfilemanager.entity.LocalMusic;
import com.softwinner.tvfilemanager.entity.LocalPicture;
import com.softwinner.tvfilemanager.entity.LocalVideo;
import com.softwinner.tvfilemanager.presenter.CardPresenter;

import java.io.File;
import java.util.List;
import java.util.Locale;

/*
 * This class demonstrates how to do in-app search
 */
public class SearchFragment extends android.support.v17.leanback.app.SearchFragment
        implements android.support.v17.leanback.app.SearchFragment.SearchResultProvider {
    private static final String TAG = "SearchFragment";
    private static final int SEARCH_DELAY_MS = 1000;

    private ArrayObjectAdapter mRowsAdapter;
    private Handler mHandler = new Handler();
    private SearchRunnable mDelayedLoad;
    private String mQuery;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mRowsAdapter = new ArrayObjectAdapter(new ListRowPresenter());
        setSearchResultProvider(this);
        setOnItemViewClickedListener(new ItemViewClickedListener());
        mDelayedLoad = new SearchRunnable();
    }

    @Override
    public ObjectAdapter getResultsAdapter() {
        return mRowsAdapter;
    }

    @Override
    public boolean onQueryTextChange(String newQuery) {
        Log.i(TAG, String.format("Search Query Text Change %s", newQuery));
        loadQuery(newQuery);
        return true;
    }

    @Override
    public boolean onQueryTextSubmit(String query) {
        Log.i(TAG, String.format("Search Query Text Submit %s", query));
        loadQuery(query);
        return true;
    }

    private void loadQuery(String query) {
        mQuery = query;
        mRowsAdapter.clear();
        mHandler.removeCallbacks(mDelayedLoad);
        if (!TextUtils.isEmpty(query) && !query.equals("nil")) {
            mDelayedLoad.setSearchQuery(query);
            mHandler.postDelayed(mDelayedLoad, SEARCH_DELAY_MS);
        }
    }

    private void loadRows(String query) {
        List<LocalVideo> videos = MyApplication.mLocalVideo;
        List<LocalPicture> pictures = MyApplication.mLocalPicture;
        List<LocalMusic> musics = MyApplication.mLocalMusic;
        ArrayObjectAdapter listRowAdapter = new ArrayObjectAdapter(new CardPresenter());

        for (LocalVideo video : videos) {
            if (video.getFile_name().toLowerCase(Locale.ENGLISH)
                    .contains(query.toLowerCase(Locale.ENGLISH))) {
                listRowAdapter.add(video);
            }
        }

        for (LocalPicture picture : pictures) {
            if (picture.getFile_name().toLowerCase(Locale.ENGLISH)
                    .contains(query.toLowerCase(Locale.ENGLISH))) {
                listRowAdapter.add(picture);
            }
        }

        for (LocalMusic music : musics) {
            if (music.getFile_name().toLowerCase(Locale.ENGLISH)
                    .contains(query.toLowerCase(Locale.ENGLISH))) {
                listRowAdapter.add(music);
            }
        }

        HeaderItem header = new HeaderItem(0, getResources().getString(R.string.search_results)
                + " '" + mQuery + "'",
                null);
        mRowsAdapter.add(new ListRow(header, listRowAdapter));
    }

    private class SearchRunnable implements Runnable {

        private volatile String searchQuery;

        public SearchRunnable() {
        }

        public void run() {
            loadRows(searchQuery);
        }

        public void setSearchQuery(String value) {
            this.searchQuery = value;
        }
    }

    private final class ItemViewClickedListener implements OnItemViewClickedListener {
        @Override
        public void onItemClicked(Presenter.ViewHolder itemViewHolder, Object item,
                                  RowPresenter.ViewHolder rowViewHolder, Row row) {

            if (item instanceof LocalVideo) {
                LocalVideo video = (LocalVideo) item;
                Log.d(TAG, "Movie: " + video.toString());
                Intent intent = new Intent(getActivity(), VideoDetailsActivity.class);
                intent.putExtra(VideoDetailsActivity.VIDEO, video);

                Bundle bundle = ActivityOptionsCompat.makeSceneTransitionAnimation(
                        getActivity(),
                        ((ImageCardView) itemViewHolder.view).getMainImageView(),
                        VideoDetailsActivity.SHARED_ELEMENT_NAME).toBundle();
                getActivity().startActivity(intent, bundle);

            } else if (item instanceof LocalPicture) {
                LocalPicture picture = (LocalPicture) item;
                File file = new File(picture.getFile_path());
                Intent picIntent = new Intent();
                picIntent.setAction(android.content.Intent.ACTION_VIEW);
                picIntent.setDataAndType(Uri.fromFile(file), "image/*");
                try {
                    getActivity().startActivity(picIntent);
                } catch (ActivityNotFoundException e) {
                    e.printStackTrace();
                }

            } else if(item instanceof LocalMusic) {
                LocalMusic music = (LocalMusic) item;
                File file = new File(music.getFile_path());
                Intent misIntent = new Intent();
                misIntent.setAction(android.content.Intent.ACTION_VIEW);
                misIntent.setDataAndType(Uri.fromFile(file), "audio/*");
                try {
                    getActivity().startActivity(misIntent);
                } catch (ActivityNotFoundException e) {
                    e.printStackTrace();
                }
            }


        }
    }


}
