package com.softwinner.ImageParser;

import androidx.annotation.NonNull;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.widget.LinearLayout;
import android.view.KeyEvent;
import android.util.DisplayMetrics;
import android.Manifest;
import android.content.pm.PackageManager;
import android.widget.ImageView;
import android.widget.Toast;
import com.softwinner.GifPlayer;
import android.view.View;
import android.os.Build;

public class MainActivity extends Activity implements View.OnClickListener {

    private final String TAG = "ImageParser-Main";
    private String image_path;
    private Context mContext;
    private Bitmap mBitmap;
    private int mWidth, mHeight;
    private ImageView mIvShow;
    private LinearLayout mLlSfvRoot;
    private SurfaceView mSurfaceview;
    private GifPlayer mGifPlayer;
    private boolean mIsPause = false;
    private String gifPath;
    private int maxWidth;
    private int maxHeight;
    private int minWidth;
    private int minHeight;
    private int widthScaleFactor;
    private int heightScaleFactor;
    private int iv_width;
    private int iv_height;
    private LinearLayout.LayoutParams mLayoutParams;
    private boolean isGif;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mSurfaceview = (SurfaceView) findViewById(R.id.surfaceView);
        mIvShow = (ImageView) findViewById(R.id.iv_show);
        mLlSfvRoot = (LinearLayout) findViewById(R.id.ll_sfv_root);
        mIvShow.setOnClickListener(this);

        mContext = (Context) this;

        Uri uri = getIntent().getData();
        Log.d(TAG,"uri:"+uri);
        image_path = DocUtils.getFilePathByUri(mContext, uri);
        Log.d(TAG,"image_path:"+image_path);

        if (Build.VERSION.SDK_INT < 30) {
            if(checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
                requestPermissions(new String[] {Manifest.permission.READ_EXTERNAL_STORAGE}, 666);
            } else {
                init();
            }
        } else {
            init();
        }
    }

    private void init() {
        isGif = DocUtils.isGifType(image_path);
        initParam();
        if(isGif) {
            mLlSfvRoot.setVisibility(View.GONE);
            mIvShow.setVisibility(View.VISIBLE);
            initGifPlayer(image_path);
        } else {
            mLlSfvRoot.setVisibility(View.VISIBLE);
            mIvShow.setVisibility(View.GONE);
            initSurfaceView(image_path);
        }
    }

    private void initParam() {
        Log.d(TAG,"initParam");
        DisplayMetrics dm=new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        maxWidth = 4 * dm.widthPixels;
        maxHeight = 4 * dm.heightPixels;
        minWidth = (int) (0.2 * dm.widthPixels);
        minHeight = (int) (0.2 * dm.heightPixels);
        widthScaleFactor = (int) (0.1 * dm.widthPixels);
        heightScaleFactor = (int) (0.1 * dm.heightPixels);
        iv_width = dm.widthPixels;
        iv_height = dm.heightPixels;
        mLayoutParams = new LinearLayout.LayoutParams(iv_width, iv_height);
    }

    private Bitmap initBitmap(String filePath) {
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(filePath,opts);
        mHeight = opts.outHeight;
        mWidth = opts.outWidth;
        opts.inJustDecodeBounds = false;
        Bitmap bitmap = BitmapFactory.decodeFile(filePath,opts);
        Log.d(TAG,"pic width = " + mWidth + " height = " + mHeight);
        return bitmap;
    }

    private void initSurfaceView(String filePath) {
        Log.d(TAG, "initSurfaceView file path = " + filePath);
        mBitmap = initBitmap(filePath);

        SurfaceHolder holder = mSurfaceview.getHolder();
        holder.addCallback(
                new Callback() {

                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        Canvas canvas = null;
                        try {
                            canvas = holder.lockCanvas(null);
                            synchronized (holder) {
                                onDraw(canvas);
                            }
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            if (canvas != null) {
                                holder.unlockCanvasAndPost(canvas);
                            }
                        }
                    }

                    @Override
                    public void surfaceChanged(
                            SurfaceHolder holder, int format, int width, int height) {
                        Canvas canvas = null;
                        try {
                            canvas = holder.lockCanvas(null);
                            synchronized (holder) {
                                onDraw(canvas);
                            }
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            if (canvas != null) {
                                holder.unlockCanvasAndPost(canvas);
                            }
                        }
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                    }

                    protected void onDraw(Canvas canvas) {
                        canvas.drawColor(Color.BLACK);
                        canvas.drawBitmap(mBitmap, 0, 0, new Paint());
                    }
                });
        mSurfaceview.getHolder().setFixedSize(mWidth, mHeight);
    }

    private void initGifPlayer(String filePath) {
        Log.d(TAG, "initGifPlayer file path = " + filePath);
        if (mGifPlayer == null) {
            mGifPlayer = new GifPlayer();
        }
        mGifPlayer.play(filePath,true);
        mGifPlayer.setOnDrawGifCallBack(new GifPlayer.OnDrawGifCallBack() {

            @Override
            public void onDrawFrame(final Bitmap bitmap) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mIvShow.setImageBitmap(bitmap);
                    }
                });
            }

            @Override
            public void onError(final String msg) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mContext,msg,Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP:
                Log.d(TAG,"KEYCODE_DPAD_UP");
                iv_width -= widthScaleFactor;
                if (iv_width <= minWidth) {
                    iv_width = minWidth;
                }
                iv_height -= heightScaleFactor;
                if (iv_height <= minHeight) {
                    iv_height = minHeight;
                }
                setLayoutParams(iv_width,iv_height);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                Log.d(TAG,"KEYCODE_DPAD_DOWN");
                iv_width += widthScaleFactor;
                if (iv_width >= maxWidth) {
                    iv_width = maxWidth;
                }
                iv_height += heightScaleFactor;
                if (iv_height >= maxHeight) {
                    iv_height = maxHeight;
                }
                setLayoutParams(iv_width,iv_height);
                break;
        }
        return super.onKeyDown(keyCode, event);
    }

    private void setLayoutParams(int width,int height) {
        if (mLayoutParams != null) {
            mLayoutParams.width = width;
            mLayoutParams.height = height;
            if (isGif) {
                mIvShow.setLayoutParams(new LinearLayout.LayoutParams(width, height));
            } else {
                mLlSfvRoot.setLayoutParams(new LinearLayout.LayoutParams(width, height));
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 666) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED){
                init();
            } else {
                Log.e(TAG,"no read permission,then finish activity!!!");
                finish();
            }
        }
    }

    @Override
    public void onClick(View v) {
        if(v.getId() == R.id.iv_show) {
            if(mGifPlayer != null) {
                if(!mIsPause) {
                    mGifPlayer.pause();
                }else{
                     mGifPlayer.resume();
                }
                mIsPause = !mIsPause;
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mGifPlayer != null) {
            mGifPlayer.release();
        }
    }

}
