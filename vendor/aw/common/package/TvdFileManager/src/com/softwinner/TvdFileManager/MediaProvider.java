package com.softwinner.TvdFileManager;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.StatFs;
import android.util.Log;

import java.io.File;
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Comparator;

/**
 *
 * @author chenjd
 * @help <br>
 *       1.用于缩略图的管理 <br>
 *       getImageThumbFromDB(String path) 从数据库中获取缩略图,返回null表示数据库中没保存该图片的缩略图数据 <br>
 *       getImageThumbFromMK(String path) 直接生成缩略图，建议先调用第一个从数据库中取，如果没找到再调用这个方法 <br>
 *       clearThumbnailData()清除数据库中保存的缩略图数据，调用这方法后，
 *       数据库中将只保留最新的MaxThumbnailNum个缩略图数据 <br>
 *       2.用于文件夹目录下的文件列表生成 <br>
 *       定义了几种的文件过滤类 <br>
 *       getList(String, FileFilter) 根据文件的过滤类型滤出文件夹内的文件列表
 */
public class MediaProvider {
    public static final String RETURN = "../";
    public static final String NETWORK_NEIGHBORHOOD = "NetworkNeighborhoodList";
    public static final String NFS_SHARE = "NFSServerList";
    public static final int AUDIOTYPE = 0x01;
    public static final int IMAGETYPE = 0x02;
    public static final int VIDEOTYPE = 0x04;
    public static final int ALLTYPE = 0x08;
    public static final int DIRTYPE = 0x00;

    private static final int MAXTHUMBNAILNUM = 200;
    private int filesNum = 0;
    private Context context;
    private ImageDatabase imageDB;
    private static final String TAG = "MediaProvider";
    private ArrayList<String> mlist;

    private ThumbnailCreator thumbCreator = null;

    public MediaProvider(Context context1) {
        this.context = context1;
        this.imageDB = new ImageDatabase(context1);
        mlist = new ArrayList<String>();
    }

