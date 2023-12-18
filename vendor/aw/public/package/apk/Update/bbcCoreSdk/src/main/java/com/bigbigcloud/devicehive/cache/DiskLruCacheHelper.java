package com.bigbigcloud.devicehive.cache;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Environment;
import android.util.Log;

import com.bigbigcloud.devicehive.utils.Utils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Serializable;

/**
 * Created by zhy on 15/7/28.
 */
public class DiskLruCacheHelper
{
    private static final String DIR_NAME = "diskCache";
    private static final int MAX_SIZE = 10 * 1024 * 1024;
    private static final int MAX_FILECOUNT = 100;
    private static final int DEFAULT_APP_VERSION = 1;

    /** {@value */
    public static final int DEFAULT_BUFFER_SIZE = 32 * 1024; // 32 Kb
    /** {@value */
    public static final Bitmap.CompressFormat DEFAULT_COMPRESS_FORMAT = Bitmap.CompressFormat.PNG;
    /** {@value */
    public static final int DEFAULT_COMPRESS_QUALITY = 100;

    /**
     * The default valueCount when open DiskLruCache.
     */
    private static final int DEFAULT_VALUE_COUNT = 1;

    private static final String TAG = "DiskLruCacheHelper";

    private DiskLruCache mDiskLruCache;

    public DiskLruCacheHelper(Context context) throws IOException
    {
        mDiskLruCache = generateCache(context, DIR_NAME, MAX_SIZE, MAX_FILECOUNT);
    }

    public DiskLruCacheHelper(Context context, String dirName) throws IOException
    {
        mDiskLruCache = generateCache(context, dirName, MAX_SIZE, MAX_FILECOUNT);
    }

    public DiskLruCacheHelper(Context context, String dirName, int maxSize, int maxFilecount) throws IOException
    {
        mDiskLruCache = generateCache(context, dirName, maxSize, maxFilecount);
    }

    //custom cache dir
    public DiskLruCacheHelper(File dir) throws IOException
    {
        mDiskLruCache = generateCache(null, dir, MAX_SIZE, MAX_FILECOUNT);
    }

    public DiskLruCacheHelper(Context context, File dir) throws IOException
    {
        mDiskLruCache = generateCache(context, dir, MAX_SIZE, MAX_FILECOUNT);
    }

    public DiskLruCacheHelper(Context context, File dir, int maxSize, int maxFilecount) throws IOException
    {
        mDiskLruCache = generateCache(context, dir, maxSize, maxFilecount);
    }

    private DiskLruCache generateCache(Context context, File dir, int maxSize, int maxFilecount) throws IOException
    {
        if (!dir.exists() || !dir.isDirectory())
        {
            throw new IllegalArgumentException(
                    dir + " is not a directory or does not exists. ");
        }

        int appVersion = context == null ? DEFAULT_APP_VERSION : Utils.getAppVersion(context);

        DiskLruCache diskLruCache = DiskLruCache.open(
                dir,
                appVersion,
                DEFAULT_VALUE_COUNT,
                maxSize, maxFilecount);

        return diskLruCache;
    }

    private DiskLruCache generateCache(Context context, String dirName, int maxSize, int maxFilecount) throws IOException
    {
        DiskLruCache diskLruCache = DiskLruCache.open(
                getDiskCacheDir(context, dirName),
                Utils.getAppVersion(context),
                DEFAULT_VALUE_COUNT,
                maxSize, maxFilecount);
        return diskLruCache;
    }


