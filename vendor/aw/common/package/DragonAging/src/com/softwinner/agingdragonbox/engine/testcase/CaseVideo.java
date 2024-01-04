package com.softwinner.agingdragonbox.engine.testcase;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnInfoListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnSeekCompleteListener;
import android.media.MediaPlayer.OnVideoSizeChangedListener;
import android.net.Uri;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Timer;
import java.util.TimerTask;

import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.ThreadVar;
import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.engine.Utils;
import com.softwinner.agingdragonbox.xml.Node;

/**
 * Video测试
 *
 * @author maizirong
 *
 */
// boolean threadExit=true;
// boolean threadExitDdr=true;
public class CaseVideo extends BaseCase implements OnPreparedListener, OnErrorListener,
        OnCompletionListener, OnBufferingUpdateListener, OnInfoListener, OnSeekCompleteListener,
        OnVideoSizeChangedListener, SurfaceHolder.Callback {
    private static final String TAG             = "DragonAging-CaseVideo";
    public static final String  SAMPLE          = "case_video_sample";

    private File                mFullCache;

    private ProgressBar         mProgressBar;

    private MediaPlayer         mMediaPlayer;

    private SurfaceView         surfaceView;
    private SurfaceHolder       surfaceHolder;

    private Button              mRedo;

    private TextView            mTitle;

    private static final int    STATE_ERROR     = -1;
    private static final int    STATE_IDLE      = 0;
    private static final int    STATE_PREPARING = 1;
    private static final int    STATE_PREPARED  = 2;
    private static final int    STATE_PAUSED    = 3;
    private static final int    STATE_PLAYING   = 4;

    private int                 mCurrentState   = STATE_IDLE;
    private AudioManager        mAM;
    private IntentFilter        mPlugedFilter;

    private boolean             checkHeadset    = false;
    private static final String HEADSET         = "headset";

    private TimerTask           mTimerTask;
    private Timer               mTimer          = new Timer();
    private boolean             threadExit      = true;
    private boolean             threadExitDdr  = true;
    private static int          ledTime         = 500;
    Thread                      browseThread    = null;
    ThreadVar                   threadVar      = new ThreadVar();

   // private void writeFileToCache(String fileName) {
   //     AssetManager am = mContext.getAssets();
   //     try {
   //         File fullCache = new File(mContext.getCacheDir().getPath() + "/" + fileName);
   //         if (!fullCache.exists()) {
   //             fullCache.createNewFile();
   //         }
   //         mFullCache = fullCache;
   //         fullCache.setReadable(true, false);
   //         InputStream is = am.open(fileName);
   //         BufferedInputStream bis = new BufferedInputStream(is);
   //         BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(fullCache));
   //         byte[] buf = new byte[1 * 1024 * 1024];
   //         int length = 0;
   //         while ((length = bis.read(buf)) > 0) {
   //             bos.write(buf, 0, length);
   //         }
   //         bis.close();
   //         is.close();
   //         bos.close();
   //     } catch (IOException e) {
   //         e.printStackTrace();
   //     }
   // }

    public void playVideo(Uri uri) {
        if (uri == null) {
            // Log.e(Utils.APP_TAG,
            // "##########playVideo()..error: uri is null!");
            return;
        }
        try {
            mMediaPlayer.reset();
            mCurrentState = STATE_IDLE;
            mMediaPlayer.setDataSource(mContext, uri);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            // mMediaPlayer.setRawDataMode(MediaPlayer.AUDIO_DATA_MODE_SPDIF_RAW);
            mMediaPlayer.prepare();
            mCurrentState = STATE_PREPARING;
            return;
        } catch (Exception ex) {
            Log.e(TAG, "Unable to open content: " + uri.getPath());
            ex.printStackTrace();
            mCurrentState = STATE_ERROR;
            return;
        }
    }

    private Runnable mVideoRunnable = new Runnable() {
                                        @Override
                                        public void run() {
                                            playVideo(Uri.fromFile(mFullCache));

                                        }
                                    };

    @Override
    protected void finalize() throws Throwable {
        if (mMediaPlayer != null) {
            mMediaPlayer.release();
        }
        super.finalize();
    }

    @Override
    public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
        // TODO Auto-generated method stub

    }

    @Override
    public void surfaceCreated(SurfaceHolder arg0) {
        // TODO Auto-generated method stub
        mMediaPlayer.setDisplay(arg0);

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder arg0) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onVideoSizeChanged(MediaPlayer arg0, int arg1, int arg2) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onSeekComplete(MediaPlayer arg0) {
        // TODO Auto-generated method stub

    }

    @Override
    public boolean onInfo(MediaPlayer arg0, int arg1, int arg2) {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onBufferingUpdate(MediaPlayer arg0, int arg1) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onCompletion(MediaPlayer arg0) {
        // TODO Auto-generated method stub
        mCurrentState = STATE_IDLE;
        if (mTimerTask != null) {
            mTimerTask.cancel();
        }
        new Thread(mVideoRunnable).start();

    }

    @Override
    public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
        //mCurrentState = STATE_ERROR;
        //threadVar.threadExit = false;
        //Log.e(TAG, "##threadExit = " + threadVar.threadExit+", the Led will stop blinking");
        Log.e(TAG, "the media occured an error,but we ignore it!!!");
        return true;
    }

    @Override
    public void onPrepared(MediaPlayer arg0) {
        mCurrentState = STATE_PREPARED;
        mMediaPlayer.start();
        mProgressBar.setMax(mMediaPlayer.getDuration());
        mTimerTask = new TimerTask() {
            @Override
            public void run() {
                mProgressBar.setProgress(mMediaPlayer.getCurrentPosition());
            }
        };
        mTimer.schedule(mTimerTask, 500, 500);

    }

    @Override
    protected void onInitialize(Node attr) {
        mMaxFailedTimes = 0;
        if (attr == null || attr.getAttribute(HEADSET) == null) {
            checkHeadset = false;
        } else {
            checkHeadset = true;
        }

        setView(R.layout.case_video);
        mProgressBar = (ProgressBar) getView().findViewById(R.id.video_progress_bar);
        mAM = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        mRedo = (Button) getView().findViewById(R.id.video_redo);
        mTitle = (TextView) getView().findViewById(R.id.video_name);

        surfaceView = (SurfaceView) getView().findViewById(R.id.surfaceView1);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mRedo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                setResult(BaseCase.RESULT_RESET);
                startCase();
            }
        });
        // 保持在同一线程
        mEngine.getWorkerHandler().post(new Runnable() {
            @Override
            public void run() {
                String filePath = Utils.writeFileToCacheSecure(SAMPLE,mContext);
                if(filePath!=null){
                    mFullCache = new File(filePath);
                }
            }
        });

        mMediaPlayer = new MediaPlayer();
        mMediaPlayer.setOnPreparedListener(this);
        mMediaPlayer.setOnErrorListener(this);
        mMediaPlayer.setOnCompletionListener(this);
        mMediaPlayer.setOnBufferingUpdateListener(this);
        setName(R.string.case_video_name);

        mPlugedFilter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        mContext.registerReceiver(mReceiver, mPlugedFilter);
        AudioManager adm = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        /*
         * ArrayList<String> audioOutputChannels =
         * adm.getAudioDevices(AudioManager.AUDIO_OUTPUT_TYPE);
         * audioOutputChannels.contains(AudioManager.AUDIO_NAME_SPDIF);
         * adm.setAudioDeviceActive(audioOutputChannels,
         * AudioManager.AUDIO_OUTPUT_ACTIVE);
         */
        if (adm.isWiredHeadsetOn()) {
            Log.d(TAG, "wired headset on");
        }

    }

    protected void onPassableChange() {
        mUiHandler.post(new Runnable() {
            @Override
            public void run() {
                if (getPassable()) {
                    getView().findViewById(R.id.video_name).setBackgroundColor(Color.GREEN);
                    // setResult(BaseCase.RESULT_RESET);
                } else {
                    getView().findViewById(R.id.video_name).setBackgroundColor(Color.RED);
                }
            }
        });
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

                                            @Override
                                            public void onReceive(Context context, Intent intent) {
                                                if (intent.getIntExtra("state", -1) == 1) {
                                                    if (checkHeadset) {
                                                        // setPassable(true);
                                                        Log.d(TAG, "check headset");
                                                    }

                                                }
                                            }

                                        };

    @Override
    protected boolean onCaseStarted() {
        // 保持在同一线程
        mEngine.getWorkerHandler().post(new Runnable() {
            @Override
            public void run() {
                threadVar.threadExit = true;
                threadVar.threadExitDdr = true;
                playVideo(Uri.fromFile(mFullCache));
                Log.e(TAG, "###onCaseStarted");
                //startCtrlLedThread();
            }
        });
        return false;
    }

    @Override
    protected void onCaseFinished() {
        // 保持在同一线程
        mEngine.getWorkerHandler().post(new Runnable() {
            @Override
            public void run() {
                mMediaPlayer.stop();
                mMediaPlayer.reset();
                if (mTimerTask != null) {
                    mTimerTask.cancel();
                }
            }
        });
        // mMediaPlayer.setRawDataMode(MediaPlayer.AUDIO_DATA_MODE_HDMI_RAW);

    }

    @Override
    protected void onRelease() {
        // 保持在同一线程
        onCaseFinished();
        mEngine.getWorkerHandler().post(new Runnable() {
            @Override
            public void run() {
                mMediaPlayer.release();
            }
        });
        mContext.unregisterReceiver(mReceiver);

    }

    private void chanceLedStatus(int status) {
        // char portType = 'x';
        // int portNum = 1;
        // if (Config.getTargetPlatform(0x00) == Config.PLATFORM_EAGLE) {
        //     portType = 'x';
        //     portNum = 1;
        // } else if (Config.getTargetPlatform(0x00) == Config.PLATFORM_DOLPHIN) {
        //     portType = 'a';
        //     portNum = 15;
        // } else if (Config.getTargetPlatform(0x00) == Config.PLATFORM_JAWS) {
        //     portType = 'h';
        //     portNum = 20;
        // }
        // Gpio.writeGpio(portType, portNum, status);
        if (status > 0) {
            com.softwinner.Gpio.setNormalLedOn(false);
            com.softwinner.Gpio.setStandbyLedOn(true);
        } else {
            com.softwinner.Gpio.setNormalLedOn(true);
            com.softwinner.Gpio.setStandbyLedOn(false);
        }
    }

    private void startCtrlLedThread() {
        browseThread = new Thread() {
            public void run() {
                try {
                    threadExit = threadVar.threadExit;
                    threadExitDdr = threadVar.threadExitDdr;
                    Log.e(TAG, "threadExitDdr=" + threadExitDdr);
                    Log.e(TAG, "threadExit=" + threadExit);
                    while (threadExitDdr && threadExit) {
                        if (ledTime <= 0) {
                            ledTime = 500;
                        }
                        chanceLedStatus(0);
                        Thread.sleep(ledTime);
                        chanceLedStatus(1);
                        Thread.sleep(ledTime);
                        threadExit = threadVar.threadExit;
                        threadExitDdr = threadVar.threadExitDdr;
                    }
                    chanceLedStatus(0);
                } catch (Exception e) {
                    e.printStackTrace();
                }

            }
        };
        browseThread.start();
    }
}