    /* 定义排序方式 */
    public static final Comparator ALPH = new Comparator<String>() {
        @Override
        public int compare(String arg0, String arg1) {
            File f0 = new File(arg0);
            File f1 = new File(arg1);

            /* "返回"指示永远排在第一个 */
            if (arg0.equals(RETURN)) {
                return -1;
            }
            if (arg1.equals(RETURN)) {
                return 1;
            }

            /* 文件夹永远排在前面 */
            try {
                if (f0.isDirectory() && !f1.isDirectory()) {
                    return -1;
                }
                if (!f0.isDirectory() && f1.isDirectory()) {
                    return 1;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return -1;
            }

            try {
                /* 否则根据名字排序 */
                String str0 = arg0.substring(arg0.lastIndexOf("/") + 1);
                String str1 = arg1.substring(arg1.lastIndexOf("/") + 1);
                return str0.compareToIgnoreCase(str1);
            } catch (IndexOutOfBoundsException e) {
                e.printStackTrace();
                return -1;
            }
        }
    };

    public static final Comparator LASTMODIFIED = new Comparator<String>() {
        @Override
        public int compare(String arg0, String arg1) {
            File f0 = new File(arg0);
            File f1 = new File(arg1);

            /* "返回"指示永远排在第一个 */
            if (arg0.equals(RETURN)) {
                return -1;
            }
            if (arg1.equals(RETURN)) {
                return 1;
            }

            try {
                /* 文件夹永远排在前面 */
                if (f0.isDirectory() && !f1.isDirectory()) {
                    return -1;
                }
                if (!f0.isDirectory() && f1.isDirectory()) {
                    return 1;
                }

                /* 否则根据名字排序 */
                if (f0.lastModified() > f1.lastModified()) {
                    return -1;
                } else if (f0.lastModified() == f1.lastModified()) {
                    return 0;
                } else {
                    return 1;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return -1;
            }
        }
    };

    public static final Comparator SIZE = new Comparator<String>() {
        @Override
        public int compare(String arg0, String arg1) {
            File f0 = new File(arg0);
            File f1 = new File(arg1);
            if (arg0.equals(RETURN)) {
                return -1;
            }
            if (arg1.equals(RETURN)) {
                return 1;
            }
            try {
                if (f0.isDirectory() && !f1.isDirectory()) {
                    return -1;
                }
                if (!f0.isDirectory() && f1.isDirectory()) {
                    return 1;
                }
                if (f0.length() > f1.length()) {
                    return -1;
                } else if (f0.length() == f1.length()) {
                    return 0;
                } else {
                    return 1;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return -1;
            }
        }
    };

    /* 定义文件过滤器 */
    public FileFilter musicFilter = new FileFilter() {

        @Override
        public boolean accept(File pathname) {
            // TODO Auto-generated method stub
            // keep all needed files
            try {
                if (pathname.isDirectory()) {
                    filesNum++;
                    return true;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return false;
            }

            String name = pathname.getAbsolutePath();

            if (TypeFilter.isMusicFile(name)) {
                filesNum++;
                return true;
            }

            return false;
        }
    };

    public FileFilter movieFilter = new FileFilter() {

        @Override
        public boolean accept(File pathname) {
            // TODO Auto-generated method stub
            // keep all needed files
            try {
                if (pathname.isDirectory()) {
                    filesNum++;
                    return true;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return false;
            }

            String name = pathname.getAbsolutePath();
            if (TypeFilter.isMovieFile(name)) {
                filesNum++;
                return true;
            }

            return false;
        }
    };

    public FileFilter pictureFilter = new FileFilter() {

        @Override
        public boolean accept(File pathname) {
            // TODO Auto-generated method stub
            // keep all needed files
            try {
                if (pathname.isDirectory()) {
                    filesNum++;
                    return true;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return false;
            }

            String name = pathname.getAbsolutePath();
            if (TypeFilter.isPictureFile(name)) {
                filesNum++;
                return true;
            }

            return false;
        }
    };

    public FileFilter alltypeFilter = new FileFilter() {
        @Override
        public boolean accept(File pathname) {
            filesNum++;
            return true;
        }
    };

    public int getFilesNum() {
        return filesNum;
    }

    public ArrayList<String> getList(String path, FileFilter type) {
        filesNum = 0;
        mlist.clear();
        mlist.add(RETURN);
        File file = new File(path);
        File[] fileList = null;
        if (file.canRead()) {
            fileList = file.listFiles(type);
        }
        DeviceManager devMng = new DeviceManager(context);
        int i = 0;
        if (fileList != null) {
            /* 处理多分区的情况 */
            if (devMng.getLocalDevicesList().contains(path) && devMng.hasMultiplePartition(path)) {
                for (i = 0; i < fileList.length; i++) {
                    try {
                        String child = fileList[i].getAbsolutePath();
                        StatFs statFs = new StatFs(child);
                        Log.d("chen", "child:" + child + "  block num:" + statFs.getBlockCount()
                                + "  avail block num:" + statFs.getAvailableBlocks());
                        if (statFs.getBlockCount() != 0) {

                            mlist.add(child);
                        }
                    } catch (Exception e) {
                    }
                }
            } else {
                for (i = 0; i < fileList.length; i++) {
                    scanFile(fileList[i]);
                    mlist.add(fileList[i].getAbsolutePath());
                }
            }
        }
        return (ArrayList<String>) mlist.clone();
    }

    private void scanFile(File file) {
        String path = file.getAbsolutePath();
        if (TypeFilter.isMusicFile(path)) {
            /*
             * notify the media to scan
             */
            Uri mUri = Uri.fromFile(file);
            Intent mIntent = new Intent();
            mIntent.setData(mUri);
            mIntent.setAction(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
            context.sendBroadcast(mIntent);
        }
    }

    /**
     * 从数据库中查询返回缩略图
     *
     * @param origPath
     * @return null如果查询不到该缩略图文件
     */
    public Bitmap getImageThumbFromDB(String origPath) {
        final int origPathColumn = 0;
        final int thumbPathColumn = 1;
        String[] columns = {ImageDatabase.ORIG_PATH, ImageDatabase.THUMB_PATH};
        String selection = ImageDatabase.ORIG_PATH + "=?";
        String[] args = {origPath};
        Bitmap thumbnail = null;

        Cursor c = imageDB.query(columns, selection, args, null);
        // 如果从数据库中找到该原图的缩量图，则直接使用
        if (c != null) {
            try {
                while (c.moveToNext()) {
                    String thumbPath = c.getString(c.getColumnIndex(ImageDatabase.THUMB_PATH));
                    thumbnail = BitmapFactory.decodeFile(thumbPath);
                    break;
                }
            } finally {
                c.close();
                c = null;
            }

        }
        return thumbnail;
    }

    /**
     *
     * @param origPath
     *            源图片文件路径
     * @param width
     *            缩略图设定宽度
     * @param height
     *            缩略图设定高度
     * @return 返回缩略图,失败返回null
     */
    public Bitmap getImageThumbFromMK(String origPath, int width, int height) {

        // 产生缩略图，并把该文件存到指定目录下，更新数据库中图片信息
        Bitmap thumbnail = null;
        Log.d(TAG, origPath + ":make thumbnail and insert message in database");
        ThumbnailCreator mCreator = new ThumbnailCreator(width, height);
        thumbnail = mCreator.createThumbnail(origPath);
        if (thumbnail == null) {
            return null;
        }
        String name = null;
        long nCurTime=0;
        try {
            name = origPath.substring(origPath.lastIndexOf("/") + 1, origPath.lastIndexOf("."));
             nCurTime =  System.currentTimeMillis();
        } catch (IndexOutOfBoundsException e) {
            e.printStackTrace();
            return null;
        }
        try {
        	Log.d(TAG,"===YTR==ll=nCurTime:"+nCurTime);
            File f = new File(imageDB.getAppDir() + "/" + String.valueOf(nCurTime)+"_"+name);//增加一个时间戳，区别名字一样的情况
            FileOutputStream fOut = null;
            fOut = new FileOutputStream(f);
            thumbnail.compress(Bitmap.CompressFormat.JPEG, 100, fOut);
            fOut.flush();
            fOut.close();
            imageDB.insert(origPath, f.getPath());
        } catch (Exception e) {
            // TODO Auto-generated catch block
            Log.d(TAG, "create temp file false");
            e.printStackTrace();
        }
        return thumbnail;
    }

    /**
     * 清除数据库中暂存的缩略图信息及对应的缩略图文件，需要在每次退出程序前清除过时的信息，只保留MaxThumbnailNum个缩略图信息，
     * 防止暂存区不断扩大
     */
    public void clearThumbnailData() {
        final int origPathColumn = 0;
        final int thumbPathColumn = 1;
        final int createTime = 2;

        String[] columns = {ImageDatabase.ORIG_PATH, ImageDatabase.THUMB_PATH,
                ImageDatabase.CREATE_TIME};
        String sort = ImageDatabase.CREATE_TIME + " DESC";
        // 按降序整理查询的结果
        Cursor c = imageDB.query(columns, null, null, sort);
        int i = 0;
        if (c != null) {
            try {
                while (c.moveToNext()) {
                    i++;
                    // 删除第MaxThumbnailNum以后个图片及其信息
                    if (i > MAXTHUMBNAILNUM) {
                        String thumbPath = c.getString(thumbPathColumn);
                        String origPath = c.getString(origPathColumn);
                        File file = new File(thumbPath);
                        try {
                            if (file.delete()) {
                                imageDB.delete(origPath);
                            }
                        } catch (SecurityException e) {
                        }
                    }
                }
            } finally {
                c.close();
                c = null;
            }
        } else {
            Log.d(TAG, "cursor is null");
        }
    }

    /**
     * 清除缩略图文件和数据库记录 防止暂存区不断扩大
     */
    public void cleanThumbnailData(String origPath) {
        final int origPathColumn = 0;
        final int thumbPathColumn = 1;
        String[] columns = {ImageDatabase.ORIG_PATH, ImageDatabase.THUMB_PATH};
        String selection = ImageDatabase.ORIG_PATH + "=?";
        String[] args = {origPath};
        String thumbPath = "";

        Cursor c = imageDB.query(columns, selection, args, null);
        if (c != null) {
            try {
                while (c.moveToNext()) {
                    thumbPath = c.getString(thumbPathColumn);
                    break;
                }
            } finally {
                c.close();
                c = null;
            }
        }
        imageDB.delete(origPath);
        File file = new File(thumbPath);
        if (file.exists()) {
            file.delete();
        }
    }

    public void closeDB() {
        imageDB.closeDatabase();
    }
}