    public void put(String key, String value)
    {
        DiskLruCache.Editor edit = null;
        BufferedWriter bw = null;
        try
        {
            edit = editor(key);
            if (edit == null) return;
            OutputStream os = edit.newOutputStream(0);
            bw = new BufferedWriter(new OutputStreamWriter(os));
            bw.write(value);
            edit.commit();//write CLEAN
        } catch (IOException e)
        {
            e.printStackTrace();
            try
            {
                //s
                edit.abort();//write REMOVE
            } catch (IOException e1)
            {
                e1.printStackTrace();
            }
        } finally
        {
            try
            {
                if (bw != null)
                    bw.close();
            } catch (IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    public String getAsString(String key)
    {
        InputStream inputStream = null;
        try
        {
            //write READ
            inputStream = get(key);
            if (inputStream == null) return null;
            StringBuilder sb = new StringBuilder();
            int len = 0;
            byte[] buf = new byte[128];
            while ((len = inputStream.read(buf)) != -1)
            {
                sb.append(new String(buf, 0, len));
            }
            return sb.toString();


        } catch (IOException e)
        {
            e.printStackTrace();
            if (inputStream != null)
                try
                {
                    inputStream.close();
                } catch (IOException e1)
                {
                    e1.printStackTrace();
                }
        }
        return null;
    }


    public void put(String key, JSONObject jsonObject)
    {
        put(key, jsonObject.toString());
    }

    public JSONObject getAsJson(String key)
    {
        String val = getAsString(key);
        try
        {
            if (val != null)
                return new JSONObject(val);
        } catch (JSONException e)
        {
            e.printStackTrace();
        }
        return null;
    }


    public void put(String key, JSONArray jsonArray)
    {
        put(key, jsonArray.toString());
    }

    public JSONArray getAsJSONArray(String key)
    {
        String JSONString = getAsString(key);
        try
        {
            JSONArray obj = new JSONArray(JSONString);
            return obj;
        } catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
    }


    public void put(String key, byte[] value)
    {
        OutputStream out = null;
        DiskLruCache.Editor editor = null;
        try
        {
            editor = editor(key);
            if (editor == null)
            {
                return;
            }
            out = editor.newOutputStream(0);
            out.write(value);
            out.flush();
            editor.commit();//write CLEAN
        } catch (Exception e)
        {
            e.printStackTrace();
            try
            {
                editor.abort();//write REMOVE
            } catch (IOException e1)
            {
                e1.printStackTrace();
            }

        } finally
        {
            if (out != null)
            {
                try
                {
                    out.close();
                } catch (IOException e)
                {
                    e.printStackTrace();
                }
            }
        }
    }


    public byte[] getAsBytes(String key)
    {
        byte[] res = null;
        InputStream is = get(key);
        if (is == null) return null;
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try
        {
            byte[] buf = new byte[256];
            int len = 0;
            while ((len = is.read(buf)) != -1)
            {
                baos.write(buf, 0, len);
            }
            res = baos.toByteArray();
        } catch (IOException e)
        {
            e.printStackTrace();
        }
        return res;
    }

    public void put(String key, Serializable value)
    {
        DiskLruCache.Editor editor = editor(key);
        ObjectOutputStream oos = null;
        if (editor == null) return;
        try
        {
            OutputStream os = editor.newOutputStream(0);
            oos = new ObjectOutputStream(os);
            oos.writeObject(value);
            oos.flush();
            editor.commit();
        } catch (IOException e)
        {
            e.printStackTrace();
            try
            {
                editor.abort();
            } catch (IOException e1)
            {
                e1.printStackTrace();
            }
        } finally
        {
            try
            {
                if (oos != null)
                    oos.close();
            } catch (IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    public <T> T getAsSerializable(String key)
    {
        T t = null;
        InputStream is = get(key);
        ObjectInputStream ois = null;
        if (is == null) return null;
        try
        {
            ois = new ObjectInputStream(is);
            t = (T) ois.readObject();
        } catch (ClassNotFoundException e)
        {
            e.printStackTrace();
        } catch (IOException e)
        {
            e.printStackTrace();
        } finally
        {
            try
            {
                if (ois != null)
                    ois.close();
            } catch (IOException e)
            {
                e.printStackTrace();
            }
        }
        return t;
    }

    public void put(String key, Bitmap bitmap)
    {
        put(key, Utils.bitmap2Bytes(bitmap));
    }

    public Bitmap getAsBitmap(String key)
    {
        byte[] bytes = getAsBytes(key);
        if (bytes == null) return null;
        return Utils.bytes2Bitmap(bytes);
    }


    public void put(String key, Drawable value)
    {
        put(key, Utils.drawable2Bitmap(value));
    }

    public Drawable getAsDrawable(String key)
    {
        byte[] bytes = getAsBytes(key);
        if (bytes == null)
        {
            return null;
        }
        return Utils.bitmap2Drawable(Utils.bytes2Bitmap(bytes));
    }

    public boolean remove(String key)
    {
        try
        {
            key = Utils.hashKeyForDisk(key);
            return mDiskLruCache.remove(key);
        } catch (IOException e)
        {
            e.printStackTrace();
        }
        return false;
    }

    public void close() throws IOException
    {
        mDiskLruCache.close();
    }

    public void delete() throws IOException
    {
        mDiskLruCache.delete();
    }

    public void flush() throws IOException
    {
        mDiskLruCache.flush();
    }

    public boolean isClosed()
    {
        return mDiskLruCache.isClosed();
    }

    public long size()
    {
        return mDiskLruCache.size();
    }

    public void setMaxSize(long maxSize)
    {
        mDiskLruCache.setMaxSize(maxSize);
    }

    public File getDirectory()
    {
        return mDiskLruCache.getDirectory();
    }

    public long getMaxSize()
    {
        return mDiskLruCache.getMaxSize();
    }


    public DiskLruCache.Editor editor(String key)
    {
        try
        {
            key = Utils.hashKeyForDisk(key);
            //wirte DIRTY
            DiskLruCache.Editor edit = mDiskLruCache.edit(key);
            //edit maybe null :the entry is editing
            if (edit == null)
            {
                Log.w(TAG, "the entry spcified key:" + key + " is editing by other . ");
            }
            return edit;
        } catch (IOException e)
        {
            e.printStackTrace();
        }

        return null;
    }


    //basic get
    public InputStream get(String key)
    {
        try
        {
            DiskLruCache.Snapshot snapshot = mDiskLruCache.get(Utils.hashKeyForDisk(key));
            if (snapshot == null) //not find entry , or entry.readable = false
            {
                Log.e(TAG, "not find entry , or entry.readable = false");
                return null;
            }
            //write READ
            return snapshot.getInputStream(0);

        } catch (IOException e)
        {
            e.printStackTrace();
            return null;
        }

    }


    private File getDiskCacheDir(Context context, String uniqueName)
    {
        String cachePath;
        if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())
                || !Environment.isExternalStorageRemovable())
        {
            cachePath = context.getExternalCacheDir().getPath();
        } else
        {
            cachePath = context.getCacheDir().getPath();
        }
        Log.d(TAG, " cache path " + cachePath);
        return new File(cachePath + File.separator + uniqueName);
    }

    public boolean save(String key, InputStream imageStream, IoUtils.CopyListener listener) throws IOException {
        key = Utils.hashKeyForDisk(key);
        DiskLruCache.Editor editor = mDiskLruCache.edit(key);
        if (editor == null) {
            return false;
        }

        OutputStream os = new BufferedOutputStream(editor.newOutputStream(0), DEFAULT_BUFFER_SIZE);
        boolean copied = false;
        try {
            copied = IoUtils.copyStream(imageStream, os, listener, DEFAULT_BUFFER_SIZE);
        } finally {
            IoUtils.closeSilently(os);
            if (copied) {
                editor.commit();
            } else {
                editor.abort();
            }
        }
        return copied;
    }

    public File getFile(String key) {
        DiskLruCache.Snapshot snapshot = null;
        key = Utils.hashKeyForDisk(key);
        try {
            snapshot = mDiskLruCache.get(key);
            return snapshot == null ? null : snapshot.getFile(0);
        } catch (IOException e) {
            return null;
        } finally {
            if (snapshot != null) {
                snapshot.close();
            }
        }
    }
    public boolean save(String key, Bitmap bitmap) throws IOException {
        key = Utils.hashKeyForDisk(key);
        DiskLruCache.Editor editor = mDiskLruCache.edit(key);
        if (editor == null) {
            return false;
        }

        OutputStream os = new BufferedOutputStream(editor.newOutputStream(0), DEFAULT_BUFFER_SIZE);
        boolean savedSuccessfully = false;
        try {
            savedSuccessfully = bitmap.compress(DEFAULT_COMPRESS_FORMAT, DEFAULT_COMPRESS_QUALITY, os);
        } finally {
            IoUtils.closeSilently(os);
        }
        if (savedSuccessfully) {
            editor.commit();
        } else {
            editor.abort();
        }
        return savedSuccessfully;
    }

}



