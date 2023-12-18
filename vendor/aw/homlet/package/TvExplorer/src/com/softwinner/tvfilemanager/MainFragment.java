package com.softwinner.tvfilemanager;

import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.support.v17.leanback.app.BackgroundManager;
import android.support.v17.leanback.app.BrowseFragment;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.HeaderItem;
import android.support.v17.leanback.widget.ImageCardView;
import android.support.v17.leanback.widget.ListRow;
import android.support.v17.leanback.widget.ListRowPresenter;
import android.support.v17.leanback.widget.OnItemViewClickedListener;
import android.support.v17.leanback.widget.OnItemViewSelectedListener;
import android.support.v17.leanback.widget.Presenter;
import android.support.v17.leanback.widget.Row;
import android.support.v17.leanback.widget.RowPresenter;
import android.support.v4.app.ActivityOptionsCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.softwinner.tvfilemanager.common.Constants;
import com.softwinner.tvfilemanager.common.TypeFilter;
import com.softwinner.tvfilemanager.common.Utils;
import com.softwinner.tvfilemanager.entity.LocalMusic;
import com.softwinner.tvfilemanager.entity.LocalPicture;
import com.softwinner.tvfilemanager.entity.LocalVideo;
import com.softwinner.tvfilemanager.presenter.MusicCardPresenter;
import com.softwinner.tvfilemanager.presenter.PictureCardPresenter;
import com.softwinner.tvfilemanager.presenter.VideoCardPresenter;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by ningxiangzhong on 15-3-11.
 */
public class MainFragment extends BrowseFragment {

    private static final String TAG = "MainFragment";

    private static final int GRID_ITEM_WIDTH = 224;
    private static final int GRID_ITEM_HEIGHT = 224;
    private static final int VIDEO_FILE_HEADER = 0;
    private static final int MUSIC_FILE_HEADER = 1;
    private static final int PICTURE_FILE_HEADER = 2;
    private static final int OTHER_HEADER = 3;

    private int videoFileNum = 0;
    private int pictureFileNum = 0;
    private int musicFileNum = 0;

    private ImageLoadManager imageLoadManager;
    private ArrayObjectAdapter mRowsAdapter;
    private ArrayObjectAdapter mVideoListRowAdapter;
    private ArrayObjectAdapter mPictureListRowAdapter;
    private ArrayObjectAdapter mMusicListRowAdapter;

    private VideoCardPresenter mVideoCardPresenter;
    private PictureCardPresenter mPictureCardPresenter;
    private MusicCardPresenter mMusicCardPresenter;

    private Drawable mDefaultBackground;
    private BackgroundManager backgroundManager;
    private DisplayMetrics mMetrics;
    private Dialog mDialog;

