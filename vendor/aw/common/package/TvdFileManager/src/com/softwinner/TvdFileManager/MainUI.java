package com.softwinner.TvdFileManager;

import android.system.Os;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.DialogInterface.OnShowListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnVideoSizeChangedListener;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.AsyncTask;
import android.os.storage.StorageVolume;
import static android.os.storage.StorageVolume.EXTRA_STORAGE_VOLUME;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.Manifest;
import android.content.pm.PackageManager;
import android.media.MediaPlayer.OnErrorListener;
import java.io.File;
import java.util.Timer;
import java.util.TimerTask;
import android.os.storage.StorageVolume;
import android.os.storage.StorageManager;
import androidx.core.content.ContextCompat;
import androidx.core.app.ActivityCompat;
import android.text.TextUtils;
import com.softwinner.view.MTextView;
import com.softwinner.view.MenuItem;
import android.provider.MediaStore;
import android.database.Cursor;

public class MainUI extends ListActivity implements SurfaceHolder.Callback, OnPreparedListener,
        OnBufferingUpdateListener, OnCompletionListener, OnErrorListener, OnVideoSizeChangedListener,
        IEventHandlerCallbacks {

    private static final String TAG = "MainUI";
    private EventHandler mHandler;
    private MediaProvider mMedia;
    private Context mContext;
    private DeviceManager mDevices;
    private EventHandler.TableRow mTable;
    private EventHandler.MenuItemListener mMenuListener;

    private LinearLayout mThumb;
    private SurfaceView mVideo;
    private ImageView mImage;
    private MTextView mPreview;
    private TextView mPath;
    private TextView mIndex;
    private ListView list;
    private PowerManager pm;

    // option 栏相关
    private Dialog menuDialog;
    private GridView menuGrid;
    private View menuView;

    private BroadcastReceiver mReceiver;

    /* 用于左右切换时延时刷新 */
    private Timer time = null;
    private TimerTask task;
    private boolean mError = false;

    private boolean returnFile = false;
    private boolean returnContentUri = false;
    private Object syncObject = new Object();

    MenuItem sort;
    MenuItem copy;
    MenuItem paste;
    MenuItem cut;
    MenuItem delete;
    MenuItem rename;
    MenuItem mkdir;
    MenuItem addShare;
    MenuItem rmShare;
    MenuItem umount;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        mContext = this;

        Os.umask(0);

        /* 清除共享区内的信息 */
        SharedPreferences pf = getSharedPreferences("partition", 0);
        SharedPreferences.Editor editor = pf.edit();
        editor.clear();


        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{
                            Manifest.permission.WRITE_EXTERNAL_STORAGE,
                            Manifest.permission.READ_EXTERNAL_STORAGE},
                    0);
        }
        pm = (PowerManager) getSystemService(Context.POWER_SERVICE);

        mMedia = new MediaProvider(this);
        mDevices = new DeviceManager(this);
        mHandler = new EventHandler(this, this, mMedia, mDevices);

        mTable = mHandler.new TableRow();
        mMenuListener = mHandler.new MenuItemListener();

        /* 设置信息预览栏内容 */
        mThumb = (LinearLayout) findViewById(R.id.thumb);
        mVideo = (SurfaceView) findViewById(R.id.thumb_movie);
        mImage = (ImageView) findViewById(R.id.thumbnail);

        Drawable bp = getResources().getDrawable(R.drawable.thumbnail_equipment);
        Bitmap m = ((BitmapDrawable) bp).getBitmap();

        mPreview = (MTextView) findViewById(R.id.preview);
        mPath = (TextView) findViewById(R.id.path);
        mIndex = (TextView) findViewById(R.id.index);
        mHandler.setViewResource(mVideo, mImage, mPreview, mPath, mIndex);

        /* 把ListView 与 TableRow 绑定,并监听items的被选中事件 */
        list = getListView();
        mHandler.setListAdapter(mTable, list);
        list.setAdapter(mTable);
        list.setOnItemSelectedListener(mHandler);
        list.setOnItemClickListener(mHandler);
        list.setOnItemLongClickListener(mHandler);

        /* 设置监听导航栏的菜单item */
        Button devices = (Button) findViewById(R.id.device_button);
        Button video = (Button) findViewById(R.id.video_button);
        Button picture = (Button) findViewById(R.id.picture_button);
        Button music = (Button) findViewById(R.id.music_button);
        Button file = (Button) findViewById(R.id.file_button);
        devices.setOnClickListener(mHandler);
        video.setOnClickListener(mHandler);
        picture.setOnClickListener(mHandler);
        music.setOnClickListener(mHandler);
        file.setOnClickListener(mHandler);

        /* 读取传进来的参数决定过滤类型 */
        Bundle b = getIntent().getExtras();
        if (b != null) {
            String action = b.getString("media_type");
            if (action != null) {
                if (action.equals("MEDIA_TYPE_MUSIC")) {
                    mHandler.setFilterType(mMedia.musicFilter);
                    music.performClick();
                } else if (action.equals("MEDIA_TYPE_PICTURE")) {
                    mHandler.setFilterType(mMedia.pictureFilter);
                    picture.performClick();
                } else if (action.equals("MEDIA_TYPE_VIDEO")) {
                    mHandler.setFilterType(mMedia.movieFilter);
                    video.performClick();
                } else if (action.equals("MEDIA_TYPE_ALL")) {
                    mHandler.setFilterType(mMedia.alltypeFilter);
                    file.performClick();
                } else {
                    devices.performClick();
                }
            } else {
                Log.d(TAG, "started by other parames");
                devices.performClick();
            }
        } else {
            devices.performClick();
        }

        /* 设置option菜单栏信息 */
        createMenu();

        /* 增加热插拔功能 */
        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                StorageVolume storage = (StorageVolume)intent.getParcelableExtra(
                        StorageVolume.EXTRA_STORAGE_VOLUME);
                String volumeId =storage.getId();
                Log.d(TAG, "devices ID:" + volumeId);
                if (intent.getAction().equals(intent.ACTION_MEDIA_REMOVED)
                        || intent.getAction().equals(intent.ACTION_MEDIA_BAD_REMOVAL)
                        || intent.getAction().equals(intent.ACTION_MEDIA_UNMOUNTED)) {
                    /* 等待1秒钟再刷新界面,这是因为底层发出卸载信号时，有时实际上还没卸载掉设备 */
                    mHandler.isoClear();
                    if (mDevices.isInterStoragePath(volumeId)) {
                        // displayToast(getResources().getString(R.string.flash_out));
                        int k = 0;
                    } else if (mDevices.isSdStoragePath(volumeId)) {
                        //displayToast(getResources().getString(R.string.sdcard_out));
                    } else if (mDevices.isUsbStoragePath(volumeId)) {
                        //displayToast(getResources().getString(R.string.usb_out));
                    }

                    if (mHandler.currentDir.startsWith(volumeId) || mHandler.isDeviceList()) {
                        showDeviceList();
                    }
                } else if (intent.getAction().equals(intent.ACTION_MEDIA_MOUNTED)) {
                    if (mHandler.isDeviceList()) {
                        showDeviceList();
                    }
                    if (mDevices.isInterStoragePath(volumeId)) {
                        // displayToast(getResources().getString(R.string.flash_in));
                        int k = 0;
                    } else if (mDevices.isSdStoragePath(volumeId)) {
                        //displayToast(getResources().getString(R.string.sdcard_in));
                    } else if (mDevices.isUsbStoragePath(volumeId)) {
                        //displayToast(getResources().getString(R.string.usb_in));
                    }
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_REMOVED);
        filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
        filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        filter.addDataScheme("file");
        registerReceiver(mReceiver, filter);

        String action = getIntent().getAction();
        if (!TextUtils.isEmpty(action)) {
            if (action.equals("com.softwinner.action.GET_FILE")) {
                returnFile = true;
            } else if(action.equals(Intent.ACTION_GET_CONTENT)) {
                returnContentUri = true;
            }
        }
    }

    private void showDeviceList() {
        mHandler.getDeviceList();
        if (mHandler.mDataSource.size() > 0) {
            mHandler.getDetailForPosition(mHandler.currentPosition);
        }
    }

    private void displayToast(String str) {
        Toast.makeText(getBaseContext(), str, Toast.LENGTH_SHORT).show();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // menu.add("menu");
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public void openOptionsMenu() {
        if (menuDialog == null) {
            setItemStatus();

            DisplayMetrics metrics = new DisplayMetrics();
            this.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            int width = metrics.widthPixels;
            int height = metrics.heightPixels;

            menuDialog = new Dialog(MainUI.this, R.style.menu_dialog);
            // Window win = menuDialog.getWindow();
            // LayoutParams params = new LayoutParams();
            // params.y = 99999;
            // params.x = 0;
            // params.width = width;
            // params.height = height;
            // win.setAttributes(params);
            menuDialog.setOnKeyListener(new OnKeyListener() {
                public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                    if (keyCode == KeyEvent.KEYCODE_MENU) {
                        dialog.dismiss();
                    }
                    return false;
                }
            });
            //menuDialog dismiss方法多处被调用，而menuDialog只被创建一次，造成高概率出现menuDialog显示后没有焦点，遥控器无法使用
            //因此需在onShow中强行设置该窗口为焦点窗口
            menuDialog.setOnShowListener(new OnShowListener() {
                public void onShow(DialogInterface dialog){
                    Window dialogWindow = menuDialog.getWindow();
                    dialogWindow.setLocalFocus(true, true);
                }
            });
            mMenuListener.setListenedMenu(menuDialog);
            menuDialog.setCanceledOnTouchOutside(true);
            menuDialog.setContentView(menuView);

            Window dialogWindow = menuDialog.getWindow();
            WindowManager.LayoutParams lp = dialogWindow.getAttributes();
            dialogWindow.setGravity(Gravity.BOTTOM);
            lp.width = width;
            dialogWindow.setAttributes(lp);

            setItemStatus();
            menuDialog.show();
        } else {
            setItemStatus();
            menuDialog.show();
        }
        // return false;
    }

    private void setItemStatus() {
        /**
         * current directory is "root"(list local disk/neighborhood/nfs
         * share/net device)
         */
        if (mHandler.isDeviceList()) {
            sort.setVisibility(View.GONE);
            copy.setVisibility(View.GONE);
            paste.setVisibility(View.GONE);
            cut.setVisibility(View.GONE);
            delete.setVisibility(View.GONE);
            rename.setVisibility(View.GONE);
            mkdir.setVisibility(View.GONE);
            addShare.setVisibility(View.VISIBLE);
            rmShare.setVisibility(View.VISIBLE);
            umount.setVisibility(View.VISIBLE);
            umount.setEnabled(mDevices.isDeviceMounted(mHandler.getDeviceSelectedPath()));
            if (mHandler.isNetDevSelected()) {
                rmShare.setEnabled(true);
            } else {
                rmShare.setEnabled(false);
            }
            addShare.setFocusable(true);
            return;
        } else {
            sort.setVisibility(View.VISIBLE);
            copy.setVisibility(View.VISIBLE);
            paste.setVisibility(View.VISIBLE);
            cut.setVisibility(View.VISIBLE);
            delete.setVisibility(View.VISIBLE);
            rename.setVisibility(View.VISIBLE);
            mkdir.setVisibility(View.VISIBLE);
            addShare.setVisibility(View.GONE);
            rmShare.setVisibility(View.GONE);
            umount.setVisibility(View.GONE);
        }
        if (mHandler.isPartitionList() || mHandler.isInSambaMode()) {
            sort.setEnabled(true);
            copy.setEnabled(false);
            paste.setEnabled(false);
            cut.setEnabled(false);
            delete.setEnabled(false);
            rename.setEnabled(false);
            mkdir.setEnabled(false);

            sort.setFocusable(true);
            copy.setFocusable(false);
            paste.setFocusable(false);
            cut.setFocusable(false);
            delete.setFocusable(false);
            rename.setFocusable(false);
            mkdir.setFocusable(false);
            return;
        } else if (mHandler.isReturnItemSelected()) {
            sort.setEnabled(true);
            copy.setEnabled(false);
            cut.setEnabled(false);
            delete.setEnabled(false);
            rename.setEnabled(false);

            sort.setFocusable(true);
            copy.setFocusable(false);
            cut.setFocusable(false);
            delete.setFocusable(false);
            rename.setFocusable(false);
        } else {
            sort.setEnabled(true);
            copy.setEnabled(true);
            cut.setEnabled(true);
            delete.setEnabled(true);
            rename.setEnabled(true);

            sort.setFocusable(true);
            copy.setFocusable(true);
            cut.setFocusable(true);
            delete.setFocusable(true);
            rename.setFocusable(true);
        }

        /**
         * Have executed copy operation, can do paste
         */
        if (mHandler.hasFileOperate()) {
            paste.setEnabled(true);
            paste.setFocusable(true);
        } else {
            paste.setEnabled(false);
            paste.setFocusable(false);
        }
        mkdir.setEnabled(true);
        mkdir.setFocusable(true);

        /**
         * In NFS Share folder, list all NFS Server, not allow operate to the
         * server
         */
        if (mHandler.isNFSShare() || mHandler.isNFSServer()) {
            sort.setEnabled(false);
            copy.setEnabled(false);
            paste.setEnabled(false);
            cut.setEnabled(false);
            delete.setEnabled(false);
            rename.setEnabled(false);
            mkdir.setEnabled(false);

            sort.setFocusable(false);
            copy.setFocusable(false);
            paste.setFocusable(false);
            cut.setFocusable(false);
            delete.setFocusable(false);
            rename.setFocusable(false);
            mkdir.setFocusable(false);
        }
    }

    private void createMenu() {
        menuView = View.inflate(this, R.layout.gridview_menu, null);
        sort = (MenuItem) menuView.findViewById(R.id.sort_button);
        copy = (MenuItem) menuView.findViewById(R.id.copy_button);
        paste = (MenuItem) menuView.findViewById(R.id.paste_button);
        cut = (MenuItem) menuView.findViewById(R.id.cut_button);
        delete = (MenuItem) menuView.findViewById(R.id.delete_button);
        rename = (MenuItem) menuView.findViewById(R.id.rename_button);
        mkdir = (MenuItem) menuView.findViewById(R.id.mkdir_button);
        addShare = (MenuItem) menuView.findViewById(R.id.add_new_dev_button);
        rmShare = (MenuItem) menuView.findViewById(R.id.del_dev_button);
        umount = (MenuItem) menuView.findViewById(R.id.umount_button);
        sort.setOnClickListener(mMenuListener);
        copy.setOnClickListener(mMenuListener);
        paste.setOnClickListener(mMenuListener);
        cut.setOnClickListener(mMenuListener);
        delete.setOnClickListener(mMenuListener);
        rename.setOnClickListener(mMenuListener);
        mkdir.setOnClickListener(mMenuListener);
        addShare.setOnClickListener(mMenuListener);
        rmShare.setOnClickListener(mMenuListener);
        umount.setOnClickListener(mMenuListener);
    }

    public void playVideo(String path) {
        runOnUiThread(new Runnable(){
            public void run(){
                if (player != null) {
                    player.release();
                    player = null;
                }
                player = new MediaPlayer();
                pathname = path;
                if (holder == null) {
                    holder = mVideo.getHolder();
                    holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
                    holder.addCallback((SurfaceHolder.Callback)mContext);
                }

                mImage.setVisibility(View.GONE);
                mVideo.setVisibility(View.VISIBLE);

            }
        });
    }

    private MediaPlayer player;
    private String pathname;
    private SurfaceHolder holder;
    private static final String VTAG = "VideoPlayer";

    private void play() {
        OnPreparedListener Pl = this;
        OnCompletionListener Cl = this;
        OnVideoSizeChangedListener Vcl = this;
        OnErrorListener E1 = this;
        //use AsyncTask to operate MediaPlayer, otherwise it maybe cause ANR
        //change by chongyuzhao at 2019.01.19
        Thread thread = new Thread(){
            @Override
            public void run(){

                try {
                    player.setOnErrorListener(E1);
                    player.setDataSource(pathname);

                    player.setDisplay(holder);
                    /*
                    * int w = mVideo.getWidth(); int h = mVideo.getHeight()>(w * 3 /
                    * 4)?mVideo.getHeight():(w * 3 / 4); if (w != 0 && h != 0) {
                    * player.enableScaleMode(true, w, h); Log.d(TAG, "set scale mode:"
                    * + String.valueOf(mVideo.getWidth()) + "*" +
                    * String.valueOf(mVideo.getHeight())); }
                    */
                    player.prepareAsync();
                    player.setOnPreparedListener(Pl);
                    player.setOnCompletionListener(Cl);
                    // add by liuanlong 14/9/30
                    player.setOnVideoSizeChangedListener(Vcl);
                    // end
                    player.setAudioStreamType(AudioManager.STREAM_MUSIC);
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        };
        thread.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder1, int format, int width, int height) {
        Log.d(VTAG, "surfaceview is changed");
        Log.d(VTAG, "to:" + String.valueOf(width) + "*" + String.valueOf(height));
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder1) {
        Log.d(VTAG, "surfaceview is created");
        if (player != null) {
            // 屏幕关闭状态下不播放ThumbVideo
            if (pm.isScreenOn()) {
                play();
            }
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder1) {
        Log.d(VTAG, "surfaceview is destroyed");
        releaseMediaPlayerAsync();
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        Log.d(VTAG, "OnError - Error code: " + what + " Extra code: " + extra);
        mError = true;
        return false;
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        Log.d(VTAG, "media is prepared");
        //run in UI Thread,otherwise Video preview will not play
        try {
            if (player != null && !mError) {
                setVideoRatio(pathname);
                /**
                * if (player.setSubGate(false) != 0) { Log.d(TAG,
                * "fail to close subtitles"); } else { Log.d(TAG,
                * "success close subtitles"); }
                */
                startVideoPlayback();
                showVideoMassage(pathname);
            }
        } catch (Exception e) {
        }
        mError = false;
    }

    private void startVideoPlayback() {
        Log.v(TAG, "startVideoPlayback");
        player.start();
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        // TODO Auto-generated method stub
        Log.v(VTAG, "onVideoSizeChanged called");
        // add by liuanlong 14/9/30
        //showVideoMassage(pathname);
        // end
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        // TODO Auto-generated method stub
        Log.d(VTAG, "onCompletion called");
    }

    @Override
    public void onBufferingUpdate(MediaPlayer mp, int percent) {
        // TODO Auto-generated method stub
        Log.d(VTAG, "onBufferingUpdate percent:" + percent);
    }

    /*
     * 实现回调接口
     */

    @Override
    public boolean returnFile(File file) {
        if (!returnFile) {
            return false;
        }
        Intent ret = new Intent();
        if (file == null) {
            setResult(RESULT_CANCELED, ret);
            Log.d(TAG, "return cancel");
        } else {
            ret.setData(Uri.fromFile(file));
            setResult(RESULT_OK, ret);
            Log.d(TAG, "return file " + file.getAbsolutePath());
        }
        finish();
        return true;
    }

    @Override
    public boolean returnContentUri(String filePath) {
        if (!returnContentUri) {
            return false;
        }
        if (TextUtils.isEmpty(filePath)) {
            return false;
        }
        Intent intent = new Intent();
        Uri retUri = getContentUri(filePath);
        Log.d(TAG,"retUri :" + retUri.toString());
        intent.setData(retUri);
        setResult(RESULT_OK, intent);
        finish();
        return true;
    }

    private Uri getContentUri(String filePath) {
        Log.d(TAG,"aw_tag filePath:"+filePath);
        Cursor cursor = getContentResolver().query(MediaStore.Files.getContentUri("external"),
                null, MediaStore.Files.FileColumns.DATA + " =? ",
                new String[] { filePath }, null);
        if (cursor != null && cursor.moveToFirst()) {
            int id = cursor.getInt(cursor.getColumnIndex(MediaStore.MediaColumns._ID));
            Uri baseUri = Uri.parse("content://media/external/file/");
            return Uri.withAppendedPath(baseUri, "" + id);
        }
        return null;
    }

    @Override
    public void playThumbVideo(String path) {
        playVideo(path);

    }

    @Override
    public void releaseMediaPlayerAsync() {
        Thread thread = new Thread() {
            @Override
            public void run() {
                MediaPlayer tmp = player;
                synchronized (syncObject) {
                    player = null;
                }
                if (tmp != null) {
                    tmp.reset();
                    tmp.release();
                }
            }
        };
        thread.start();
        mHandler.removeVideoMessage();
    }

    @Override
    public void releaseMediaPlayerSync() {
        releaseMediaPlayerAsync();
    }

    @Override
    public boolean hasReleaseMedia() {
        return (player == null);
    }

    @Override
    public void stopPlaying() {
        if (player != null) {
            player.stop();
        }
    }

    @Override
    public void showVideoMassage(String path) {

        if (player == null) {
            return;
        }
        MediaPlayer mediaData = player;
        // 获取视频时间
        long ms = mediaData.getDuration();
        String duration1 = getResources().getString(R.string.duration)
                + mHandler.toDuration((long) ms) + "\n";

        // 获取视频分辨率
        String wVideo = String.valueOf(mediaData.getVideoWidth());
        String hVideo = String.valueOf(mediaData.getVideoHeight());
        String resolution1 = getResources().getString(R.string.resolution) + wVideo + "*" + hVideo;

        // 获取视频大小
        String size1 = getResources().getString(R.string.size)
                + mHandler.toSize(new File(path).length()) + "\n";

        String display1 = size1 + duration1 + resolution1;
        // modified by liuanlong 14/9/30
        if (mediaData.getVideoWidth() != 0 && mediaData.getVideoHeight() != 0) {
            runOnUiThread(new Runnable(){
                public void run(){
                    mPreview.setText2(display1);
                }
            });
        }
        // end
    }

    /*
     * 释放资源
     */
    @Override
    protected void onPause() {
        overridePendingTransition(0,0);
        super.onPause();
        Log.d(TAG, "activity pause");
        releaseMediaPlayerAsync();
    }

    @Override
    protected void onResume() {
        super.onResume();
        int pos = list.getSelectedItemPosition();
        mHandler.setItemSelected(pos);
        mHandler.isoClear();
    }

    @Override
    protected void onDestroy() {

        try {
            Log.d("nfs", "clear");
            mHandler.clear();
            if (mReceiver != null) {
                unregisterReceiver(mReceiver);
            }
        } catch (Exception e) {
            Log.e(TAG, "unknow exception");
            e.printStackTrace();
        }
        releaseMediaPlayerAsync();
        super.onDestroy();
        Log.d(TAG, "activity destroy");
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
                moveToLeft();
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                moveToRight();
                break;
            case KeyEvent.KEYCODE_BACK:
                if (!mHandler.backToPreDir()) {
                    exit();
                }
                break;
            default:
                return super.onKeyDown(keyCode, event);
        }
        return true;
    }

    private void exit() {
        if (!returnFile(null) || !returnContentUri(null)) {
            finish();
        }
    }

    private void moveToLeft() {
        int buttonId = 0;
        switch (mHandler.currentNavigation) {
            case R.id.device_button:
                mHandler.loadBrowseDir();
                buttonId = R.id.file_button;
                break;
            case R.id.video_button:
                mHandler.saveBrowseDir();
                buttonId = R.id.device_button;
                break;
            case R.id.picture_button:
                buttonId = R.id.video_button;
                break;
            case R.id.music_button:
                buttonId = R.id.picture_button;
                break;
            case R.id.file_button:
                buttonId = R.id.music_button;
                break;
            default:
                break;
        }
        Button bt = (Button) findViewById(buttonId);
        mHandler.currentNavigation = buttonId;
        delayToRefresh();

        mHandler.setButtonSelected(bt);

    }

    private void moveToRight() {
        int buttonId = 0;
        switch (mHandler.currentNavigation) {
            case R.id.device_button:
                mHandler.loadBrowseDir();
                buttonId = R.id.video_button;
                break;
            case R.id.video_button:
                buttonId = R.id.picture_button;
                break;
            case R.id.picture_button:
                buttonId = R.id.music_button;
                break;
            case R.id.music_button:
                buttonId = R.id.file_button;
                break;
            case R.id.file_button:
                mHandler.saveBrowseDir();
                buttonId = R.id.device_button;
                break;
            default:
                break;
        }
        Button bt = (Button) findViewById(buttonId);
        mHandler.currentNavigation = buttonId;

        delayToRefresh();

        mHandler.setButtonSelected(bt);

    }

    private void delayToRefresh() {
        if (time != null) {
            time.cancel();
        }
        time = new Timer();
        task = new TimerTask() {
            @Override
            public void run() {
                final Button mb = (Button) findViewById(mHandler.currentNavigation);
                mb.post(new Runnable() {

                    @Override
                    public void run() {
                        mb.performClick();
                    }
                });
            }
        };
        time.schedule(task, 500);
    }

    /* 以原本视频分辨率的比例来调整预览播放窗口大小 */
    private void setVideoRatio(String path) {
        int vh = 0;
        int vw = 0;
        if (player != null) {
            vh = player.getVideoHeight();
            vw = player.getVideoWidth();
        }
        if (vw == 0 || vh == 0) {
            return;
        }
        int width = mThumb.getWidth();
        int height = (vh * width) / vw;
        if (height > mThumb.getHeight()) {
            height = mThumb.getHeight();
        }
        ViewGroup.LayoutParams params = mVideo.getLayoutParams();
        params.width = width;
        params.height = height;
        mVideo.setLayoutParams(params);
    }

}
