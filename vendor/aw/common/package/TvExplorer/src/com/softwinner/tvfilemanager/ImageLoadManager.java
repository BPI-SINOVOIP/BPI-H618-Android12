package com.softwinner.tvfilemanager;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.media.MediaMetadataRetriever;
import android.media.ThumbnailUtils;
import android.os.AsyncTask;
import android.support.v17.leanback.widget.ImageCardView;
import android.support.v4.util.LruCache;
import android.util.Log;
import android.widget.ImageView;

import com.softwinner.tvfilemanager.common.Constants;
import com.softwinner.tvfilemanager.common.TypeFilter;
import com.softwinner.tvfilemanager.common.Utils;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.SoftReference;
import java.lang.reflect.Field;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * Created by ningxiangzhong on 15-3-15.
 */
public class ImageLoadManager {

    private BitmapDrawable defaultPictureBitmap;
    private BitmapDrawable defaultVideoBitmap;

    private static ImageLoadManager imageLoadManager;
    private static final Object imageLoadManagerLock = new Object();

    private static final int CORE_POOL_SIZE = 5;

    private static final Executor BITMAP_DOWNLOADER_THREAD_POOL_EXECUTOR =
            Executors.newFixedThreadPool(CORE_POOL_SIZE);

    /**
     * 图片源类型: 文件,网络,资源ID *
     */
    public enum IMAGE_LOAD_TYPE {
        FILE_PATH, FILE_URL, FILE_RESOURCE_ID
    }

    private String TAG = "ImageLoadManager...";

    private Context context;

    private Set<ImageLoadTask> taskCollection;

    private LruCache<String, BitmapDrawable> BitmapMemoryCache;


    /**
     * 最大内存 *
     */
    final int maxCacheSize;

    /**
     * 建立线程安全,支持高并发的容器 *
     */
    private static ConcurrentHashMap<String, SoftReference<Bitmap>> currentHashmap
            = new ConcurrentHashMap<String, SoftReference<Bitmap>>();

    //single
    public final static ImageLoadManager getInstance(Context context) {
        if (imageLoadManager == null) {
            synchronized (imageLoadManagerLock) {
                if (imageLoadManager == null) {
                    imageLoadManager = new ImageLoadManager(context);
                }
            }
        }
        return imageLoadManager;
    }

    public ImageLoadManager(Context context) {

        this.context = context;
        taskCollection = new HashSet<ImageLoadTask>();
        defaultPictureBitmap = new BitmapDrawable(BitmapFactory.decodeResource(context.getResources(),
                R.drawable.thumbnail_picture));
        defaultVideoBitmap = new BitmapDrawable(BitmapFactory.decodeResource(context.getResources(),
                R.drawable.thumbnail_video));

        maxCacheSize = (int) (Runtime.getRuntime().maxMemory() / 8);

        BitmapMemoryCache = new LruCache<String, BitmapDrawable>(maxCacheSize) {
            @Override
            protected int sizeOf(String key, BitmapDrawable value) {
                if (value != null) {
                    Log.e("imageloadManagerDebug" , "size : " + value.getBitmap().getByteCount()/(1024) + "kb");
                    return value.getBitmap().getByteCount();
                    //return value.getRowBytes() * value.getHeight();	//旧版本的方法
                } else {
                    return 0;
                }
            }

        /*//这个方法当LruCache的内存容量满的时候会调用,将oldValue的元素移除出来腾出空间给新的元素加入
        @Override
        protected void entryRemoved(boolean evicted, String key,Bitmap oldValue, Bitmap newValue)
        {
            if(oldValue != null)
            {
                // 当硬引用缓存容量已满时，会使用LRU算法将最近没有被使用的图片转入软引用缓存
                currentHashmap.put(key, new SoftReference<Bitmap>(oldValue));
            }
        }*/

        };
    }

    /**
     * 针对提供图片资源ID来显示图片的方法
     *
     * @param loadType        图片加载类型
     * @param imageResourceID 图片资源id
     * @param imageView       显示图片的ImageView
     */
    public void setImageView(IMAGE_LOAD_TYPE loadType, int imageResourceID, ImageView imageView) {
        if (loadType == IMAGE_LOAD_TYPE.FILE_RESOURCE_ID) {
//			if(ifResourceIdExist(imageResourceID))
//			{
//				imageView.setImageResource(imageResourceID);
//
//			}else{	//映射无法获取该图片,则显示默认图片
//				imageView.setImageResource(R.drawable.pic_default);
//			}
            try {
                imageView.setImageResource(imageResourceID);
                return;
            } catch (Exception e) {
                Log.e(TAG, "Can find the imageID of " + imageResourceID);
                e.printStackTrace();
            }
            //默认图片
            imageView.setImageResource(R.drawable.thumbnail_picture);
        }
    }