    private Bitmap videoGrid;
    private Bitmap pictureGrid;
    private Bitmap musicGrid;


    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 2:
                    //video更新
                    mVideoListRowAdapter.add(msg.obj);
                    mVideoListRowAdapter.notifyArrayItemRangeChanged(videoFileNum, 1);
                    break;
                case 3:
                    //picture更新
                    mPictureListRowAdapter.add(msg.obj);
                    mPictureListRowAdapter.notifyArrayItemRangeChanged(pictureFileNum, 1);
                    break;
                case 4:
                    //music更新
                    mMusicListRowAdapter.add(msg.obj);
                    mMusicListRowAdapter.notifyArrayItemRangeChanged(musicFileNum, 1);
                    break;
                default:
                    break;
            }

        }
    };

    private TvFileManagerBroadCastReceiver mTvFileManagerBroadCastReceiver;

    public class TvFileManagerBroadCastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {

            String action = intent.getAction();
            Uri uri = intent.getData();
            final String path;

            videoFileNum = 0;
            pictureFileNum = 0;
            musicFileNum = 0;

            Log.d(TAG, ">>>>>>>> onReceiver" + intent.getAction());
            if (uri.getScheme().equals("file")) {
                path = uri.getPath();

                Log.d(TAG, "path : " + path);

                if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {

                    Log.d(TAG, "A device Mounted");

                    if (!MyApplication.pathMounted.contains(path)) {
                        MyApplication.pathMounted.add(path);
                    }
                    if (MyApplication.pathRemoved.contains(path)) {
                        MyApplication.pathRemoved.remove(path);
                    }
                    //重新加载数据
                    new loadDataAsyncTask().execute();

                }

                if (action.equals(Intent.ACTION_MEDIA_REMOVED)) {

                    Log.d(TAG, "A device Removed");

                    MyApplication.pathRemoved.add(path);
                    if (MyApplication.pathMounted.contains(path)) {
                        MyApplication.pathMounted.remove(path);
                    }

//                    imageLoadManager.cancelAllTask();
                    imageLoadManager.clearCache();
                    //更新界面
                    loadRows();
                    //重新加载数据
                    new loadDataAsyncTask().execute();

                }
            }

        }
    }

    //异步任务，加载数据loadData()
    class loadDataAsyncTask extends AsyncTask<Integer, Integer, ArrayList<Object>> {

        @Override
        protected ArrayList doInBackground(Integer... params) {
            Log.e(TAG, "loadData starting....");

            getSortedFilePathList(MyApplication.getPathMounted());
            return null;
        }

    }

    private void registerReceiver() {

        mTvFileManagerBroadCastReceiver = new TvFileManagerBroadCastReceiver();
        IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        intentFilter.addDataScheme("file");
        getActivity().registerReceiver(mTvFileManagerBroadCastReceiver, intentFilter);

    }

    private void unRegisterReceiver() {

        if (mTvFileManagerBroadCastReceiver != null) {
            getActivity().unregisterReceiver(mTvFileManagerBroadCastReceiver);
            mTvFileManagerBroadCastReceiver = null;
        }

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {

        Log.i(TAG, "onCreate");

        super.onActivityCreated(savedInstanceState);

        registerReceiver();

        prepareBackgroundManager();

        setupUIElements();

        setupEventListeners();

        initData();

        loadRows();

        new loadDataAsyncTask().execute();

    }

    private void prepareBackgroundManager() {

        backgroundManager = BackgroundManager.getInstance(getActivity());
        backgroundManager.attach(getActivity().getWindow());

        mDefaultBackground = getResources().getDrawable(R.drawable.default_background);

        mMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(mMetrics);
    }

    private void setupUIElements() {
//        setBadgeDrawable(getActivity().getResources().getDrawable(R.drawable.videos_by_google_banner));

        setTitle(getString(R.string.browse_title)); // Badge, when set, takes precedent
        // over title
        setHeadersState(HEADERS_ENABLED);
        setHeadersTransitionOnBackEnabled(true);

        // set fastLane (or headers) background color
        setBrandColor(getResources().getColor(R.color.fastlane_background));
        // set search icon color
        setSearchAffordanceColor(getResources().getColor(R.color.search_opaque));
    }

    private void setupEventListeners() {
        setOnSearchClickedListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getActivity(), SearchActivity.class);
                startActivity(intent);
            }
        });

        setOnItemViewClickedListener(new ItemViewClickedListener());
       // setOnItemViewSelectedListener(new ItemViewSelectedListener());
    }

    @Override
    public void onDestroy() {

        super.onDestroy();
        unRegisterReceiver();

        //暴力自杀，为了终止子线程，不然下次再进入加载数据会出错，还有其他好的办法？？
        android.os.Process.killProcess(Process.myPid());

    }

    private void initData() {

        imageLoadManager = ImageLoadManager.getInstance(getActivity());

        //Adapter初始化
        mRowsAdapter = new ArrayObjectAdapter(new ListRowPresenter());

        mVideoCardPresenter = new VideoCardPresenter();
        mVideoListRowAdapter = new ArrayObjectAdapter(mVideoCardPresenter);

        mPictureCardPresenter = new PictureCardPresenter();
        mPictureListRowAdapter = new ArrayObjectAdapter(mPictureCardPresenter);

        mMusicCardPresenter = new MusicCardPresenter();
        mMusicListRowAdapter = new ArrayObjectAdapter(mMusicCardPresenter);

        //Grid Bitmap 初始化
        videoGrid = BitmapFactory.decodeResource(getResources(), R.drawable.video_grid);
        pictureGrid = BitmapFactory.decodeResource(getResources(), R.drawable.picture_grid);
        musicGrid = BitmapFactory.decodeResource(getResources(), R.drawable.music_grid);
    }

    private void loadRows() {

        Log.d(TAG, "LoadRowsStart...");
        Long startTime = System.currentTimeMillis();

        mRowsAdapter.clear();
        mVideoListRowAdapter.clear();
        mPictureListRowAdapter.clear();
        mMusicListRowAdapter.clear();

        //LocalVideo
        LocalVideo localVideo = new LocalVideo();
        localVideo.setFile_path("video.mp4");
        localVideo.setFile_name("Video GridView");
        localVideo.setVideo_thumbnail(videoGrid);
        mVideoListRowAdapter.add(localVideo);

        HeaderItem mLocalVideoHeader = new HeaderItem(VIDEO_FILE_HEADER, Constants.HEADER_CATEGORY[0], null);
        mRowsAdapter.add(new ListRow(mLocalVideoHeader, mVideoListRowAdapter));

        //LocalPicture
        LocalPicture localPicture = new LocalPicture();
        localPicture.setPicture_path("picture.jpg");
        localPicture.setPicture_name("Picture GridView");
        localPicture.setPicture_thumbanil(pictureGrid);
        mPictureListRowAdapter.add(localPicture);

        HeaderItem mLocalPictureHeader = new HeaderItem(PICTURE_FILE_HEADER, Constants.HEADER_CATEGORY[2], null);
        mRowsAdapter.add(new ListRow(mLocalPictureHeader, mPictureListRowAdapter));

        //LocalMusic
        LocalMusic localMusic = new LocalMusic();
        localMusic.setMusic_name("Music GridView");
        localMusic.setMusic_thumbnail(musicGrid);
        mMusicListRowAdapter.add(localMusic);

        HeaderItem mLocalMusicHeader = new HeaderItem(MUSIC_FILE_HEADER, Constants.HEADER_CATEGORY[1], null);
        mRowsAdapter.add(new ListRow(mLocalMusicHeader, mMusicListRowAdapter));

        setAdapter(mRowsAdapter);

        Long loadTotalTime = System.currentTimeMillis() - startTime;
        Log.d(TAG, "loadTotalTime = " + loadTotalTime);
        Log.d(TAG, "loadRowsEnd");

    }

    private void loadLocalVideoData(String filePath) {

        String file_path = filePath;
        String file_name = file_path.substring(file_path.lastIndexOf("/") + 1);
        LocalVideo localVideo = Utils.buildLocalVideo(file_path, file_name);
        videoFileNum++;

        synchronized (MyApplication.mLocalVideo) {
            MyApplication.mLocalVideo.add(localVideo);
        }

        Message message = new Message();
        message.obj = localVideo;
        message.what = 2;
        mHandler.sendMessage(message);

    }

    private void loadLocalPictureData(String filePath) {

        String file_path = filePath;
        String file_name = file_path.substring(file_path.lastIndexOf("/") + 1);

        LocalPicture localPicture = Utils.buildLocalPicture(file_path, file_name);
        pictureFileNum++;

        synchronized (MyApplication.mLocalPicture) {
            MyApplication.mLocalPicture.add(localPicture);
        }

        Message message = new Message();
        message.obj = localPicture;
        message.what = 3;
        mHandler.sendMessage(message);

    }

    private void loadLocalMusicData(String filePath) {

        String file_path = filePath;
        String file_name = file_path.substring(file_path.lastIndexOf("/") + 1);
        LocalMusic localMusic = Utils.buildLocalMusic(file_path, file_name);
        musicFileNum++;

        synchronized (MyApplication.mLocalMusic) {
            MyApplication.mLocalMusic.add(localMusic);
        }

        Message message = new Message();
        message.obj = localMusic;
        message.what = 4;
        mHandler.sendMessage(message);
    }

    //获取数据并更新显示到界面。
    private void getSortedFilePathList(List<String> path) {

        MyApplication.mLocalVideo.clear();
        MyApplication.mLocalPicture.clear();
        MyApplication.mLocalMusic.clear();

        //移除掉除了第一个外的所有数据，再重新加载，防止重复加载内部SD卡里的数据。
        mVideoListRowAdapter.removeItems(1, mVideoListRowAdapter.size() - 1);
        mPictureListRowAdapter.removeItems(1, mPictureListRowAdapter.size() - 1);
        mMusicListRowAdapter.removeItems(1, mMusicListRowAdapter.size() - 1);

        int i;
        String filePath = null;

        for (i = 0; i < path.size(); i++) {

            File file = new File(path.get(i));
            File[] fileList = null;

            if (file.canRead()) {
                fileList = file.listFiles();
            }

            if (fileList != null) {

                for (int m = 0; m < fileList.length; m++) {
                    if (fileList[m].isDirectory()) {

//                        getDirFileList(fileList[m].getAbsolutePath());

                        final String directory = fileList[m].getAbsolutePath();
                        new Thread() {
                            @Override
                            public void run() {

                                getDirFileList(directory);

                            }
                        }.start();

                    } else {
                        filePath = fileList[m].getAbsolutePath();
                        if (TypeFilter.isMovieFile(filePath)) {
                            loadLocalVideoData(filePath);
                        } else if (TypeFilter.isPictureFile(filePath)) {
                            loadLocalPictureData(filePath);
                        } else if (TypeFilter.isMusicFile(filePath)) {
                            loadLocalMusicData(filePath);
                        }
//                    Log.e("ningxzMainFragment", fileList[m].getName());
                    }
                }
            }
        }
    }

    //辅助getSortedFilePathList函数的函数
    public void getDirFileList(String path) {

        File file = new File(path);
        File[] fileList = null;
        String filePath = null;
        if (file.canRead()) {
            fileList = file.listFiles();
        }
        if (fileList != null) {
            for (int m = 0; m < fileList.length; m++) {
                if (fileList[m].isDirectory()) {

                    getDirFileList(fileList[m].getAbsolutePath());

                } else {
                    filePath = fileList[m].getAbsolutePath();
                    if (TypeFilter.isMovieFile(filePath)) {
                        loadLocalVideoData(filePath);
                    } else if (TypeFilter.isPictureFile(filePath)) {
                        loadLocalPictureData(filePath);
                    } else if (TypeFilter.isMusicFile(filePath)) {
                        loadLocalMusicData(filePath);
                    }
//                    Log.e("ningxzMainFragment", fileList[m].getName());
                }
            }
        }
    }

    private final class ItemViewClickedListener implements OnItemViewClickedListener {
        @Override
        public void onItemClicked(Presenter.ViewHolder itemViewHolder, Object item,
                                  RowPresenter.ViewHolder rowViewHolder, Row row) {

            if (item instanceof LocalVideo) {
                LocalVideo video = (LocalVideo) item;
                Log.d(TAG, "Item: " + item.toString());

                if (!video.getFile_name().equals("Video GridView")) {
                    Intent intent = new Intent(getActivity(), VideoDetailsActivity.class);
                    intent.putExtra(VideoDetailsActivity.VIDEO, video);

                    Bundle bundle = ActivityOptionsCompat.makeSceneTransitionAnimation(getActivity(),
                            ((ImageCardView) itemViewHolder.view).getMainImageView(),
                            VideoDetailsActivity.SHARED_ELEMENT_NAME).toBundle();
                    getActivity().startActivity(intent, bundle);
                } else  {
                    Intent intent = new Intent(getActivity(), VideoGridActivity.class);
                    startActivity(intent);
                }

            } else if (item instanceof String) {
                if (((String) item).indexOf(getString(R.string.music_grid)) >= 0) {
                    Intent intent = new Intent(getActivity(), MusicGridActivity.class);
                    startActivity(intent);
                } else if (((String) item).indexOf(getString(R.string.video_grid)) >= 0) {
                    Intent intent = new Intent(getActivity(), VideoGridActivity.class);
                    startActivity(intent);
                } else if (((String) item).indexOf(getString(R.string.picture_grid)) >= 0) {
                    Intent intent = new Intent(getActivity(), PictureGridActivity.class);
                    startActivity(intent);
                }
            } else if (item instanceof LocalPicture) {
                LocalPicture picture = (LocalPicture) item;
                if (picture.getFile_name().equals("Picture GridView")) {
                    Intent intent = new Intent(getActivity(), PictureGridActivity.class);
                    startActivity(intent);

                } else  {
                    showPicture(picture.getFile_path());
                }
            } else if (item instanceof LocalMusic) {
                LocalMusic music = (LocalMusic) item;
                if (music.getFile_name().equals("Music GridView")) {
                    Intent intent = new Intent(getActivity(), MusicGridActivity.class);
                    startActivity(intent);

                } else {
                    listenMusic(music.getFile_path());
                }
            }
        }
    }

    private final class ItemViewSelectedListener implements OnItemViewSelectedListener {
        @Override
        public void onItemSelected(Presenter.ViewHolder itemViewHolder, Object item,
                                   RowPresenter.ViewHolder rowViewHolder, Row row) {
            if (item instanceof LocalVideo) {
                backgroundManager.setDrawable(getResources().getDrawable(R.drawable.video_grid));
            } else if (item instanceof LocalPicture) {
                backgroundManager.setDrawable(getResources().getDrawable(R.drawable.picture_grid));
            } else if(item instanceof LocalMusic) {
                backgroundManager.setDrawable(getResources().getDrawable(R.drawable.music_grid));
            }
        }
    }

    //显示图片
    private void showPicture(String path) {

        File file = new File(path);
        Intent picIntent = new Intent();
        picIntent.setAction(android.content.Intent.ACTION_VIEW);
        picIntent.setDataAndType(Uri.fromFile(file), "image/*");
        try {
            getActivity().startActivity(picIntent);
        } catch (ActivityNotFoundException e) {
            e.printStackTrace();
        }

    }

    //听音乐
    private void listenMusic(String path) {

        File file = new File(path);
        Intent misIntent = new Intent();
        misIntent.setAction(android.content.Intent.ACTION_VIEW);
        misIntent.setDataAndType(Uri.fromFile(file), "audio/*");
        try {
            getActivity().startActivity(misIntent);
        } catch (ActivityNotFoundException e) {
            e.printStackTrace();
        }

    }

    private class GridItemPresenter extends Presenter {
        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent) {
            TextView view = new TextView(parent.getContext());
            view.setLayoutParams(new ViewGroup.LayoutParams(GRID_ITEM_WIDTH, GRID_ITEM_HEIGHT));
            view.setFocusable(true);
            view.setFocusableInTouchMode(true);
            view.setBackgroundColor(getResources().getColor(R.color.detail_background));
            view.setTextColor(Color.WHITE);
            view.setGravity(Gravity.CENTER);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(ViewHolder viewHolder, Object item) {
            ((TextView) viewHolder.view).setText((String) item);
        }

        @Override
        public void onUnbindViewHolder(ViewHolder viewHolder) {
        }
    }

}
