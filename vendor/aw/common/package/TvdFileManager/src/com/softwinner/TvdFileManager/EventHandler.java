package com.softwinner.TvdFileManager;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.util.Log;
import android.os.Build;
import androidx.core.content.FileProvider;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import java.util.Iterator;
import android.text.TextUtils;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.os.SystemClock;
import android.os.storage.StorageVolume;
import android.provider.MediaStore;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;
import android.text.format.Formatter;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileFilter;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.SecurityException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.EmptyStackException;
import java.util.Stack;
import java.util.Timer;
import java.util.TimerTask;
import java.net.Inet4Address;

import com.softwinner.TvdFileManager.net.NfsManagerWrapper;
import com.softwinner.TvdFileManager.net.OnSearchListener;
import com.softwinner.TvdFileManager.net.SambaManager;
import com.softwinner.tmp.nfs.NFSFolder;
import com.softwinner.tmp.nfs.NFSServer;
import com.softwinner.view.MTextView;

import jcifs.smb.SmbFile;
import jcifs.util.LocalNetwork;
import jcifs.util.SmbFileOperate;

import android.os.Bundle;
import com.softwinner.GifPlayer;

import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public class EventHandler implements OnClickListener, OnItemSelectedListener, OnItemClickListener,
        OnItemLongClickListener {
    private static final boolean V_BUG = true;
    private static final String TAG = "EventHandler";

    private boolean isListAdjusted = false;
    private boolean directNfsConnect = false;
    private int itemWidth = 0;
    private int itemHeight = 0;

    public int currentNavigation;

    private MediaProvider mMedia;
    private DeviceManager mDevices;
    private NfsManagerWrapper mNfs;
    private SambaManager mSamba;
    private String picPath;

    private Context mContext;
    private TableRow mTable;
    private ListView list;

    public String rootPath = "/";
    public String currentDir = rootPath;
    public int currentPosition = 0;
    public ArrayList<String> mDataSource;
    private String browseDir = null;
    private String filePath;

    private SurfaceView videoThumb;
    private ImageView imageThumb;
    private MTextView preview;
    private TextView path;
    private TextView index;
    private IEventHandlerCallbacks mCallbacks;

    /* 文件目录扫描进度 */
    private ProgressDialog scanDialog;
    private int num = 0;
    private static final int SCANFILES = 0x03;
    private static final String GO_INTO_DIR = "goIntoDir";
    private static final String BACK_TO_PRE = "backToPreDir";
    private static final String NONE = "NONE";
    //when goIntoDir, we press return key so fast, it will cause mDataSource is not current. so we dont
    //backToPreDir when scaningFile, and do it after scaned.
    private boolean scaningFile = false;
    private boolean scaningBack = false;

    /* 定义过滤类型 */
    private Comparator sort = MediaProvider.ALPH;

    /* 用于切换不同按键焦点 */
    private View pre = null;
    private View now = null;

    /* 用于判断是否该播放视频（用户上下选择太快时不必播放） */
    private String videoPath = null;
    private Timer mTimer = null;
    private TimerTask mTask = null;
    private Handler mHandler = null;
    private static final int VIDEO_DELAY = 1500;
    /* 设置缩略图延缓显示的时间 */
    private static final int PIC_DELAY = 500;

    /* 默认过滤类型：所有文件 */
    private FileFilter mFilter;

    /* 每次进入一个文件夹时，保存该文件夹的位置，方便返回时焦点能落在该文件夹上 */
    private Stack<Integer> mPathStack;

    private SharedPreferences pf;
    private SharedPreferences.Editor editor;

    private static final int SEARCH_TYPE = 0x00;
    private static final int COPY_TYPE = 0x01;
    private static final int DELETE_TYPE = 0x02;
    private boolean deleteAfterCopy = false;
    private boolean copyFromSamba = false;
    private String fileToOperate = null;

    private static final int PICTURE = 0;
    private static final int MUSIC = 1;
    private static final int VIDEO = 2;
    private static final int GIF = 3;

    private static final int BROWSER_LOCAL = 0x00;
    private static final int BROWSER_SAMBA = 0x01;
    private static LinkAddress mLinkAddress = null;
    private long mLastClickTime = 0;
    ArrayList<StorageVolume> mStorageVolumesList;
    private BroadcastReceiver mNetStateReceiver;

    private GifPlayer mGifPlayer;
    private boolean isStop = false;

    public EventHandler(Context context, final IEventHandlerCallbacks mCallbacks1,
            MediaProvider mProvider, DeviceManager mDevices1) {
        this.mContext = context;
        this.mCallbacks = mCallbacks1;
        mDataSource = new ArrayList<String>();
        mMedia = mProvider;
        this.mDevices = mDevices1;
        mNfs = new NfsManagerWrapper(mContext);
        mSamba = new SambaManager(mContext);

        mPathStack = new Stack<Integer>();
        mPathStack.push(Integer.valueOf(0));

        mTimer = new Timer();
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                //use AsyncTask to operate file, because nfs/samba maybe cause ANR
                switch (msg.what) {
                    case PICTURE:
                        Integer[] rect = {imageThumb.getWidth(), imageThumb.getHeight()};
                        new AsyncTask<Integer, Void, Void>(){
                            @Override
                            protected Void doInBackground(Integer... params){
                                handlePictureMessage(params[0], params[1]);
                                return null;
                            }
                        }.execute(rect);
                        break;
                    case VIDEO:
                        new AsyncTask<Void, Void, Void>(){
                            @Override
                            protected Void doInBackground(Void... params){
                                mCallbacks1.playThumbVideo(videoPath);
                                return null;
                            }
                        }.execute();
                        break;
                    case GIF:
                        final String gifPath = msg.getData() != null ? msg.getData().getString("KEY_GIF_PATH") : "";
                        new AsyncTask<Void, Void, Void>(){
                            @Override
                            protected Void doInBackground(Void... params){
                                handleGifMessage(gifPath);
                                return null;
                            }
                        }.execute();
                        break;
                    default:
                        break;
                }
            }
        };

        /* 设置默认的过滤 */
        mFilter = mMedia.alltypeFilter;

        /* 保存设备分区名字的映射 */
        pf = mContext.getSharedPreferences("partition", 0);
        editor = pf.edit();

        IntentFilter mNetFilter = new IntentFilter();
        mNetFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        mNetStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                ConnectivityManager cm = (ConnectivityManager) mContext
                        .getSystemService(Context.CONNECTIVITY_SERVICE);
                if (cm != null) {
                    NetworkInfo info = cm.getActiveNetworkInfo();
                    if (info != null && info.isConnected()) {
                        //LinkProperties link = cm.getActiveLinkProperties();
                        LinkProperties link = null;
                        Class<ConnectivityManager> cls_conn = ConnectivityManager.class;
                        try {
                            Method mGetActiveLinkProperties = cls_conn.getDeclaredMethod("getActiveLinkProperties");
                            mGetActiveLinkProperties.setAccessible(true);
                            link = (LinkProperties) mGetActiveLinkProperties.invoke(cm);
                        } catch (NoSuchMethodException e) {
                            e.printStackTrace();
                        } catch (IllegalAccessException e) {
                            e.printStackTrace();
                        } catch (InvocationTargetException e) {
                            e.printStackTrace();
                        }
                        if(link!=null) {
                            for (LinkAddress l : link.getLinkAddresses()) {
                                    mLinkAddress = l;
                                    if (l.getAddress() instanceof Inet4Address) {
                                        int networkPrefixLength = 0;// == l.getNetworkPrefixLength()
                                        Class<LinkAddress> cls_linkAddress = LinkAddress.class;
                                        try {
                                            Method mGetNetworkPrefixLength = cls_linkAddress.getDeclaredMethod("getNetworkPrefixLength");
                                            mGetNetworkPrefixLength.setAccessible(true);
                                            networkPrefixLength = (int) mGetNetworkPrefixLength.invoke(l);
                                        } catch (NoSuchMethodException e) {
                                            e.printStackTrace();
                                        } catch (IllegalAccessException e) {
                                            e.printStackTrace();
                                        } catch (InvocationTargetException e) {
                                            e.printStackTrace();
                                        }
                                        LocalNetwork.setCurIpv4(l.getAddress().getHostAddress(),networkPrefixLength);
                                        if (isInSambaMode() || isInSambaMountDir()) {
                                            Log.d(TAG, "samba: mLinkAddress changed, go to rootDir");
                                            getDeviceList();
                                            mTable.notifyDataSetChanged();
                                            getDetailForPosition(1);
                                            list.setSelection(1);
                                        } else if (isInNfsMode()) {
                                            Log.d(TAG, "nfs: mLinkAddress changed, go to rootDir");
                                            getDeviceList();
                                            mTable.notifyDataSetChanged();
                                            getDetailForPosition(2);
                                            list.setSelection(2);
                                        }
                                        break;
                                    }
                            }
                        }
                    } else {
                        if (isInSambaMode() || isInSambaMountDir()) {
                            Log.d(TAG, "samba: network disconnected, go to rootDir");
                            getDeviceList();
                            mTable.notifyDataSetChanged();
                            getDetailForPosition(1);
                            list.setSelection(1);
                        } else if (isInNfsMode()) {
                            Log.d(TAG, "nfs: network disconnected, go to rootDir");
                            getDeviceList();
                            mTable.notifyDataSetChanged();
                            getDetailForPosition(2);
                            list.setSelection(2);
                        }
                    }
                }
            }
        };
        mContext.registerReceiver(mNetStateReceiver, mNetFilter);
    }

    private void handlePictureMessage(int width, int height){
        Rect rect = getPicResolution(picPath);
        float radio = ((float) rect.height()) / rect.width();
        if (rect.width() == 0 || rect.height() == 0) {
            int k = 0;
        } else if ((((float) height) / width) > radio) {
            height = width * rect.height() / rect.width();
        } else {
            width = height * rect.width() / rect.height();
        }
        Bitmap bm = mMedia.getImageThumbFromMK(picPath, width, height);
        if (picPath.equals(mDataSource.get(currentPosition))) {
            ((Activity)mContext).runOnUiThread(new Runnable(){
                public void run(){
                    imageThumb.setBackground(null);
                    imageThumb.setImageBitmap(bm);
                    getPicDetail(picPath, rect.width(), rect.height());
                }
            });
        }
    }

    private void playGif(String path) {
        Log.d(TAG, "begin play gif");
        String errMsg = mContext.getResources().getString(R.string.not_app_to_open_the_gif);
        openImageParser(path,errMsg);
    }

    private void showGifMessage(String path1) {
        Rect rect = getPicResolution(path1);
        getPicDetail(path1, rect.width(), rect.height());
        Message msg = new Message();
        msg.what = GIF;
        Bundle bundle = new Bundle();
        bundle.putString("KEY_GIF_PATH",path1);
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }

    private void handleGifMessage(String filePath) {
        Log.d(TAG,"handleGifMessage");
        isStop = false;
        releaseGifPlayer();
        if(mGifPlayer == null) {
            mGifPlayer = new GifPlayer();
        }
        mGifPlayer.play(filePath,true);
        mGifPlayer.setOnDrawGifCallBack(new GifPlayer.OnDrawGifCallBack() {

            @Override
            public void onDrawFrame(final Bitmap bitmap) {
                ((Activity)mContext).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (!isStop) {
                            imageThumb.setImageBitmap(bitmap);
                        }
                    }
                });
            }

            @Override
            public void onError(final String msg) {
                ((Activity)mContext).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mContext,msg,Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });

    }

    private void releaseGifPlayer(){
        if(mGifPlayer != null) {
            Log.d(TAG,"releaseGifPlayer");
            mGifPlayer.release();
            mGifPlayer = null;
            isStop = true;
        }
    }

    public void setViewResource(SurfaceView video, ImageView image, MTextView preview1,
            TextView path1, TextView index1) {
        this.videoThumb = video;
        this.imageThumb = image;
        this.preview = preview1;
        this.path = path1;
        this.index = index1;
    }

    @Override
    public void onClick(View view) {
        int id = view.getId();
        currentNavigation = id;
        setButtonSelected(view);

        mCallbacks.releaseMediaPlayerSync();
        releaseGifPlayer();

        //imageThumb.setVisibility(View.VISIBLE);
        videoThumb.setVisibility(View.GONE);
        mTimer.cancel();
        if (currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD)) {
            return;
        }
        showNothing();

        switch (id) {
            case R.id.device_button:
                mFilter = mMedia.alltypeFilter;
                getDeviceList();
                break;
            case R.id.video_button:
                mFilter = mMedia.movieFilter;
                if (currentDir.equals(rootPath)) {
                    getDeviceList();
                } else {
                    scanDir(currentDir, NONE);
                }
                break;
            case R.id.picture_button:
                mFilter = mMedia.pictureFilter;
                if (currentDir.equals(rootPath)) {
                    getDeviceList();
                } else {
                    scanDir(currentDir, NONE);
                }
                break;
            case R.id.music_button:
                mFilter = mMedia.musicFilter;
                if (currentDir.equals(rootPath)) {
                    getDeviceList();

                } else {
                    scanDir(currentDir, NONE);
                }
                break;
            case R.id.file_button:
                mFilter = mMedia.alltypeFilter;
                if (currentDir.equals(rootPath)) {
                    getDeviceList();
                } else {
                    scanDir(currentDir, NONE);
                }
                break;
            default:
                break;
        }
    }

    public void saveBrowseDir() {
        browseDir = currentDir;
    }

    public void loadBrowseDir() {
        if (browseDir != null) {
            currentDir = browseDir;
        }
    }

    private String getData(int position) {
        if (position > mDataSource.size() - 1 || position < 0) {
            Log.d(TAG, "MediaProvider.RETURN=" + MediaProvider.RETURN);
            return MediaProvider.RETURN;
        }
        return mDataSource.get(position);
    }

    public void getDeviceList() {
        mDataSource.clear();
        currentDir = rootPath;
        currentPosition = 0;
        mStorageVolumesList=mDevices.getMountedDevicesList();
        for(StorageVolume sv:mStorageVolumesList) {
            mDataSource.add(sv.getPath());
        }
        /*
         * if (list.requestFocusFromTouch()) { } if (mDataSource.size() > 0) {
         * // 判断哪些设备有多分区 for (int i = 0; i < mDataSource.size(); i++) { if
         * (mDevices.hasMultiplePartition(mDataSource.get(i))) {
         * mapPartitionName(mDataSource.get(i)); } } list.setSelection(0); }
         */
        mDataSource.add(mContext.getResources().getString(R.string.samba));
        mDataSource.add(mContext.getResources().getString(R.string.nfs));
        ArrayList<String> netlist = mDevices.getNetDeviceList();
        if (netlist != null) {
            mDataSource.addAll(netlist);
        }
        mTable.notifyDataSetChanged();
        //update Detail info when switch other tag,such as Video,Picture
        getDetailForPosition(currentPosition);
        list.setSelection(currentPosition);
    }

    private void mapPartitionName(String devices) {
        /* 映射其分区名,要区分插拔时卸载失败导致的假分区 */
        File f = new File(devices);
        String partition;
        File[] list1 = f.listFiles();
        Comparator fileNameSort = new Comparator<File>() {
            @Override
            public int compare(File f1, File f2) {
                try {
                    String filePath1 = f1.getAbsolutePath();
                    String filePath2 = f2.getAbsolutePath();
                    String str1 = filePath1.substring(filePath1.lastIndexOf("/") + 1);
                    String str2 = filePath2.substring(filePath2.lastIndexOf("/") + 1);
                    return str1.compareToIgnoreCase(str2);
                } catch (IndexOutOfBoundsException e) {
                    e.printStackTrace();
                    return -1;
                }
            }
        };
        Arrays.sort(list1, fileNameSort);
        String[] map = {mContext.getResources().getString(R.string.partitionA),
                mContext.getResources().getString(R.string.partitionB),
                mContext.getResources().getString(R.string.partitionC),
                mContext.getResources().getString(R.string.partitionD),
                mContext.getResources().getString(R.string.partitionE),
                mContext.getResources().getString(R.string.partitionF),
                mContext.getResources().getString(R.string.partitionG),
                mContext.getResources().getString(R.string.partitionH),
                mContext.getResources().getString(R.string.partitionI),
                mContext.getResources().getString(R.string.partitionJ),
                mContext.getResources().getString(R.string.partitionK),
                mContext.getResources().getString(R.string.partitionL),
                mContext.getResources().getString(R.string.partitionM),
                mContext.getResources().getString(R.string.partitionN),
                mContext.getResources().getString(R.string.partitionO),
                mContext.getResources().getString(R.string.partitionP),
                mContext.getResources().getString(R.string.partitionQ),
                mContext.getResources().getString(R.string.partitionR),
                mContext.getResources().getString(R.string.partitionS),
                mContext.getResources().getString(R.string.partitionT),
                mContext.getResources().getString(R.string.partitionU),
                mContext.getResources().getString(R.string.partitionV),
                mContext.getResources().getString(R.string.partitionW),
                mContext.getResources().getString(R.string.partitionX),
                mContext.getResources().getString(R.string.partitionY),
                mContext.getResources().getString(R.string.partitionZ)};
        int j = 0;
        for (int i = 0; i < list1.length; i++) {
            // partition = list1[i].substring(list1[i].lastIndexOf("/") + 1);
            partition = list1[i].getAbsolutePath();

            try {
                Log.d("chen", "partition:" + partition + "   start--------");
                StatFs statFs = new StatFs(partition);
                Log.d("chen", "----------------------end");
                if (statFs.getBlockCount() == 0) {
                    continue;
                }
            } catch (Exception e) {
                continue;
            }
            if (j < map.length) {
                editor.putString(partition, map[j]);
                j++;
            } else {
                editor.putString(partition,
                        mContext.getResources().getString(R.string.partitionOther));
            }
        }
        editor.commit();
    }

    private String getMappedName(String name) {
        return pf.getString(name, name);
    }

    private String usbPath2Device(String path){
        if(path.indexOf("Storage01")>0){
            return mContext.getResources().getString(R.string.partitionA);
        }else if(path.indexOf("Storage02")>0){
            return mContext.getResources().getString(R.string.partitionB);
        }else if(path.indexOf("Storage03")>0){
            return mContext.getResources().getString(R.string.partitionC);
        }else if(path.indexOf("Storage04")>0){
            return mContext.getResources().getString(R.string.partitionD);
        }else if(path.indexOf("Storage05")>0){
            return mContext.getResources().getString(R.string.partitionE);
        }else if(path.indexOf("Storage06")>0){
            return mContext.getResources().getString(R.string.partitionF);
        }else if(path.indexOf("Storage07")>0){
            return mContext.getResources().getString(R.string.partitionF);
        }else if(path.indexOf("Storage08")>0){
            return mContext.getResources().getString(R.string.partitionG);
        }else if(path.indexOf("Storage09")>0){
            return mContext.getResources().getString(R.string.partitionH);
        }else if(path.indexOf("Storage10")>0){
            return mContext.getResources().getString(R.string.partitionI);
        }else if(path.indexOf("Storage11")>0){
            return mContext.getResources().getString(R.string.partitionJ);
        }else if(path.indexOf("Storage12")>0){
            return mContext.getResources().getString(R.string.partitionK);
        }else if(path.indexOf("Storage13")>0){
            return mContext.getResources().getString(R.string.partitionL);
        }else if(path.indexOf("Storage14")>0){
            return mContext.getResources().getString(R.string.partitionM);
        }else if(path.indexOf("Storage15")>0){
            return mContext.getResources().getString(R.string.partitionN);
        }else if(path.indexOf("Storage16")>0){
            return mContext.getResources().getString(R.string.partitionO);
        }else if(path.indexOf("Storage17")>0){
            return mContext.getResources().getString(R.string.partitionP);
        }else if(path.indexOf("Storage18")>0){
            return mContext.getResources().getString(R.string.partitionQ);
        }else if(path.indexOf("Storage19")>0){
            return mContext.getResources().getString(R.string.partitionR);
        }else if(path.indexOf("Storage20")>0){
            return mContext.getResources().getString(R.string.partitionS);
        }else if(path.indexOf("Storage21")>0){
            return mContext.getResources().getString(R.string.partitionT);
        }else if(path.indexOf("Storage22")>0){
            return mContext.getResources().getString(R.string.partitionU);
        }else if(path.indexOf("Storage23")>0){
            return mContext.getResources().getString(R.string.partitionV);
        }else if(path.indexOf("Storage24")>0){
            return mContext.getResources().getString(R.string.partitionW);
        }else if(path.indexOf("Storage25")>0){
            return mContext.getResources().getString(R.string.partitionX);
        }else if(path.indexOf("Storage26")>0){
            return mContext.getResources().getString(R.string.partitionY);
        }else if(path.indexOf("Storage27")>0){
            return mContext.getResources().getString(R.string.partitionZ);
        }else{
            return mContext.getResources().getString(R.string.usb);
        }
    }
    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG,"onItemSelected pos:"+position);
        videoThumb.setVisibility(View.GONE);
        imageThumb.setVisibility(View.VISIBLE);
        if (mTimer != null) {
            mTimer.cancel();
        }
        mCallbacks.releaseMediaPlayerAsync();
        releaseGifPlayer();
        currentPosition = position;
        showNothing();
        getDetailForPosition(position);
        /*
         * if(!isListAdjusted) { adjustListSize(view); }
         */
    }

    public void setItemSelected(int position) {
        videoThumb.setVisibility(View.GONE);
        imageThumb.setVisibility(View.VISIBLE);
        if (mTimer != null) {
            mTimer.cancel();
        }
        //mCallbacks.releaseMediaPlayerAsync();
        currentPosition = position;
        showNothing();
        getDetailForPosition(position);
    }

    private void adjustListSize(View item) {
        int height = list.getHeight();
        int deviderHeight;
        int over = 0;
        if (height == 0) {
            return;
        } else {
            itemHeight = item.getHeight();
            if (itemHeight != 0) {
                LayoutParams lp = list.getLayoutParams();
                deviderHeight = list.getDividerHeight();
                over = (height % (itemHeight + deviderHeight)) - deviderHeight;
                lp.height = height - over;
                list.setPadding(list.getPaddingLeft(), list.getPaddingTop(),
                        list.getPaddingRight(), list.getPaddingBottom() + over);
                isListAdjusted = true;
            }
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
        // TODO Auto-generated method stub
        mCallbacks.releaseMediaPlayerAsync();
        releaseGifPlayer();
        Log.d(TAG, "nothing-------");
    }

    @Override
    public void onItemClick(final AdapterView<?> parent, final View view, final int position,
            final long id) {
        mLastClickTime  = SystemClock.elapsedRealtime();
        mTimer.cancel();

        mCallbacks.releaseMediaPlayerAsync();
        releaseGifPlayer();

        filePath = mDataSource.get(position);

        Log.d(TAG, "filePath: " + filePath);

        isListAdjusted = false;
        if (Config.isSupportMouse()) {
            currentPosition = position;
        }

        File file = new File(filePath);
        if (filePath.endsWith(MediaProvider.RETURN)) {
            backToPreDir();
        } else if (filePath.equals(mContext.getResources().getString(R.string.samba))) {
            if (!checkNet(mContext)) {
                Toast.makeText(mContext,
                        mContext.getResources().getString(R.string.net_not_connected),
                        Toast.LENGTH_SHORT).show();
                return;
            }
            filePath = "smb://";
            mSamba.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isListAdjusted) {
                                mDataSource.clear();
                                mDataSource.add(MediaProvider.RETURN);
                                isListAdjusted = true;
                            }
                            mDataSource.add(path1);
                            mTable.notifyDataSetChanged();
                        }
                    });
                }

                @Override
                public boolean onFinish(boolean finish) {
                    if (finish) {
                        currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
                    }
                    return false;
                }
            });
        } else if (mSamba.isSambaWorkgroup(filePath)) {
            mSamba.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isListAdjusted) {
                                mDataSource.clear();
                                mDataSource.add(MediaProvider.RETURN);
                                isListAdjusted = true;
                            }
                            mDataSource.add(path1);
                            mTable.notifyDataSetChanged();
                        }
                    });
                }

                @Override
                public boolean onFinish(boolean finish) {
                    if (finish) {
                        currentDir = filePath;
                    }
                    return false;
                }
            });
        } else if (mSamba.isSambaServices(filePath)) {
            final String smbServer = filePath;
            mSamba.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isListAdjusted) {
                                mDataSource.clear();
                                mDataSource.add(MediaProvider.RETURN);
                                isListAdjusted = true;
                            }
                            mDataSource.add(path1);
                            mTable.notifyDataSetChanged();
                        }
                    });
                }

                @Override
                public boolean onFinish(boolean finish) {
                    if (finish) {
                        currentDir = filePath;
                    }
                    return false;
                }
            });
        } else if (mSamba.isSambaShare(filePath)) {
            mSamba.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            goIntoDir(path1);
                        }
                    });

                }

                @Override
                public boolean onFinish(boolean finish) {
                    return false;
                }
            });
        } else if (filePath.equals(mContext.getResources().getString(R.string.nfs))) {
            if (!checkNet(mContext)) {
                Toast.makeText(mContext,
                        mContext.getResources().getString(R.string.net_not_connected),
                        Toast.LENGTH_SHORT).show();
                return;
            }
            mNfs.startSearch(filePath, new OnSearchListener() {
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isListAdjusted) {
                                mDataSource.clear();
                                mDataSource.add(MediaProvider.RETURN);
                                isListAdjusted = true;
                            }
                            mDataSource.add(path1);
                            mTable.notifyDataSetChanged();
                        }
                    });
                }

                public boolean onFinish(boolean finish) {
                    if (!finish) {
                        currentDir = rootPath;
                        return false;
                    }
                    // Mark current dir is NFS_SHARE
                    currentDir = MediaProvider.NFS_SHARE;
                    return true;
                }
            });
        } else if (mNfs.isNfsServer(filePath)) {
            final String nfsServer = filePath;
            mNfs.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isListAdjusted) {
                                mDataSource.clear();
                                mDataSource.add(MediaProvider.RETURN);
                                isListAdjusted = true;
                            }
                            mDataSource.add(path1);
                            mTable.notifyDataSetChanged();
                        }
                    });
                }

                @Override
                public boolean onFinish(boolean finish) {
                    if (finish) {
                        Log.d(TAG, "Search nfs shared success");
                        currentDir = filePath;
                    }
                    return finish;
                }
            });
        } else if (mNfs.isNfsShare(filePath)) {
            mNfs.startSearch(filePath, new OnSearchListener() {
                @Override
                public void onReceiver(final String path1) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            goIntoDir(path1);
                        }
                    });
                }

                @Override
                public boolean onFinish(boolean finish) {
                    return false;
                }
            });
        } else if (isDeviceList() && !mDevices.isDeviceMounted(filePath)) {
            boolean mountResule = mDevices.doMount(filePath);
            if (mountResule) {
                goIntoDir(mDataSource.get(currentPosition));
            } else {
                Toast.makeText(mContext, mContext.getText(R.string.try_mount_fail),
                        Toast.LENGTH_LONG).show();
            }
        } else if (file.isDirectory()) {
            if (!playBlurayFolder(filePath)) {
                goIntoDir(mDataSource.get(currentPosition));
            }
        } else {
            playFile(filePath);
        }
    }

    private boolean checkNet(Context context) {
        try {
            ConnectivityManager connectivity = (ConnectivityManager) context
                    .getSystemService(Context.CONNECTIVITY_SERVICE);
            if (connectivity != null) {

                NetworkInfo info = connectivity.getActiveNetworkInfo();
                if (info != null && info.isConnected()) {

                    if (info.getState() == NetworkInfo.State.CONNECTED) {
                        return true;
                    }
                }
            }
        } catch (Exception e) {
            return false;
        }
        return false;
    }

    private String findBlurayVideo(String path1) {
        /* 是否在设置中打开了蓝光文件夹播放功能 */
        // boolean enable =
        // Settings.System.getInt(mContext.getContentResolver(),
        // Settings.System.BD_FOLDER_PLAY_MODE, 0) != 0 ? true : false;
        boolean enable = true;
        if (!enable) {
            Log.d(TAG, "the BD_FOLDER_PLAY_MODE is unable in setting");
            return null;
        }
        /* 是否符合蓝光文件夹的目录结构 */
        String subDir = "BDMV/STREAM";
        String p = path1 + "/" + subDir;
        File f = new File(p);
        if (!f.exists() || !f.isDirectory()) {
            return null;
        }

        return path1;
        /*
         * String[] list = f.list(); if (list == null) { return null; } for
         * (String st : list) { File file = new File(f, st); if
         * (TypeFilter.isMovieFile(st)) Log.d("chen", "movie"); if
         * (file.isFile() && TypeFilter.isMovieFile(st)) { String pre =
         * st.substring(0, st.lastIndexOf(".")); Log.d("chen",
         * "findBlurayVideo()  st=" + st + "  pre=" + pre); Integer itg =
         * Integer.valueOf(pre); if (itg != null) Log.d("chen", "integer  " +
         * itg.intValue()); if (itg == null || pre.length() != 5) { continue; }
         *
         * return file.getAbsolutePath(); } } return null;
         */
    }

    private boolean playBlurayFolder(String path1) {
        String videoPath1 = findBlurayVideo(path1);
        if (videoPath1 == null) {
            Log.d(TAG, "it is not a bluray folder");
            return false;
        } else {
            Log.d("chen", "play " + videoPath1);
        }
        File file = new File(videoPath1);
        Intent bdIntent = new Intent();
        //bdIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, true);
        ComponentName cm = new ComponentName("com.softwinner.TvdVideo",
                "com.softwinner.TvdVideo.TvdVideoActivity");
        bdIntent.setComponent(cm);
        setContentUri(bdIntent,file,"video/*");
        try {
            Log.d(TAG, "begin to play bluray folder");
            mContext.startActivity(bdIntent);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "can not find app to play bluray folder");
            return false;
        }

    }

    private boolean playBlurayFolder(String path1, String realpath) {
        Log.d(TAG, "enter playBlurayFolder ");
        String videoPath1 = findBlurayVideo(path1);
        if (videoPath1 == null) {
            Log.d(TAG, "it is not a bluray folder");
            return false;
        } else {
            Log.d(TAG, "play: " + videoPath1);
        }
        File file = new File(videoPath1);
        Intent bdIntent = new Intent();
        bdIntent.putExtra("VideoPath000", realpath);
        //bdIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, true);
        ComponentName cm = new ComponentName("com.softwinner.TvdVideo",
                "com.softwinner.TvdVideo.TvdVideoActivity");
        bdIntent.setComponent(cm);
        setContentUri(bdIntent,file,"video/*");
        try {
            Log.d(TAG, "begin to play bluray folder");
            mContext.startActivity(bdIntent);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "can not find app to play bluray folder");
            return false;
        }

    }

    private void playMusic(String path1) {
        File file = new File(path1);
        Intent picIntent = new Intent();
        picIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(picIntent,file,"audio/*");
        try {
            mContext.startActivity(picIntent);
        } catch (ActivityNotFoundException e) {
            displayToast(mContext.getResources().getString(R.string.not_app_to_play_the_music));
        }
    }

    private void playVideo(String path1) {

        File file = new File(path1);
        Intent movieIntent = new Intent();
        movieIntent.setAction(android.content.Intent.ACTION_VIEW);
        //movieIntent.putExtra(MediaStore.PLAYLIST_TYPE, MediaStore.PLAYLIST_TYPE_CUR_FOLDER);
        // movieIntent.putExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, false);
        //movieIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, false);
        setContentUri(movieIntent,file,"video/*");
        movieIntent.setClassName("com.softwinner.TvdVideo","com.softwinner.TvdVideo.TvdVideoActivity");
        /* make sure that media must be reset */
        mCallbacks.releaseMediaPlayerSync();
        releaseGifPlayer();
        Log.d(TAG, "Start");
        try {
            mContext.startActivity(movieIntent);
            return;
        } catch (ActivityNotFoundException e) {
            Log.d(TAG, "no TvdVideo, use default player");
        }

        //use default player to play video if has not TvdVideo.
        Intent defaultIntent = new Intent();
        defaultIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(defaultIntent,file,"video/*");
        try {
            mContext.startActivity(defaultIntent);
        } catch (Exception e) {
            displayToast(mContext.getResources().getString(R.string.not_app_to_play_the_video));
        }
    }

    private void playPicture(String path1) {
        String errMsg = mContext.getResources().getString(R.string.not_app_to_open_the_pic);
        File file = new File(path1);
        Intent picIntent = new Intent();
        picIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(picIntent,file,"image/*");
        try {
            mContext.startActivity(picIntent);
            return;
        } catch (ActivityNotFoundException e) {
            displayToast(errMsg);
        }

        //use ImageParser to play picture if has not Gallery.
        openImageParser(path1,errMsg);
    }

    private void openImageParser(String path,String errMsg) {
        ComponentName componentName = new ComponentName("com.softwinner.ImageParser",
            "com.softwinner.ImageParser.MainActivity");
        Intent picIntent = new Intent();
        picIntent.setData(Uri.parse(path));
        picIntent.setComponent(componentName);
        try {
            mContext.startActivity(picIntent);
        } catch (ActivityNotFoundException e) {
            displayToast(errMsg);
        }
    }

    private void playPdf(String path1) {
        File file = new File(path1);
        Intent pdfIntent = new Intent();
        pdfIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(pdfIntent,file,"application/pdf");
        try {
            mContext.startActivity(pdfIntent);
        } catch (ActivityNotFoundException e) {
            selectFileTypeDialog(new File(path1));
        }

    }

    private void playApk(String path1) {
        File file = new File(path1);

        Intent apkIntent = new Intent();
        apkIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(apkIntent,file,"application/vnd.android.package-archive");
        try {
            mContext.startActivity(apkIntent);
        } catch (ActivityNotFoundException e) {
            displayToast(mContext.getResources().getString(R.string.not_app_to_open_the_file));
        }

    }

    private void playHtml(String path1) {
        File file = new File(path1);
        Intent htmlIntent = new Intent();
        htmlIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(htmlIntent,file,"text/html");
        try {
            mContext.startActivity(htmlIntent);
        } catch (ActivityNotFoundException e) {
            selectFileTypeDialog(new File(path1));
        }
    }

    private void playTxt(String path1) {
        File file = new File(path1);
        Intent txtIntent = new Intent();
        txtIntent.setAction(android.content.Intent.ACTION_VIEW);
        setContentUri(txtIntent,file,"text/plain");
        try {
            mContext.startActivity(txtIntent);
        } catch (ActivityNotFoundException e) {
            displayToast(mContext.getResources().getString(R.string.not_app_to_open_the_file));
        }
    }

    private void playIso(String path1) {
        if (path1 == null) {
            Log.e(TAG, "playIso Path is null.");
        } else {
            Log.d("playIso", " play " + path1);
            File file = new File(path1);
            Intent bdIntent = new Intent();
            //bdIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, false);
            ComponentName cm = new ComponentName("com.softwinner.TvdVideo",
                    "com.softwinner.TvdVideo.TvdVideoActivity");
            bdIntent.setComponent(cm);
            setContentUri(bdIntent,file,"video/*");

            try {
                Log.d(TAG, "begin to play iso");
                mContext.startActivity(bdIntent);
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "can not find app to play iso");
            }
        }
    }
    private void setContentUri(Intent intent,File file,String type){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                // Android7.0及以上版本
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                //Uri contentUri = FileProvider.getUriForFile(mContext, "应用包名" + ".fileProvider", file);//参数二:应用包名+".fileProvider"(和步骤一中的Manifest文件中的provider节点下的authorities对应)
                Uri contentUri = FileProvider.getUriForFile(mContext, "com.softwinner.TvdFileManager.fileProvider", file);
                intent.setDataAndType(contentUri, type);

            } else {
                // Android7.0以下版本
                intent.setDataAndType(Uri.fromFile(file), type);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            }
    }
    private String openType = null;

    private void selectFileTypeDialog(final File openFile) {
        String mFile = mContext.getResources().getString(R.string.open_file);
        String mText = mContext.getResources().getString(R.string.text);
        String mAudio = mContext.getResources().getString(R.string.music);
        String mVideo = mContext.getResources().getString(R.string.video);
        String mImage = mContext.getResources().getString(R.string.picture);
        CharSequence[] fileType = {mText, mAudio, mVideo, mImage};
        AlertDialog.Builder builder;
        AlertDialog dialog;
        builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mFile);
        builder.setIcon(R.drawable.help);
        builder.setItems(fileType, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                // TODO Auto-generated method stub
                Intent mIntent = new Intent();
                switch (which) {
                    case 0:
                        openType = "text/*";
                        break;
                    case 1:
                        openType = "audio/*";
                        break;
                    case 2:
                        openType = "video/*";
                        break;
                    case 3:
                        openType = "image/*";
                        break;
                    default:
                        break;
                }
                mIntent.setAction(android.content.Intent.ACTION_VIEW);
                mIntent.setDataAndType(Uri.fromFile(openFile), openType);
                setContentUri(mIntent,openFile,openType);
                try {
                    mContext.startActivity(mIntent);
                } catch (ActivityNotFoundException e) {
                    displayToast(mContext.getResources().getString(
                            R.string.not_app_to_open_the_file));
                }
            }
        });
        dialog = builder.create();
        dialog.show();
    }

    private void playFile(String path1) {
        if (mCallbacks.returnFile(new File(path1)) || mCallbacks.returnContentUri(path1)) {
            return;
        }

        if (TypeFilter.isMovieFile(path1)) {
            playVideo(path1);
        } else if (TypeFilter.isMusicFile(path1)) {
            playMusic(path1);
        } else if (TypeFilter.isPictureFile(path1)) {
            if (TypeFilter.isGifType(path1)){
                playGif(path1);
            } else {
                playPicture(path1);
            }
        } else if (TypeFilter.isApkFile(path1)) {
            playApk(path1);
        } else if (TypeFilter.isTxtFile(path1)) {
            playTxt(path1);
        } else if (TypeFilter.isHtml32File(path1)) {
            playHtml(path1);
        } else if (TypeFilter.isPdfFile(path1)) {
            playPdf(path1);
        } else if (TypeFilter.isISOFile(path1)) {
            playIso(path1);
        } else {
            selectFileTypeDialog(new File(path1));
        }
    }

    private void showNothing() {
        ((Activity)mContext).runOnUiThread(new Runnable(){
            public void run(){
                imageThumb.setBackgroundResource(R.drawable.thumbnail_bg);
                imageThumb.setImageResource(R.drawable.thumbnail_equipment);
                preview.setBackgroundDrawable(null);
                preview.setText2("");
                index.setText("");
                path.setText("");
            }
        });
    }

    private void showDeviceMessagePre(){
        imageThumb.setBackgroundResource(R.drawable.thumbnail_bg);
        imageThumb.setImageResource(R.drawable.thumbnail_equipment);
        preview.setBackgroundResource(R.drawable.preview);
    }
    private String showDeviceMessageExec(String path1) {
        String target = null;
        target = getDeviceName(path1);
        try {
            long totalsize = 0;
            long availsize = 0;
            long usedsize = 0;
            if (mDevices.hasMultiplePartition(path1)) {
                File f = new File(path1);
                File[] list1 = f.listFiles();
                for (int i = 0; i < list1.length; i++) {
                    totalsize += getTotalSize(list1[i].getAbsolutePath());
                    availsize += getAvailableSize(list1[i].getAbsolutePath());
                }
                usedsize = totalsize - availsize;
            } else {
                totalsize = getTotalSize(path1);
                availsize = getAvailableSize(path1);
                usedsize = totalsize - availsize;
            }
            String availSize = toSize(availsize);
            String usedSize = toSize(usedsize);

            String target1 = mContext.getResources().getString(R.string.target) + target + "\n";
            String usedsize1 = mContext.getResources().getString(R.string.used_size) + usedSize
                    + "\n";
            String availsize1 = mContext.getResources().getString(R.string.avail_size) + availSize;
            String display1 = target1 + usedsize1 + availsize1;

            return display1;

        } catch (Exception e) {
            Log.e(TAG, "fail to catch the size of the devices");
        }
        return null;

    }

    private void showDeviceMessage(String path1){
        showDeviceMessagePre();
        new AsyncTask<String, Void, String>(){
            @Override
            protected String doInBackground(String... params){
                return showDeviceMessageExec(params[0]);
            }

            @Override
            protected void onPostExecute(String value){
                preview.setText2(value);
            }
        }.execute(path1);
    }
    /* 获取全部空间,..GB */
    private long getTotalSize(String path1) {
        StatFs statfs = new StatFs(path1);
        long totalBlocks = statfs.getBlockCount();
        long blockSize = statfs.getBlockSize();
        long totalsize = blockSize * totalBlocks;
        return totalsize;
    }

    /* 获取可用空间 */
    private long getAvailableSize(String path1) {
        StatFs statfs = new StatFs(path1);
        long blockSize = statfs.getBlockSize();
        long availBlocks = statfs.getAvailableBlocks();
        long availsize = blockSize * availBlocks;
        return availsize;
    }

    private void showMusicMessagePre(){
        imageThumb.setBackgroundResource(R.drawable.thumbnail_bg);
        imageThumb.setImageResource(R.drawable.thumbnail_music);
        preview.setBackgroundResource(R.drawable.preview);
    }

    private String showMusicMessageExec(String path1) {
        final String externalVolume = "external";
        final int pathIndex = 0;
        final int artistIndex = 1;
        final int durationIndex = 2;
        final int titleIndex = 3;

        String singer1 = null;
        String title1 = null;
        String duration1 = null;
        String size1 = null;

        String[] projection = {MediaStore.Audio.Media.DATA, MediaStore.Audio.Media.ARTIST,
                MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.TITLE};
        Uri uri = MediaStore.Audio.Media.getContentUri(externalVolume);
        ContentResolver cr = mContext.getContentResolver();
        String[] selectionArgs = {path1};

        Cursor c = cr.query(uri, projection, MediaStore.Audio.Media.DATA + "=?", selectionArgs,
                null);
        if (c != null) {
            try {
                while (c.moveToNext()) {
                    String filePath1 = c.getString(pathIndex);
                    singer1 = mContext.getResources().getString(R.string.singer)
                            + c.getString(artistIndex) + "\n";
                    title1 = mContext.getResources().getString(R.string.title)
                            + c.getString(titleIndex) + "\n";
                    long time ;
                    if (!TextUtils.isEmpty(c.getString(durationIndex)) && TextUtils.isDigitsOnly(c.getString(durationIndex))) {
                            time = Integer.valueOf(c.getString(durationIndex)).longValue();
                    } else {
                            time = 0;
                    }
                    duration1 = mContext.getResources().getString(R.string.duration)
                            + toDuration(time) + "\n";
                    size1 = mContext.getResources().getString(R.string.size)
                            + toSize(new File(path1).length());
                }
            } finally {
                c.close();
                c = null;
            }
        } else {
            Log.d(TAG, "cursor is null");
        }
        if (singer1 == null || title1 == null || duration1 == null || size1 == null) {
            /* 不能获取该信息,返回 */
            return null;
        }
        String display1 = singer1 + title1 + duration1 + size1;
        return display1;
    }

    private void showMusicMessage(String path1){
        showMusicMessagePre();
        new AsyncTask<String, Void, String>(){
            @Override
            protected String doInBackground(String... params){
                return showMusicMessageExec(params[0]);
            }

            @Override
            protected void onPostExecute(String value){
                preview.setText2(value);
            }
        }.execute(path1);
    }

    private void showVideoMessage(String path1) {
        imageThumb.setBackgroundResource(R.drawable.thumbnail_bg);
        imageThumb.setImageResource(R.drawable.thumbnail_video);
        preview.setBackgroundResource(R.drawable.preview);
        videoPath = path1;
        // 延时播放
        Message msg = new Message();
        msg.what = VIDEO;
        mHandler.sendMessageDelayed(msg, VIDEO_DELAY);
    }

    public void removeVideoMessage() {
        mHandler.removeMessages(VIDEO);
    }

    private void showPictureMessage(String path1) {
        /* 获取图片缩略图 */
        Bitmap thumb = mMedia.getImageThumbFromDB(path1);
        if (thumb != null) {
            imageThumb.setBackground(null);
            imageThumb.setImageBitmap(thumb);
            Rect rect = getPicResolution(path1);
            getPicDetail(path1, rect.width(), rect.height());
        } else {
            /* 先设置一个默认图片 */
            imageThumb.setBackgroundResource(R.drawable.thumbnail_bg);
            imageThumb.setImageResource(R.drawable.thumbnail_picture);
            /* 如果不能直接从数据库中获得其缩略图文件,则需要延缓时间再解码生成 */
            picPath = path1;
            if (mTimer != null) {
                mTimer.cancel();
                mTimer = null;
            }
            mTimer = new Timer();
            mTask = new TimerTask() {

                @Override
                public void run() {
                    // 延时播放
                    Message msg = new Message();
                    msg.what = PICTURE;
                    mHandler.sendMessage(msg);
                }
            };
            mTimer.schedule(mTask, PIC_DELAY);
        }

    }

    private Rect getPicResolution(String path1) {
        // 获取图片分辨率
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path1, options);
        Rect rect = new Rect(0, 0, options.outWidth, options.outHeight);
        return rect;
    }

    private void getPicDetail(String path1, int width, int height) {
        // 获取图片大小
        String s = toSize(new File(path1).length());
        String size1 = mContext.getResources().getString(R.string.size) + s + "\n";
        String wPic = String.valueOf(width);
        String hPic = String.valueOf(height);
        String resolution1 = mContext.getResources().getString(R.string.resolution) + wPic + "*"
                + hPic;

        String display1 = size1 + resolution1;
        preview.setText2(display1);
    }

    private void showFileIndex(String path1, int position) {
        showPath(path1);
        showIndex(position);
    }

    private void showPath(String path1) {
        // 显示文件路径
        String dir = mContext.getResources().getString(R.string.directory);
        String type = getTypeName(path1);
        String dev = getDeviceName(path1);
        String filepath;
        if (path1.equals(MediaProvider.RETURN)) {
            filepath = dir + type;
        } else if (currentDir.equals(rootPath)) {
            filepath = dir + type + "/" + dev;
        } else {
            String file = null;
            try {
                file = path1.substring(path1.lastIndexOf("/") + 1);
            } catch (NullPointerException e) {
                e.printStackTrace();
                file = "";
            }
            filepath = dir + ":" + type + "/" + dev + "/" + file;
        }
        this.path.setText(filepath);
    }

    private void showIndex(int position) {
        // 显示item在列表中的索引
        String index1 = String.valueOf(position + 1) + "/" + String.valueOf(mDataSource.size());
        this.index.setText(index1);
    }

    private String getDeviceName(String path1) {
        String target = "";
        ArrayList<StorageVolume> list1 = mDevices.getLocalDevicesList();
        Iterator it1 = list1.iterator();
        while(it1.hasNext()){
            StorageVolume item =(StorageVolume) it1.next();
            if (path1.startsWith(item.getPath())) {
                    if (mDevices.isInterStoragePath(item.getPath())) {
                        target = mContext.getResources().getString(R.string.flash);
                    } else if (mDevices.isSdStoragePath(item.getId())) {
                        target = mContext.getResources().getString(R.string.sdcard);
                    } else if (mDevices.isUsbStoragePath(item.getId())) {
                        target = mContext.getResources().getString(R.string.usb);
                    } else if (mDevices.isSataStoragePath(item.getPath())) {
                        target = mContext.getResources().getString(R.string.sata);
                    }
                    return target;
            }
        }

        return target;
    }

    private String getTypeName(String path1) {
        String music = mContext.getResources().getString(R.string.music);
        String picture = mContext.getResources().getString(R.string.picture);
        String video = mContext.getResources().getString(R.string.video);
        String all = mContext.getResources().getString(R.string.file);
        String device = mContext.getResources().getString(R.string.equipment);
        switch (currentNavigation) {
            case R.id.music_button:
                return music;
            case R.id.picture_button:
                return picture;
            case R.id.video_button:
                return video;
            case R.id.file_button:
                return all;
            case R.id.device_button:
                return device;
            default:
                break;
        }
        return "";
    }


    public String toSize(long mbyte){
        return (mbyte < 0)? "invalid byte"
                : Formatter.formatShortFileSize(mContext, mbyte);
    }

    public String toDuration(long ms) {

        long hours;
        long minutes;
        long seconds;
        long t1;
        long t2;
        hours = ms / (1000 * 60 * 60);
        t1 = ms - hours * 1000 * 60 * 60;
        minutes = t1 / (1000 * 60);
        t2 = t1 - minutes * 1000 * 60;
        seconds = t2 / 1000;
        String duration;
        duration = String.valueOf(hours) + ":" + String.valueOf(minutes) + ":"
                + String.valueOf(seconds);
        return duration;
    }

    public void setButtonSelected(View view) {
        now = view;
        if (!now.equals(pre)) {
            if (pre != null) {
                ((Button) pre).setSelected(false);
            }
            now.setSelected(true);
            pre = now;
        }
    }

    public void setListAdapter(TableRow mTable1, ListView mlist) {
        // TODO Auto-generated method stub
        this.mTable = mTable1;
        this.list = mlist;
        this.list.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (hasFocus) {
                    /* 获取焦点时，显示条目中第一个文件的信息 */
                    //currentPosition = 0;
                    getDetailForPosition(0);
                } else {
                    /* 文件列表栏丢失焦点时，清除预览信息 */
                    mCallbacks.releaseMediaPlayerAsync();
                    releaseGifPlayer();
                    //currentPosition = -1;
                    showNothing();
                }
            }
        });
    }

    /**
     * 返回上一层，如果成功返回真
     */
    public boolean backToPreDir() {
        Log.d(TAG, "back to pre dir " + currentDir);
        videoThumb.setVisibility(View.GONE);
        imageThumb.setVisibility(View.VISIBLE);
        if (mTimer != null) {
            mTimer.cancel();
        }
        mCallbacks.releaseMediaPlayerAsync();
        releaseGifPlayer();
        showNothing();
        if (scaningFile) {
            scaningBack = true;
            return true;
        }
        /* 如果所在目录是某个设备的根目录，则返回设备总列表 根目录 */
        int i;
        int j;
        ArrayList<StorageVolume> devicesList;
        devicesList = mDevices.getLocalDevicesList();
        for (j = 0; j < devicesList.size(); j++) {
            if (currentDir.equals(devicesList.get(j).getPath())) {
                getDeviceList();
                mTable.notifyDataSetChanged();
                i = mPathStack.pop().intValue();
                //setSelection when backToPreDir anyway. Because currentPosition has update, and Selection was not
                //if (currentPosition == i) {
                    getDetailForPosition(i);
                //} else {
                    list.setSelection(i);
                //}
                saveBrowseDir();
                return true;
            }
        }
        /* 如果当前目录是samba的share文件夹的挂载点,则返回samba的share文件夹总列表 */
        if (mSamba.isSambaMountedPoint(currentDir)) {
            try {
                mPathStack.pop();
            } catch (EmptyStackException e) {
                Log.d("TvdFileManager", e.getMessage());
            }
            // mSamba.umountSmb(currentDir);
            mDataSource.clear();
            mDataSource.add(MediaProvider.RETURN);
            ArrayList<SmbFile> lst = mSamba.getAllSmbShared();
            for (SmbFile file : lst) {
                mDataSource.add(file.getPath());
            }
            mTable.notifyDataSetChanged();
            try {
                currentDir = mSamba.getAllSmbServices().get(0).getPath();
            } catch (Exception e) {
                currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
            }
            list.setSelection(0);
            return true;
        } else if (mSamba.isSambaServices(currentDir)) {
            mDataSource.clear();
            if (mSamba.getAllSmbWorkgroup().size() == 0) {
                getDeviceList();
                mTable.notifyDataSetChanged();
                getDetailForPosition(1);
                list.setSelection(1);
            } else {
                mDataSource.add(MediaProvider.RETURN);
                ArrayList<SmbFile> lst = mSamba.getAllSmbServices();
                for (SmbFile file : lst) {
                    mDataSource.add(file.getPath());
                }
                mTable.notifyDataSetChanged();
                try {
                    currentDir = mSamba.getAllSmbWorkgroup().get(0).getPath();
                } catch (Exception e) {
                    currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
                }
                list.setSelection(0);
            }
            return true;
        } else if (mSamba.isSambaWorkgroup(currentDir)) {
            mDataSource.clear();
            mDataSource.add(MediaProvider.RETURN);
            ArrayList<SmbFile> lst = mSamba.getAllSmbWorkgroup();
            for (SmbFile file : lst) {
                mDataSource.add(file.getPath());
            }
            mTable.notifyDataSetChanged();
            currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
            return true;
        } else if (currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD)) {
            mSamba.clearSambaList();
            getDeviceList();
            mTable.notifyDataSetChanged();
            getDetailForPosition(1);
            list.setSelection(1);
            return true;
        } else if (mNfs.isNfsMountedPoint(currentDir)) {
            try {
                mPathStack.pop();
            } catch (EmptyStackException e) {
                Log.d(TAG, e.getMessage());
            }
            mDataSource.clear();
            mDataSource.add(MediaProvider.RETURN);
            NFSServer server = mNfs.getServerByMountedPoint(currentDir);
            // general the server not be null
            if (server == null) {
                server = new NFSServer();
            }

            for (NFSFolder folder : server.getFolderList()) {
                mDataSource.add(NfsManagerWrapper.NFS_MARK + NfsManagerWrapper.NFS_SPLIT
                        + server.getServerIP() + NfsManagerWrapper.NFS_SPLIT
                        + folder.getFolderPath());
            }
            mTable.notifyDataSetChanged();
            try {
                currentDir = NfsManagerWrapper.NFS_MARK + NfsManagerWrapper.NFS_SPLIT
                        + mNfs.getAllNfsServers().get(0).getServerIP();
            } catch (Exception e) {
                currentDir = MediaProvider.NFS_SHARE;
            }
            list.setSelection(0);
            return true;
        } else if (mNfs.isNfsServer(currentDir)) {
            mDataSource.clear();
            mDataSource.add(MediaProvider.RETURN);
            ArrayList<NFSServer> lst = mNfs.getAllNfsServers();
            for (NFSServer server : lst) {
                mDataSource.add(NfsManagerWrapper.NFS_MARK + NfsManagerWrapper.NFS_SPLIT
                        + server.getServerIP());
            }
            mTable.notifyDataSetChanged();
            try {
                currentDir = MediaProvider.NFS_SHARE;
            } catch (Exception e) {
                currentDir = MediaProvider.NFS_SHARE;
            }
            list.setSelection(0);
            return true;
        } else if (currentDir.equals(MediaProvider.NFS_SHARE)) {
            getDeviceList();
            mTable.notifyDataSetChanged();
            getDetailForPosition(2);
            list.setSelection(2);
            return true;
        } else if (!currentDir.equals(rootPath)) {
            Log.d(TAG, "--------back to pre ");
            currentDir = currentDir.substring(0, currentDir.lastIndexOf("/"));
            saveBrowseDir();
            scanDir(currentDir, BACK_TO_PRE);
            return true;
        }
        return false;
    }

    /**
     * 进入下一层。成功返回真
     */
    public boolean goIntoDir(String path1) {
        File file = new File(path1);
        if (file.isDirectory()) {
            if (file.canRead() || (isInSambaMountDir(path1) || SmbFileOperate.canRead(file))) {
                currentDir = path1;
                saveBrowseDir();

                scanDir(path1, GO_INTO_DIR);
                return true;
            } else {
                Toast.makeText(mContext, mContext.getResources().getString(R.string.can_not_open),
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        } else {
            return false;
        }
    }

    /**
     * 获取position位置上的文件的信息,
     */
    public void getDetailForPosition(int position) {
        try {
            mCallbacks.releaseMediaPlayerSync();
            releaseGifPlayer();

            String path1 = mDataSource.get(position);

            /* 显示索引条 */
            showFileIndex(path1, position);
            /* 显示预览信息 */

            int i;
            ArrayList<StorageVolume> devicesList = mDevices.getLocalDevicesList();
            for (i = 0; i < devicesList.size(); i++) {
                if (path1.equals(devicesList.get(i).getPath())) {
                    showDeviceMessage(path1);
                    return;
                }
            }
            if (path1.equals(MediaProvider.RETURN)) {
                showDeviceMessage(currentDir);
            } else {
                if(isInSambaMountDir() || isNFSServer()){
                    new AsyncTask<String, Void, Boolean>(){
                        private File file;
                        @Override
                        protected Boolean doInBackground(String... params){
                            file =new File(params[0]);
                            if(!file.exists()){
                                return false;
                            }else {
                                return true;
                            }
                        }
                        @Override
                        protected void onPostExecute(Boolean value){
                            if(value){
                                if (file.isDirectory()) {
                                    showDeviceMessage(path1);
                                } else if (TypeFilter.isMovieFile(path1)) {
                                    showVideoMessage(path1);
                                } else if (TypeFilter.isMusicFile(path1)) {
                                    showMusicMessage(path1);
                                } else if (TypeFilter.isPictureFile(path1)) {
                                    if (TypeFilter.isGifType(path1)) {
                                        showGifMessage(path1);
                                    } else {
                                        showPictureMessage(path1);
                                    }
                                } else {
                                    showNothing();
                                }
                            }
                        }
                    }.execute(path1);
                }else{
                    File file = new File(path1);
                    if(!file.exists()){
                        /* 有可能显示的只是一个字符串,如"网络邻居",或者其他非文件,这时什么都不做 */
                        int k = 0;
                    }else {
                        if (file.isDirectory()) {
                            showDeviceMessage(path1);
                        } else if (TypeFilter.isMovieFile(path1)) {
                            showVideoMessage(path1);
                        } else if (TypeFilter.isMusicFile(path1)) {
                            showMusicMessage(path1);
                        } else if (TypeFilter.isPictureFile(path1)) {
                            if (TypeFilter.isGifType(path1)) {
                                showGifMessage(path1);
                            } else {
                                showPictureMessage(path1);
                            }
                        } else {
                            showNothing();
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "exception");
        }
    }

    /**
     * 扫描文件
     */
    public void scanDir(String path1, String extraFlag) {
        mDataSource.clear();
        currentDir = path1;
        String[] st = {path1, extraFlag};
        mTable.notifyDataSetChanged();
        scaningFile = true;
        new BackgroundWork(SCANFILES).execute(st);
    }

    /**
     * 清除暂存区内容
     */
    public void clear() {
        Log.d("umount", "EventHandler.clear");
        mNfs.clear();
        mCallbacks.releaseMediaPlayerAsync();
        releaseGifPlayer();
        mMedia.clearThumbnailData();
        mMedia.closeDB();
        mSamba.clear();
        mContext.unregisterReceiver(mNetStateReceiver);
    }

    public void isoClear() {
    }

    /**
     * 设置过滤类型
     */
    public void setFilterType(FileFilter filter) {
        mFilter = filter;
    }

    public boolean isDeviceList() {
        if (currentDir.equals(rootPath)) {
            return true;
        }
        return false;
    }

    public boolean isPartitionList() {
        try {
            File f = new File(currentDir);
            if (!f.exists() || !f.isDirectory()) {
                return false;
            }
            return (mDevices.hasMultiplePartition(currentDir));
        } catch (Exception e) {
            return false;
        }
    }

    public boolean isInSambaMode() {
        if (currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD)
                || mSamba.isSambaWorkgroup(currentDir)
                || mSamba.isSambaServices(currentDir)) {
            return true;
        }
        return false;
    }

    public boolean isInSambaMountDir() {
        return (mSamba.isSambaMountDir(currentDir));
    }

    public boolean isInSambaMountDir(String path1) {
        return (mSamba.isSambaMountDir(path1));
    }

    public boolean isInNfsMode() {
        if (currentDir.equals(MediaProvider.NFS_SHARE) || mNfs.isNfsServer(currentDir)
                || mNfs.isNfsMountedPoint(currentDir)) {
            return true;
        }
        return false;
    }

    /**
     * Current folder is NFS Share, list all NFS Server
     *
     * @return
     */
    public boolean isNFSShare() {
        if (currentDir.equals(MediaProvider.NFS_SHARE)) {
            return true;
        }
        return false;
    }

    /**
     * Current folder is NFS Server, list all Server shared folder
     *
     * @return
     */
    public boolean isNFSServer() {
        if (mNfs.isNfsServer(currentDir)) {
            return true;
        }
        return false;
    }

    public boolean isReturnItemSelected() {
        try {
            if (currentPosition >= 0 && mDataSource.size() > currentPosition) {
                String name = mDataSource.get(currentPosition);
                return (name.equals(MediaProvider.RETURN));
            } else {
                return true;
            }

        } catch (Exception e) {
            return true;
        }
    }

    public String getDeviceSelectedPath() {
        String path1 = null;
        if (currentDir == rootPath && currentPosition >= 0
                && mDataSource.size() > currentPosition) {
            path1 = mDataSource.get(currentPosition);
        }
        return path1;
    }

    public boolean isNetDevSelected() {
        if (currentDir != rootPath) {
            return false;
        }
        String name = null;
        if (currentPosition >= 0 && mDataSource.size() > currentPosition) {
            name = mDataSource.get(currentPosition);
            String[] attrs = name.split("\\|");
            if (attrs.length != 2) {
                return false;
            }
            if (!attrs[0].equals("nfs") && !attrs[0].equals("smb")) {
                return false;
            }

            return true;
        }
        return false;
    }

    /* 判断是否可以操作文件:复制/粘贴... */
    public boolean hasFileOperate() {
        return (fileToOperate != null);
    }

    /* 定义文件操作 */
    public void searchForFile(String name) {
        new BackgroundWork(SEARCH_TYPE).execute(name);
    }

    private void showDeleteDialog(String name) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mContext.getResources().getString(R.string.delete));
        builder.setMessage(mContext.getResources().getString(R.string.delete_message));
       // builder.setIcon(R.mipmap.ic_launcher_round);
        //点击对话框以外的区域是否让对话框消失
        builder.setCancelable(true);
        //设置正面按钮
        builder.setPositiveButton(mContext.getResources().getString(R.string.delete), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                deleteFile(name);
                dialog.dismiss();
            }
        });
        //设置反面按钮
        builder.setNegativeButton(mContext.getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        //显示对话框
        dialog.show();
    }

    public void deleteFile(String name) {
        /*
         * 符合以下规则之一时，操作无效，直接返回 1.删除项为“返回” 2.为某设备的根路径 3.为某设备分区路径
         */
        Log.d("DBG_FileManager", "deleteFile clearThumbnailData");
        mMedia.cleanThumbnailData(name);
        if (name.equals(MediaProvider.RETURN) || mDevices.isLocalDevicesRootPath(name)
                || mDevices.hasMultiplePartition(name.substring(0, name.lastIndexOf("/")))) {
            return;
        }
        /*
         * 符合以下规则之一时，权限不足，操作无效 1.该文件不可写
         */
        File f = new File(name);
        if (!(f.canWrite() || (isInSambaMountDir() && SmbFileOperate.canWrite(f)))) {
            displayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
            return;
        }
        new BackgroundWork(DELETE_TYPE).execute(name);
    }

    public void copyFile(String oldLocation, String newLocation) {
        String msg = "";
        File old = new File(oldLocation);
        String name = old.getName();
        File newDir = new File(newLocation);
        final String[] data = {oldLocation, newLocation};

        /*
         * 符合以下规则之一时，操作无效，直接返回 1.复制项为“返回” 2.复制项为某设备路径，或为某设备分区的路径
         * 3.粘贴时处于设备列表状态，或属于设备分区列表状态 4.复制或剪切项的粘贴路径为自己所在路径
         */
        if (oldLocation.equals(MediaProvider.RETURN)
                || mDevices.isLocalDevicesRootPath(oldLocation)
                || newLocation.equals(rootPath)
                || mDevices.hasMultiplePartition(newLocation)
                || mDevices.hasMultiplePartition(oldLocation.substring(0,
                        oldLocation.lastIndexOf("/")))) {
            return;
        }

        // do not support copy to samba dir
        /* if (isInSambaMountDir(newLocation)) {
            displayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
            return;
        } */
        /*
         * 不能粘贴到自己或自己的子目录中
         */
    oldLocation = oldLocation + "/";
        newLocation = newLocation + "/";
        if (newLocation.contains(oldLocation)) {
            if (deleteAfterCopy) {
                msg = mContext.getResources().getString(R.string.can_not_cut) + name;
            } else {
                msg = mContext.getResources().getString(R.string.can_not_copy) + name;
            }
            if (newLocation.equals(oldLocation)) {
                msg = msg + mContext.getResources().getString(R.string.target_equals_src);
            } else {
                msg = msg + mContext.getResources().getString(R.string.target_is_child);
            }
            displayToast(msg);
            return;
        }

        /* 当复制/剪切的地方有一个同名字的文件时,提示是否覆盖 */
        File newFile = new File(newDir, name);
        Log.v(TAG, "===name===" + name);
        Log.v(TAG, "===newDir===" + newDir.getPath());
        Log.v(TAG, "===newFile===" + newFile.getPath());

        if (newFile.exists()) {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            // builder.setTitle(mContext.getResources().getString(R.string.Warning));
            // builder.setIcon(R.drawable.warning);
            builder.setMessage(mContext.getResources().getString(R.string.sure_cover_file) + name);
            builder.setCancelable(false);

            builder.setNegativeButton(mContext.getResources().getString(R.string.cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
            String convice = "";
            if (deleteAfterCopy) {
                convice = mContext.getResources().getString(R.string.cut);
            } else {
                convice = mContext.getResources().getString(R.string.copy);
            }
            builder.setPositiveButton(convice, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    new BackgroundWork(COPY_TYPE).execute(data);
                }
            });
            AlertDialog alertD = builder.create();
            alertD.show();
            return;
        }
        if (newFile.exists()
                && newFile.getPath().equals(oldLocation)
                && ((newFile.isDirectory() && old.isDirectory()) || (newFile.isFile() && old
                        .isFile()))) {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            // builder.setTitle(mContext.getResources().getString(R.string.Warning));
            // builder.setIcon(R.drawable.warning);
            builder.setMessage(mContext.getResources().getString(R.string.sure_cover_file) + name);
            builder.setCancelable(false);

            builder.setNegativeButton(mContext.getResources().getString(R.string.cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
            String convice = "";
            if (deleteAfterCopy) {
                convice = mContext.getResources().getString(R.string.cut);
            } else {
                convice = mContext.getResources().getString(R.string.copy);
            }
            builder.setPositiveButton(convice, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    new BackgroundWork(COPY_TYPE).execute(data);
                }
            });
            AlertDialog alertD = builder.create();
            alertD.show();
            return;
        }

        /*
         * 符合以下规则之一时，权限不足，操作失败 1.复制项不可读， 2.如果为剪切，还需要可写权限 2.粘贴路径不可写
         */
        if (!old.exists()) {
            displayToast(mContext.getResources().getString(R.string.operate_no_exit_file));
        }

        if (!(old.canRead() || (isInSambaMountDir(oldLocation) && SmbFileOperate.canRead(old)))
                || (deleteAfterCopy && !(old.canWrite()
                || (isInSambaMountDir(oldLocation) && SmbFileOperate
                        .canWrite(old)))) || !newDir.canWrite()) {
            displayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
            return;
        }
        if (isInSambaMountDir(oldLocation)) {
            copyFromSamba = true;
        }
        new BackgroundWork(COPY_TYPE).execute(data);
    }

    private void displayToast(String str) {
        Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
    }

    private static class ViewHolder {
        ImageView icon;
        TextView text;
    }

    public class TableRow extends ArrayAdapter<String> {
        public TableRow() {
            super(mContext, R.layout.tablerow, mDataSource);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder;
            if (convertView == null) {
                LayoutInflater inflater = (LayoutInflater) mContext
                        .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                convertView = inflater.inflate(R.layout.tablerow, parent, false);
                holder = new ViewHolder();
                holder.icon = (ImageView) convertView.findViewById(R.id.row_image);
                holder.text = (TextView) convertView.findViewById(R.id.text_view);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }

            /*
             * 根据类型来选择icon
             */
            String path1 = getData(position);
            StorageVolume storageVolume = null;
            for(StorageVolume sv:mStorageVolumesList) {
                if(path1.equals(sv.getPath())) {
                    storageVolume = sv;
                        break;
                }
            }
            File file = new File(path1);
            String fileName = path1.substring(path1.lastIndexOf("/") + 1);
            if (mDevices.isInterStoragePath(path1)) {
                holder.text.setText(R.string.local_disk);
                holder.icon.setImageResource(R.drawable.litter_disk);
            } else if ((storageVolume!=null) && (mDevices.isSdStoragePath(storageVolume.getId()))) {
                holder.text.setText(R.string.sdcard);
                holder.icon.setImageResource(R.drawable.litter_sd);
            } else if ((storageVolume!=null) && (mDevices.isUsbStoragePath(storageVolume.getId()))) {
                holder.text.setText(storageVolume.getUuid()==null?"UDISK":storageVolume.getUuid());
                holder.icon.setImageResource(R.drawable.litter_usb);
            } else if (mDevices.isSataStoragePath(path1)) {
                holder.text.setText(R.string.sata);
                holder.icon.setImageResource(R.drawable.litter_sata);
            } else if (mDevices.isNetStoragePath(path1)) {
                holder.text.setText(path1);
                holder.icon.setImageResource(R.drawable.litter_net_dev);
            } else if (path1.equals(MediaProvider.RETURN)) {
                holder.text.setText("");
                holder.icon.setImageResource(R.drawable.litter_back);
            } else if (path1.equals(mContext.getResources().getString(R.string.samba))) {
                holder.text.setText(path1);
                holder.icon.setImageResource(R.drawable.litter_samba);
            } else if (path1.startsWith("smb://")) {
                if (path1.endsWith("/")) {
                    path1 = path1.substring(0, path1.length() - 1);
                    fileName = path1.substring(path1.lastIndexOf("/") + 1);
                }
                holder.text.setText(fileName);
                holder.icon.setImageResource(R.drawable.litter_samba);
            } else if (path1.equals(mContext.getResources().getString(R.string.nfs))) {
                holder.text.setText(path1);
                holder.icon.setImageResource(R.drawable.litter_nfs);
            } else {
                if (file.isDirectory()) {
                    if (mDevices.isLocalDevicesRootPath(file.getParent())) {
                        String tmp = getMappedName(path1);
                        if (tmp.equals(path1)) {
                            holder.icon.setImageResource(R.drawable.litter_file);
                        } else {
                            fileName = tmp;
                            holder.icon.setImageResource(R.drawable.litter_partition);
                        }
                    } else {
                        holder.icon.setImageResource(R.drawable.litter_file);
                    }
                } else if (TypeFilter.isMovieFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_video);
                } else if (TypeFilter.isMusicFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_music);
                } else if (TypeFilter.isPictureFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_picture);
                } else if (TypeFilter.isApkFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_apk);
                } else if (TypeFilter.isExcelFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_xls);
                } else if (TypeFilter.isHtml32File(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_html);
                } else if (TypeFilter.isPdfFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_pdf);
                } else if (TypeFilter.isPptFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_ppt);
                } else if (TypeFilter.isTxtFile(path1)) {
                    holder.icon.setImageResource(R.drawable.litter_txt);
                } else {
                    holder.icon.setImageResource(R.drawable.litter_txt);
                }
                holder.text.setText(fileName);
            }
            return convertView;
        }
    }

    public class MenuItemListener implements OnClickListener {
        private Dialog menu;

        public MenuItemListener() {
        }

        @Override
        public void onClick(View v) {
            // TODO Auto-generated method stub
            if (currentPosition == -1) {
                /* 如果列表失去了焦点，无操作 */
                menu.dismiss();
                return;
            }
            switch (v.getId()) {
            /* 以下响应的是menu操作栏的内容 */
                case R.id.sort_button:
                    selectFileTypeDialog();
                    break;
                case R.id.copy_button:
                    fileToOperate = mDataSource.get(currentPosition);

                    saveCopyFilePath(fileToOperate);

                    deleteAfterCopy = false;
                    break;
                case R.id.paste_button:
                    if ((fileToOperate != null) && (currentDir != null)) {
                        copyFile(fileToOperate, currentDir);
                        fileToOperate = null;
                    }
                    break;
                case R.id.cut_button:
                    fileToOperate = mDataSource.get(currentPosition);
                    deleteAfterCopy = true;
                    break;
                case R.id.delete_button:
                    fileToOperate = mDataSource.get(currentPosition);
                    showDeleteDialog(fileToOperate);
                    //deleteFile(fileToOperate);
                    fileToOperate = null;
                    break;
                case R.id.rename_button:
                    fileToOperate = mDataSource.get(currentPosition);
                    rename(fileToOperate);
                    fileToOperate = null;
                    break;
                case R.id.mkdir_button:
                    fileToOperate = currentDir;
                    mkdir(fileToOperate);
                    fileToOperate = null;
                    break;
                case R.id.add_new_dev_button:
                    addNewDevDialog();
                    break;
                case R.id.del_dev_button:
                    delDev();
                    break;
                case R.id.umount_button:
                    String volPath = getDeviceSelectedPath();
                    if(mDevices.isInterStoragePath(volPath)){
                        displayToast(mContext.getResources().getString(R.string.umount_internal));
                        break;
                    }
                    new AsyncTask<Void, Void, Void>(){
                        @Override
                        protected Void doInBackground(Void... parmas){
                            mDevices.doUmount(volPath);
                            return null;
                        }
                    }.execute();
                    break;
                default:
                    break;
            }
            menu.dismiss();
        }

        public void setListenedMenu(Dialog menu1) {
            this.menu = menu1;
        }

        private void saveCopyFilePath(String filePath1) {
            /*
             * 用于测试 把每次选中的文件路径添加到一个文件中
             */
            File appDir = mContext.getFilesDir();
            File f = new File(appDir.getAbsolutePath() + "/copy.txt");
            if (!f.exists()) {
                try {
                    f.createNewFile();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            try {
                BufferedWriter oStream = new BufferedWriter(new FileWriter(f, true));
                oStream.append(filePath1);
                oStream.append('\n');
                oStream.flush();
                oStream.close();
                Log.d(TAG, "append \'" + filePath1 + "\' to file " + f.getAbsolutePath());
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    private void selectFileTypeDialog() {
        String sortType = mContext.getResources().getString(R.string.sort_type);
        String alpha = mContext.getResources().getString(R.string.alpha);
        String modifiedTime = mContext.getResources().getString(R.string.modified_time);
        String size = mContext.getResources().getString(R.string.by_size);
        CharSequence[] type = {alpha, modifiedTime, size};
        AlertDialog.Builder builder;
        AlertDialog dialog;
        builder = new AlertDialog.Builder(mContext);
        builder.setTitle(sortType);
        builder.setItems(type, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case 0:
                        sort = MediaProvider.ALPH;
                        break;
                    case 1:
                        sort = MediaProvider.LASTMODIFIED;
                        break;
                    case 2:
                        sort = MediaProvider.SIZE;
                        break;
                    default:
                        break;
                }
                ProgressDialog dg = ProgressDialog.show(mContext, null, mContext.getResources()
                        .getString(R.string.sorting));
                sorting();
                mTable.notifyDataSetChanged();
                if (currentPosition > 0) {
                    list.setSelection(currentPosition - 1);
                }
                dg.dismiss();
            }
        });
        dialog = builder.create();

        dialog.show();
        WindowManager.LayoutParams params = dialog.getWindow().getAttributes();
        // params.gravity = Gravity.CENTER;
        params.width = 400;
        dialog.getWindow().setAttributes(params);

    }

    private void addNewDevDialog() {
        final Dialog dialog = new Dialog(mContext);
        dialog.setContentView(R.layout.add_new_dev);
        TextView title = (TextView) dialog.findViewById(android.R.id.title);
        title.setTextSize(32);
        dialog.setTitle(mContext.getResources().getString(R.string.add_new_dev));
        ImageView addDevIcon = (ImageView) dialog.findViewById(R.id.input_icon);
        addDevIcon.setImageResource(R.drawable.mkdir_selected);
        final EditText addInput = (EditText) dialog.findViewById(R.id.add_dev_inputText);
        // This radio group current contains NFS/SMB
        final RadioGroup netShareGroup = (RadioGroup) dialog.findViewById(R.id.netShareGroup);
        Button devCancel = (Button) dialog.findViewById(R.id.add_dev_cancel_b);
        Button devCreate = (Button) dialog.findViewById(R.id.add_dev_create_b);
        devCreate.setText(mContext.getResources().getString(R.string.add_new_dev));

        devCreate.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (netShareGroup.getCheckedRadioButtonId() == R.id.netShareNFS) {
                    Log.d(TAG, "What is NFS?");
                }
                if (addInput.getText().length() < 1) {
                    dialog.dismiss();
                }
                String name = addInput.getText().toString();
                ArrayList<String> netlist = mDevices.getNetDeviceList();
                if (LocalNetwork.getIpType(name) != LocalNetwork.IP_TYPE_UNKNOWN
                        && (netlist == null || !netlist.contains(name))) {
                    displayToast(name + " "
                            + mContext.getResources().getString(R.string.add_new_dev)
                            + mContext.getResources().getString(R.string.success));
                    // Add dev is NFS
                    if (netShareGroup.getCheckedRadioButtonId() == R.id.netShareNFS) {
                        if (V_BUG) {
                            Log.d(TAG, "Add Dev type: NFS");
                        }
                        name = "nfs|" + name; // nfs:192.168.1.105
                    } else if (netShareGroup.getCheckedRadioButtonId() == R.id.netShareSMB) {
                        if (V_BUG) {
                            Log.d(TAG, "Add Dev type: SMB");
                        }
                        name = "smb|" + name; // smb:192.168.1.105
                        // save the net dev
                        // mSamba.addInCache(SmbFile.TYPE_SERVER, name);
                    }
                    mDataSource.add(name);
                    mTable.notifyDataSetChanged();
                    mDevices.saveNetDevice(name);
                } else {
                    displayToast(mContext.getResources().getString(R.string.add_new_dev)
                            + mContext.getResources().getString(R.string.fail));
                }
                dialog.dismiss();
            }
        });
        devCancel.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        dialog.show();
    }

    private void delDev() {
        String path1 = mDataSource.get(currentPosition);
        mDevices.delNetDevice(path1);
        mDataSource.remove(path1);
        mTable.notifyDataSetChanged();
    }

    private void rename(final String path1) {
        /*
         * 以下情况操作无效，直接返回 1.选中的为“返回”， 2.选中的为某设备路径
         */
        if (path1.equals(MediaProvider.RETURN) || mDevices.isLocalDevicesRootPath(path1)) {
            return;
        }

        /* 文件权限不足时，也不能改名 */
        File f = new File(path1);
        if (!(f.canWrite() || (isInSambaMountDir() && SmbFileOperate.canWrite(f)))) {
            displayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
            return;
        }
        mMedia.cleanThumbnailData(path1);
        final String name = path1.substring(path1.lastIndexOf("/") + 1);
        final Dialog dialog = new Dialog(mContext);
        dialog.setContentView(R.layout.input_layout);
        TextView title = (TextView) dialog.findViewById(android.R.id.title);
        title.setTextSize(32);
        dialog.setTitle(mContext.getResources().getString(R.string.rename));
        ImageView renameIcon = (ImageView) dialog.findViewById(R.id.input_icon);
        renameIcon.setImageResource(R.drawable.rename_selected);
        final EditText renameInput = (EditText) dialog.findViewById(R.id.input_inputText);
        Button renameCancel = (Button) dialog.findViewById(R.id.input_cancel_b);
        Button renameCreate = (Button) dialog.findViewById(R.id.input_create_b);
        renameCreate.setText(mContext.getResources().getString(R.string.rename));
        renameCreate.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (renameInput.getText().length() < 1) {
                    dialog.dismiss();
                }
                FileOperate operate;
                if (isInSambaMountDir()) {
                    operate = new SmbFileOperate(mContext);
                } else {
                    operate = new FileOperate(mContext);
                }
                try{
                    int ret = operate.renameTarget(path1, renameInput.getText().toString());
                    switch (ret) {
                        case -1:
                            displayToast(mContext.getResources().getString(
                                    R.string.renamed_to_exist_file));
                            break;
                        case 0:
                            String ext = "";
                            try {
                                ext = path1.substring(path1.lastIndexOf("/"));
                                ext = ext.substring(ext.lastIndexOf("."));
                            } catch (IndexOutOfBoundsException e) {
                                e.printStackTrace();
                                ext = "";
                            }
                            String dirPath = path1.substring(0, path1.lastIndexOf("/"));
                            String newPath = dirPath + "/" + renameInput.getText().toString() + ext;
                            Log.d(TAG, newPath);

                            displayToast(name + mContext.getResources().getString(R.string.rename_to)
                                    + renameInput.getText().toString() + ext);

                            mDataSource.set(currentPosition, newPath);
                            // sorting();
                            mTable.notifyDataSetChanged();
                            break;
                        case -2:
                        default:
                            displayToast(mContext.getResources().getString(R.string.rename_fail));
                            break;
                    }
                }catch(SecurityException e){
                }

                dialog.dismiss();
            }
        });
        renameCancel.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        dialog.show();
    }

    private void mkdir(final String path1) {
        /* 该目录文件权限不足时无法新建文件夹 */
        File file = new File(path1);
        if (!(file.canWrite() || (isInSambaMountDir() && SmbFileOperate.canWrite(file)))) {
            displayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
            return;
        }
        final Dialog dialog = new Dialog(mContext);
        dialog.setContentView(R.layout.input_layout);
        TextView title = (TextView) dialog.findViewById(android.R.id.title);
        title.setTextSize(32);
        dialog.setTitle(mContext.getResources().getString(R.string.mkdir));
        ImageView mkdirIcon = (ImageView) dialog.findViewById(R.id.input_icon);
        mkdirIcon.setImageResource(R.drawable.mkdir_selected);
        final EditText mkdirInput = (EditText) dialog.findViewById(R.id.input_inputText);
        Button mkdirCancel = (Button) dialog.findViewById(R.id.input_cancel_b);
        Button mkdirCreate = (Button) dialog.findViewById(R.id.input_create_b);
        mkdirCreate.setText(mContext.getResources().getString(R.string.mkdir));
        mkdirCreate.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (mkdirInput.getText().length() < 1) {
                    dialog.dismiss();
                }
                FileOperate operate;
                if (isInSambaMountDir()) {
                    operate = new SmbFileOperate(mContext);
                } else {
                    operate = new FileOperate(mContext);
                }
                String name = mkdirInput.getText().toString();
                if (operate.mkdirTarget(path1, name)) {
                    displayToast(name + " " + mContext.getResources().getString(R.string.create)
                            + mContext.getResources().getString(R.string.success));
                    String newPath = path1 + "/" + name;
                    Log.d(TAG, newPath);
                    try {
                        mDataSource.add(currentPosition + 1, newPath);
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage());
                        mDataSource.add(newPath);
                    }

                    // sorting();
                    mTable.notifyDataSetChanged();
                } else {
                    displayToast(mContext.getResources().getString(R.string.create)
                            + mContext.getResources().getString(R.string.fail));
                }
                dialog.dismiss();
            }
        });
        mkdirCancel.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        dialog.show();
    }

    private void sorting() {
        Object[] t = mDataSource.toArray();
        Arrays.sort(t, sort);
        mDataSource.clear();
        for (Object a : t) {
            mDataSource.add((String) a);
        }
    }

    private final class BackgroundWork extends AsyncTask<String, Integer, ArrayList<String>> {
        private ProgressDialog prDialog;
        private int type;
        private int copyRtn = 0;
        private int deleteRtn;
        private long totalSize = 0;
        private long totalNum = 0;
        private int persent = 0;
        private Timer time;
        private TimerTask task;
        private String extraFlag;
        private String copyFile;
        private String newFile;
        private String deletedFile;
        private FileOperate fp;

        private static final int DELETE_OLD_FILES = -1;
        /* 延时弹出扫描对话框 */
        private static final int DELAY_SCAN = -2;
        private boolean isScanning = true;

        private BackgroundWork(int type1) {
            this.type = type1;
            if (isInSambaMountDir()) {
                fp = new SmbFileOperate(mContext);
            } else {
                fp = new FileOperate(mContext);
            }
            if (copyFromSamba) {
                fp = new SmbFileOperate(mContext);
                copyFromSamba = false;
            }
        }

        /**
         * This is done on the EDT thread. this is called before doInBackground
         * is called
         */
        @Override
        protected void onPreExecute() {

            switch (type) {
                case SEARCH_TYPE:
                    showProgressDialog(R.drawable.icon,
                            mContext.getResources().getString(R.string.hello), null,
                            ProgressDialog.STYLE_SPINNER, true);
                    break;
                case COPY_TYPE:
                    String title;
                    title = mContext.getResources().getString(R.string.copy_to_newDir);
                    showProgressDialog(R.drawable.icon, title, null,
                            ProgressDialog.STYLE_HORIZONTAL, true);
                    break;
                case DELETE_TYPE:
                    showProgressDialog(R.drawable.icon,
                            mContext.getResources().getString(R.string.delete_files), null,
                            ProgressDialog.STYLE_HORIZONTAL, true);
                    break;
                case SCANFILES:
                    // showProgressDialog(R.drawable.icon,
                    // mContext.getResources().getString(R.string.scan), null,
                    // ProgressDialog.STYLE_SPINNER, false);
                    break;
                default:
                    break;
            }
        }

        private void showProgressDialog(int icon, String title, String message, int style,
                final boolean cancelable) {
            prDialog = new ProgressDialog(mContext);
            prDialog.setProgressStyle(style);
            prDialog.setIcon(icon);
            prDialog.setTitle(title);
            prDialog.setIndeterminate(false);
            prDialog.setCancelable(cancelable);
            prDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
                @Override
                public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                    if (event.getAction() == KeyEvent.ACTION_DOWN) {
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            fp.setCancel();
                        }
                    }
                    return false;
                }
            });
            if (message != null) {
                prDialog.setMessage(message);
            }
            prDialog.show();
            prDialog.getWindow().setLayout(800, 480);
        }

        /**
         * background thread here
         */
        @Override
        protected ArrayList<String> doInBackground(String... params) {
            switch (type) {
                case SEARCH_TYPE:
                    return null;

                case COPY_TYPE:
                    int prePri = Thread.currentThread().getPriority();
                    copyFile = params[0];
                    newFile = params[1] + copyFile.substring(copyFile.lastIndexOf("/"));
                    /* 如果在同个盘上剪切,相当于重命名 */
                    if (deleteAfterCopy) {
                        File old = new File(copyFile);
                        File f = new File(newFile);
                        if (old.renameTo(f)) {
                            mMedia.cleanThumbnailData(copyFile);
                            try {
                                Runtime runtime = Runtime.getRuntime();
                                runtime.exec("sync");
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                            RefreshMedia mRefresh = new RefreshMedia(mContext);
                            mRefresh.notifyMediaAdd(f.getAbsolutePath());
                            mRefresh.notifyMediaDelete(old.getAbsolutePath());
                            return null;
                        }
                    }
                    // delete new file thumbnail data before copy. avoid original data not update
                    mMedia.cleanThumbnailData(newFile);

                    fp.scanFiles(params[0]);
                    totalSize = fp.getScanSize();
                    totalNum = fp.getScanNum();

                    /* 判断磁盘空间是否足够 */
                    long available = getAvailableSize(params[1]);
                    if (available < totalSize) {
                        copyRtn = -2;
                        return null;
                    }

                    Thread.currentThread().setPriority(5);
                    time = new Timer();
                    task = new TimerTask() {
                        @Override
                        public void run() {
                            int t;
                            if (totalSize > 0) {
                                t = (int) (fp.getCopySize() * 100 / totalSize);
                            } else {
                                /* 当复制的都是文件夹时,总大小为0，这时要按复制的文件数量算 */
                                t = (int) (fp.getCopyNum() * 100 / totalNum);
                            }
                            if (t != persent) {
                                persent = t;
                            }
                            if (persent > 99) {
                                if (fp.isCopying()) {
                                    persent = 99;
                                } else {
                                    persent = 100;
                                }
                            }
                            publishProgress(Integer.valueOf(persent));
                        }
                    };
                    time.schedule(task, 0, 100);
                    copyRtn = fp.copyToDirectory(params[0], params[1]);

                    if (deleteAfterCopy) {
                        mMedia.cleanThumbnailData(copyFile);
                        publishProgress(Integer.valueOf(DELETE_OLD_FILES));
                        fp.deleteTarget(params[0]);
                    }

                    Thread.currentThread().setPriority(prePri);
                    return null;
                case DELETE_TYPE:
                    deletedFile = params[0];
                    fp.scanFiles(deletedFile);
                    totalNum = fp.getScanNum();
                    time = new Timer();
                    task = new TimerTask() {
                        @Override
                        public void run() {
                            int t = (int) (fp.getDeletedNum() * 100 / totalNum);
                            if (t != persent) {
                                persent = t;
                            }
                            if (persent > 100) {
                                persent = 100;
                            }
                            publishProgress(Integer.valueOf(persent));
                        }
                    };
                    time.schedule(task, 0, 100);

                    int size = params.length;
                    for (int i = 0; i < size; i++) {
                        deleteRtn = fp.deleteTarget(params[i]);
                    }
                    return null;
                case SCANFILES:
                    extraFlag = params[1];
                    time = new Timer();
                    task = new TimerTask() {
                        @Override
                        public void run() {
                            publishProgress(Integer.valueOf(DELAY_SCAN));
                        }
                    };
                    time.schedule(task, 200);
                    return mMedia.getList(params[0], mFilter);
                default:
                    break;
            }
            return null;
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            int i = values[0].intValue();
            if (i >= 0) {
                prDialog.setProgress(values[0].intValue());
            } else {
                switch (i) {
                    case DELETE_OLD_FILES:
                        prDialog.setTitle(mContext.getResources().getString(
                                R.string.delete_old_files));
                        break;
                    case DELAY_SCAN:
                        if (isScanning) {
                            showProgressDialog(R.drawable.icon,
                                    mContext.getResources().getString(R.string.scan), null,
                                    ProgressDialog.STYLE_SPINNER, false);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        /**
         * This is called when the background thread is finished. Like
         * onPreExecute, anything here will be done on the EDT thread.
         */
        @Override
        protected void onPostExecute(final ArrayList<String> file) {
            switch (type) {
                case SEARCH_TYPE:
                    prDialog.dismiss();
                    break;
                case COPY_TYPE:

                    if (copyRtn == -1) {
                        displayToast(mContext.getResources().getString(R.string.copy_fail));
                        prDialog.dismiss();
                        return;
                    }

                    if (copyRtn == -2) {
                        displayToast(mContext.getResources().getString(R.string.copy_fail) + ","
                                + mContext.getResources().getString(R.string.not_enough_space));
                        prDialog.dismiss();
                        return;
                    }

                    /* 如果只是覆盖当前目录下的文件，则不需要更新文件列表 */
                    if (!mDataSource.contains(newFile)) {
                        mDataSource.add(newFile);
                        sorting();
                        mTable.notifyDataSetChanged();
                    }
                    if (time != null) {
                        time.cancel();
                    }
                    prDialog.dismiss();
                    break;
                case DELETE_TYPE:
                    if (deleteRtn != 0) {
                        displayToast(mContext.getResources().getString(R.string.delete_fail));
                        prDialog.dismiss();
                        return;
                    }
                    mDataSource.remove(currentPosition);
                    // after remove current item, video will display the video/image of new item thumb,
                    // and video will change the detail information to new item
                    setItemSelected(currentPosition);
                    getDetailForPosition(currentPosition);
                    mTable.notifyDataSetChanged();
                    if (time != null) {
                        time.cancel();
                    }
                    prDialog.dismiss();
                    break;

                case SCANFILES:
                    isScanning = false;
                    if (time != null) {
                        time.cancel();
                    }
                    mDataSource.clear();
                    mDataSource.addAll(file);

                    sorting();
                    if (prDialog != null) {
                        prDialog.dismiss();
                    }
                    mTable.notifyDataSetChanged();
                    if (extraFlag.equals(BACK_TO_PRE)) {
                        int i = mPathStack.pop().intValue();
                        if (currentPosition == i) {
                            getDetailForPosition(i);
                        } else {
                            list.setSelection(i);
                        }
                    } else if (extraFlag.equals(GO_INTO_DIR)) {
                        mPathStack.push(Integer.valueOf(currentPosition));
                        if (currentPosition == 0) {
                            /* 当前位置不是第一个，进入目录时焦点不变（第0个保持亮高），只需要更新预览信息的内容 */
                            getDetailForPosition(currentPosition);
                        } else {
                            /* 这时焦点会改变，需要触发onItemSelected()，使第0个亮高并且更新预览信息内容 */
                            list.setSelection(0);
                        }
                    } else {
                        list.setSelection(0);
                    }
                    scaningFile = false;
                    if (scaningBack) {
                        scaningBack = false;
                        backToPreDir();
                        return;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    private static class Config {
        public static boolean isSupportMouse() {
            return true;
        }
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
          if(SystemClock.elapsedRealtime() - mLastClickTime < 1000){
                    mLastClickTime = SystemClock.elapsedRealtime();
                    return true;
        }
        onItemSelected(arg0, arg1, arg2, arg3);
        ((Activity) mContext).openOptionsMenu();
        return true;
    }
}