    /**
     * 针对提供图片文件链接或下载链接来显示图片的方法
     *
     * @param loadType      图片加载类型
     * @param imageFilePath 图片文件的本地文件地址或网络URL的下载链接
     * @param imageCardView 显示图片的ImageView
     */
    public void setImageView(IMAGE_LOAD_TYPE loadType, String imageFilePath, ImageCardView imageCardView) {
        if (imageFilePath == null || imageFilePath.trim().equals("")) {

            imageCardView.setMainImage(defaultPictureBitmap);

        } else {
            BitmapDrawable bitmap = getBitmapFromMemoryCache(imageFilePath);
            if (bitmap != null) {
                imageCardView.setMainImage(bitmap);
            } else {
                if (TypeFilter.isPictureFile(imageFilePath)) {
                    imageCardView.setMainImage(defaultPictureBitmap);
                } else if (TypeFilter.isMovieFile(imageFilePath)) {
                    imageCardView.setMainImage(defaultVideoBitmap);
                }
                ImageLoadTask task = new ImageLoadTask(loadType, imageCardView);
                //通过一句代码，一个对象的检查，最后这个继续引用task，导致其不能正常释放，task又引用imageCardView
                // ，从而导致Views的对象无限增大，从而导致内存泄露。
//                taskCollection.add(task);
                task.executeOnExecutor(BITMAP_DOWNLOADER_THREAD_POOL_EXECUTOR, imageFilePath);
            }
        }
    }


    /**
     * 从LruCache中获取一张图片，如果不存在就返回null
     *
     * @param key 键值可以是图片文件的filePath,可以是图片URL地址
     * @return Bitmap对象, 或者null
     */
    public BitmapDrawable getBitmapFromMemoryCache(String key) {
        /*try {
            if (BitmapMemoryCache.get(key) == null) {
                if (currentHashmap.get(key) != null) {
                    return currentHashmap.get(key).get();
                }
            }
            return BitmapMemoryCache.get(key);

        } catch (Exception e) {
            e.printStackTrace();
        }*/
        return BitmapMemoryCache.get(key);
    }

    /**
     * 将图片放入缓存
     *
     * @param key
     * @param bitmap
     */
    public void addBitmapToCache(String key, BitmapDrawable bitmap) {
        BitmapMemoryCache.put(key, bitmap);
    }


    /**
     * 图片异步加载
     *
     * @author Mr.Et
     */
    private class ImageLoadTask extends AsyncTask<String, Void, BitmapDrawable> {
        private String imagePath;
        private ImageCardView imageCardView;
        private IMAGE_LOAD_TYPE loadType;

        public ImageLoadTask(IMAGE_LOAD_TYPE loadType, ImageCardView imageCardView) {
            this.loadType = loadType;
            this.imageCardView = imageCardView;
        }


