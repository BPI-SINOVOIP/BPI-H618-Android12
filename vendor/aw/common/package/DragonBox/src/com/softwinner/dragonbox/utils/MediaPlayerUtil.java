package com.softwinner.dragonbox.utils;

import java.io.IOException;
import java.util.ArrayList;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.SoundPool;
import android.util.Log;
import android.widget.Toast;

import com.softwinner.dragonbox.R;

public class MediaPlayerUtil {

    private final static String TAG = "MusicerUtil";
    private static MediaPlayerUtil instance;
    private int lastStreamVolume;
    private AudioManager mAM;
    MediaPlayer mediaPlayer;

    Context mContext;

    public static final int PLAY_LEFT = 1;
    public static final int PLAY_RIGHT = 2;
    public static final int PLAY_NORMAL = 3;
    public static final int PLAY_SPEAKER = 4;

    public static synchronized MediaPlayerUtil getInstance(Context context) {
        if (instance == null) {
            instance = new MediaPlayerUtil(context);
        }
        return instance;
    }

    private MediaPlayerUtil(Context context) {
        mContext = context;
        mAM = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        //ArrayList<String> audioOutputChannels = mAM.getActiveAudioDevices(AudioManager.AUDIO_OUTPUT_ACTIVE);

        //for (String stc : audioOutputChannels) {
        //	Log.e(TAG, "=audioOutputChannels=" + "." + stc + ".");
        //}
        //audioOutputChannels.clear();
        //audioOutputChannels.add(AudioManager.AUDIO_NAME_CODEC);
        //audioOutputChannels.add(AudioManager.AUDIO_NAME_HDMI);
        //audioOutputChannels.add(AudioManager.AUDIO_NAME_SPDIF);

        //mAM.setAudioDeviceActive(audioOutputChannels,AudioManager.AUDIO_OUTPUT_ACTIVE);
        //int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        //mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
    }

    /**
     *
     * @param playType
     *            PLAY_LEFT or PLAY_RIGHT or PLAY_NORMAL
     */
    public boolean playMusic(int playType) {
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);

        mediaPlayer = new MediaPlayer();
        mediaPlayer.reset();
        final AssetFileDescriptor fileDesc;
        switch (playType) {
            case PLAY_LEFT:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.left);
                mediaPlayer.setVolume(1, 0);
                break;
            case PLAY_RIGHT:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.right);
                mediaPlayer.setVolume(0, 1);
                break;
            case PLAY_NORMAL:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi);
                mediaPlayer.setVolume(1, 1);
                break;
            case PLAY_SPEAKER:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.ringtone);
                mediaPlayer.setVolume(1, 1);
                break;
            default:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi);
                break;
        }

        try {
            mediaPlayer.setDataSource(fileDesc.getFileDescriptor(), fileDesc.getStartOffset(), fileDesc.getDeclaredLength());
            mediaPlayer.prepareAsync();
            mediaPlayer.setOnPreparedListener(new OnPreparedListener(){
                public void onPrepared(MediaPlayer arg0){
                    mediaPlayer.setLooping(true);
                    mediaPlayer.start();
                    try {
                        fileDesc.close();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            });
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return false;
    }

    public boolean playMusicRandom(int playType,int random) {
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);

        mediaPlayer = new MediaPlayer();
        mediaPlayer.reset();
        final AssetFileDescriptor fileDesc;
        switch (playType) {
            case PLAY_LEFT:
                switch (random) {
                    case 1:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_l1);
                        break;
                    case 2:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_l2);
                        break;
                    case 3:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_l3);
                        break;
                    case 4:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_l4);
                        break;
                    default:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_l1);
                        break;
                }
                mediaPlayer.setVolume(1, 0);
                break;
            case PLAY_RIGHT:
                switch (random) {
                    case 1:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_r1);
                        break;
                    case 2:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_r2);
                        break;
                    case 3:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_r3);
                        break;
                    case 4:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_r4);
                        break;
                    default:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.cvbs_r1);
                        break;
                }
                mediaPlayer.setVolume(0, 1);
                break;
            case PLAY_NORMAL:
                switch (random) {
                    case 1:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi1);
                        break;
                    case 2:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi2);
                        break;
                    case 3:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi3);
                        break;
                    case 4:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi4);
                        break;
                    default:
                        fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi1);
                        break;
                }
                mediaPlayer.setVolume(1, 1);
                break;
            default:
                fileDesc = mContext.getResources().openRawResourceFd(R.raw.hdmi);
                break;
        }

        try {
            mediaPlayer.setDataSource(fileDesc.getFileDescriptor(), fileDesc.getStartOffset(), fileDesc.getDeclaredLength());
            mediaPlayer.prepareAsync();
            mediaPlayer.setOnPreparedListener(new OnPreparedListener(){
                public void onPrepared(MediaPlayer arg0){
                    mediaPlayer.setLooping(true);
                    mediaPlayer.start();
                    try {
                        fileDesc.close();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            });
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return false;
    }

    public void pause() {
        if (mediaPlayer != null) {
            mediaPlayer.pause();
        }
    }

    public void stop() {
        if (mediaPlayer != null && mediaPlayer.isPlaying()) {
            mediaPlayer.stop();
        }
    }

    public void release() {
        if (mediaPlayer != null) {
            mediaPlayer.release();
            mediaPlayer = null;
        }
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, lastStreamVolume, 0);
    }
}