        @Override
        protected BitmapDrawable doInBackground(String... params) {
            imagePath = params[0];

            try {
                if (loadType == IMAGE_LOAD_TYPE.FILE_PATH) {

                    if (TypeFilter.isMovieFile(imagePath)) {

                        MediaMetadataRetriever mediaMetadataRetriever = new MediaMetadataRetriever();

                        try {

                            mediaMetadataRetriever.setDataSource(imagePath);
//                            Bitmap videoThumbnail = ThumbnailUtils.extractThumbnail(mediaMetadataRetriever.getFrameAtTime(),
//                                    Constants.THUMBNAIL_WIDTH, Constants.THUMBNAIL_HEGITH,
//                                    ThumbnailUtils.OPTIONS_RECYCLE_INPUT);

                            //将获取的新图片放入缓存
                            BitmapDrawable drawable = new BitmapDrawable(ThumbnailUtils.extractThumbnail(mediaMetadataRetriever.getFrameAtTime(),
                                    Constants.THUMBNAIL_WIDTH, Constants.THUMBNAIL_HEGITH,
                                    ThumbnailUtils.OPTIONS_RECYCLE_INPUT));
                            addBitmapToCache(imagePath, drawable);
                            return drawable;

                        } catch (Exception e) {
                            e.printStackTrace();
                            return null;
                        } finally {

                            try {
                                if (mediaMetadataRetriever != null) {
                                    mediaMetadataRetriever.release();
                                    mediaMetadataRetriever = null;
                                }
                            } catch (RuntimeException ex) {
                                ex.printStackTrace();
                                return null;
                                // Ignore failures while cleaning up.
                            }
                        }

                    } else if (TypeFilter.isPictureFile(imagePath)) {

                        if (new File(imagePath).exists()) {
//                            Bitmap bitmap = Utils.decodedBitmapFromFile(imagePath,
//                                    Constants.THUMBNAIL_WIDTH, Constants.THUMBNAIL_HEGITH);
                            //将获取的新图片放入缓存
                            BitmapDrawable drawable = new BitmapDrawable(Utils.decodedBitmapFromFile(imagePath,
                                    Constants.THUMBNAIL_WIDTH, Constants.THUMBNAIL_HEGITH));
                            addBitmapToCache(imagePath, drawable);
                            return drawable;
                        }
                        return null;
                    }

                } /*else if (loadType == IMAGE_LOAD_TYPE.FILE_URL) {    //从网络下载图片
                    byte[] datas = getBytesOfBitMap(imagePath);
                    if (datas != null) {
//						BitmapFactory.Options opts = new BitmapFactory.Options();
//						opts.inSampleSize = 2;
//						Bitmap bitmap = BitmapFactory.decodeByteArray(datas, 0, datas.length, opts);
                        Bitmap bitmap = BitmapFactory.decodeByteArray(datas, 0, datas.length);
                        addBitmapToCache(imagePath, bitmap);
                        return bitmap;
                    }
                    return null;
                }*/

            } catch (Exception e) {
                e.printStackTrace();
                //可自定义其他操作
            }
            return null;
        }

        @Override
        protected void onPostExecute(BitmapDrawable bitmap) {
            try {
                if (imageCardView != null) {
                    if (bitmap != null) {
                        imageCardView.setMainImage(bitmap);
                    } else {
                        Log.e(TAG, "The bitmap result is null...");
                        //获取图片失败时显示默认图片
                        if (TypeFilter.isPictureFile(imagePath)) {
                            imageCardView.setMainImage(defaultPictureBitmap);
                        } else if (TypeFilter.isMovieFile(imagePath)) {
                            imageCardView.setMainImage(defaultVideoBitmap);
                        }
                    }
                } else {
                    Log.e(TAG, "The imageView is null...");
                }

            } catch (Exception e) {
                e.printStackTrace();
            }
        }


    }


    /**
     * InputStream转byte[]
     *
     * @param inStream
     * @return
     * @throws Exception
     */
    private byte[] readStream(InputStream inStream) throws Exception {
        ByteArrayOutputStream outStream = new ByteArrayOutputStream();
        byte[] buffer = new byte[2048];
        int len = 0;
        while ((len = inStream.read(buffer)) != -1) {
            outStream.write(buffer, 0, len);
        }
        outStream.close();
        inStream.close();
        return outStream.toByteArray();
    }

    /**
     * 获取下载图片并转为byte[]
     *
     * @param imgUrl
     * @return
     */
    private byte[] getBytesOfBitMap(String imgUrl) {
        try {
            URL url = new URL(imgUrl);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setConnectTimeout(10 * 1000);  //10s
            conn.setReadTimeout(20 * 1000);
            conn.setRequestMethod("GET");
            conn.connect();
            InputStream in = conn.getInputStream();
            return readStream(in);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * 该资源ID是否有效
     *
     * @param resourceId 资源ID
     * @return
     */
    private boolean ifResourceIdExist(int resourceId) {
        try {
            Field field = R.drawable.class.getField(String.valueOf(resourceId));
            Integer.parseInt(field.get(null).toString());
            return true;

        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    /**
     * 取消所有任务
     */
    public void cancelAllTask() {
        if (taskCollection != null) {
            for (ImageLoadTask task : taskCollection) {
                task.cancel(false);
            }
        }
    }

    /**
     * 清除缓存
     */
    public void clearCache() {
        BitmapMemoryCache.evictAll();
    }

    public LruCache<String, BitmapDrawable> getBitmapMemoryCache() {
        return BitmapMemoryCache;
    }

}
