package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;
import java.io.File;
import java.util.List;
import java.util.ArrayList;
import java.util.LinkedList;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.util.Log;
import android.util.TypedValue;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.TextView;
import java.lang.*;


import android.media.AudioManager;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.security.PrivilegedActionException;
import java.math.BigInteger;

import android.R.integer;
import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.widget.AbsoluteLayout;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.MediaController;
import android.widget.TextView;
import android.view.MenuItem;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.View;
import android.content.Context;
import android.media.AudioRecord;
import android.media.AudioFormat;
import android.media.MediaRecorder;
import android.util.Log;
import android.view.KeyEvent;
import com.softwinner.dragonbox.R;

public class CaseMicRecordGraphic extends IBaseCase implements SurfaceHolder.Callback, OnClickListener{
    private static String TAG = "CaseMicRecordGraphic";
    private static int SLEEP_TIME = 25;

    private Paint circlePaint;
    private Paint center;
    private Paint paintLine;
    private Paint mPaint;
    private int mChannelCount = 0;//待显示的波形数目
    private int mChannel = AudioFormat.CHANNEL_IN_STEREO;
    private int mSource = MediaRecorder.AudioSource.VOICE_RECOGNITION;
    private int mFormat = AudioFormat.ENCODING_PCM_16BIT;
    private int mSampleRate = 48000;

    private int mRateX = 1;//

    private int mXStep = 10;
    private int mPadding = 20;
    private final Object mSurfaceLock = new Object();
    private DrawThread mThread;
    private RecordThread mRecordThread;
    private AudioRecord ar;
    private TextView mTvMicRecordResult;
    private boolean recording = false;
    private boolean writing = false;
    private Button recordOrPauseBtn;
    private List<File> saveFile;
    private List<FileOutputStream> saveFileStream;
    private List<Button> lstPlayMicButton = new ArrayList<Button>();//根据待显示的波形数目添加对应的播放按钮
    private LinearLayout mLlPlayMic;//
    private LinkedList[] mPcmBuffers ;//显示的波形对应的声道数
    SurfaceView mMaxSurfaceV;//显示声音波形
    private MediaPlayer  mMediaplayer;
    private Context mContext;

    public CaseMicRecordGraphic(Context context) {
        super(context, R.string.case_micrecord_name, R.layout.case_micrecord_graphic_max,
                R.layout.case_micrecord_min, TYPE_MODE_MANUAL);
        mContext = context;
        mMaxSurfaceV = (SurfaceView) mMaxView.findViewById(R.id.case_mic_surface);
        mMaxSurfaceV.setZOrderOnTop(true);
        LayoutParams lp = mMaxSurfaceV.getLayoutParams();
        lp.width = (int)context.getResources().getDimension(R.dimen.camera_size_width);
        lp.height = (int)context.getResources().getDimension(R.dimen.camera_size_height);
        Log.d("mylog", "width = "+lp.width+"height = "+lp.height);
        mTvMicRecordResult = (TextView) mMinView.findViewById(R.id.case_micrecord_status);
    }

    private void playRecord(int channel) {
        try {
            Log.i(TAG,"play record :"+(channel+1) + " path=" + saveFile.get(channel).getPath());
            if (mMediaplayer == null) {
                mMediaplayer = new MediaPlayer();
            }
            mMediaplayer.reset();
            mMediaplayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaplayer.setDataSource(saveFile.get(channel).getPath());
            mMediaplayer.setOnPreparedListener(new OnPreparedListener() {
                    public void onPrepared(MediaPlayer mp) {
                        mMediaplayer.start();
                        }
            });
            mMediaplayer.prepareAsync();
        }catch(Exception e)
        {
            e.printStackTrace();
        }
    }

    public void setParameter(int iChannel, int intDisplayChannel, int intSampleRate,int intSource){
        mChannel = iChannel;
        mChannelCount = intDisplayChannel;
        mSource = intSource;
        mSampleRate = intSampleRate;
        mPcmBuffers =  new LinkedList[mChannelCount];
        saveFile = new ArrayList<File>();
        for (int i = 0; i < mChannelCount; i++){
            mPcmBuffers[i] = new LinkedList();//生成空链表，初始化。
            saveFile.add(new File("/sdcard/test_"+i+".wav"));
        }
    }

    private void initCanvas(){
        circlePaint = new Paint();//画圆
        circlePaint.setColor(Color.rgb(246, 131, 126));//设置上圆的颜色
        center = new Paint();
        center.setColor(Color.rgb(39, 199, 175));// 画笔为color
        center.setStrokeWidth(1);// 设置画笔粗细
        center.setAntiAlias(true);
        center.setFilterBitmap(true);
        center.setStyle(Paint.Style.FILL);
        paintLine =new Paint();
        paintLine.setColor(Color.rgb(169, 169, 169));
        mPaint = new Paint();
        mPaint.setColor(Color.rgb(39, 199, 175));// 画笔为color
        mPaint.setStrokeWidth(1);// 设置画笔粗细
        mPaint.setAntiAlias(true);
        mPaint.setFilterBitmap(true);
        mPaint.setStyle(Paint.Style.FILL);
    }

    private void initPlayMicButton() {
        mLlPlayMic = (LinearLayout) mMaxView.findViewById(R.id.case_ll_play_mic);
        recordOrPauseBtn = (Button) mMaxView.findViewById(R.id.record_or_pase);
        int canvasHeight = (int)mContext.getResources().getDimension(R.dimen.camera_size_height);
        for(int i=0;i<mChannelCount;i++) {
            Button button = new Button(mContext);
            button.setText("M"+(i+1));
            button.setEnabled(false);
            button.setSingleLine(true);
            button.setPadding(1, 1, 1, 1);
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, 12);
            button.setOnClickListener(this);
            LayoutParams layoutParams = new LayoutParams(LayoutParams.WRAP_CONTENT, canvasHeight/mChannelCount);
            mLlPlayMic.addView(button, i,layoutParams);
            lstPlayMicButton.add(button);
        }
        Log.e("mylog", "view size = "+mLlPlayMic.getChildCount());
        recordOrPauseBtn.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v)
                {
                    recording = !recording;
                    if (recording) {
                        recordOrPauseBtn.setText(R.string.case_micrecord_stop);
                        for(int i=0;i<lstPlayMicButton.size();i++) {
                        	lstPlayMicButton.get(i).setEnabled(false);
                        }
                    } else {
                        recordOrPauseBtn.setText(R.string.case_micrecord_record);
                        for(int i=0;i<lstPlayMicButton.size();i++) {
                            lstPlayMicButton.get(i).setEnabled(true);
                        }
                    }
                }
        });
    }

    public CaseMicRecordGraphic(Context context, XmlPullParser xmlParser) {
        this(context);
        mContext = context;
        String sChannel = xmlParser.getAttributeValue(null, "channel");//AudioRecord的传参，录制几个声道，注意区分displayChannel,左声道(4),右声道(8)，左右声道(12),8声道(1020)
        String strDisplayChannel = xmlParser.getAttributeValue(null, "displayChannel");//将录制的声道数解析成  几个  波形显示(1,2,4,6,8)
        String strSampleRate = xmlParser.getAttributeValue(null, "sampleRate");//sampleRate：AudioRecord的传参，采样率，可选值：16000/48000
        String strSource = xmlParser.getAttributeValue(null, "source");//source：AudioRecord的传参，录制来源，可选值：1(MIC)、6(VOICE_RECOGNITION)
        String strFormat = xmlParser.getAttributeValue(null, "format");//AudioRecord的传参，录制声音的每个点使用多少位表示，可选值：2(16bit)、3(8bit)
        int iChannel=12;
        int intDisplayChannel=6;
        int intSampleRate=48000;
        int intSource=6;
        try{
            iChannel = Integer.parseInt(sChannel);
            intDisplayChannel = Integer.parseInt(strDisplayChannel);
            intSampleRate = Integer.parseInt(strSampleRate);
            intSource = Integer.parseInt(strSource);
        } catch (Exception e) {
            e.printStackTrace();
            iChannel = 1;
        }
        setParameter(iChannel,intDisplayChannel,intSampleRate,intSource);
        initCanvas();
        initPlayMicButton();
    }

    public CaseMicRecordGraphic(Context context, int c) {
        this(context);
        setParameter(1020,8,16000,1);
        initCanvas();
        initPlayMicButton();
    }

    @Override
    public void onStartCase() {
        SurfaceHolder surfaceHolder;
        surfaceHolder = mMaxSurfaceV.getHolder();
        surfaceHolder.addCallback(this);
        setDialogPositiveButtonEnable(true);
    }

    @Override
    public void onStopCase() {
        try{
            synchronized (mSurfaceLock) {
                mThread.setRun(false);
                mRecordThread.setRun(false);
                if(ar.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
                    ar.stop();
                }
                if(ar != null) {
                    ar.release();
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        mTvMicRecordResult.setText(getCaseResult() ? R.string.case_micrecord_status_success_text
                : R.string.case_micrecord_status_fail_text);
        if (mMediaplayer != null) {
            try{
                mMediaplayer.release();
                mMediaplayer = null;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        for(int i=0;i<mChannelCount;i++) {
            File file = saveFile.get(i);
            if(file.exists()) {
                file.delete();
            }
        }
    }

    @Override
    public void reset() {
        super.reset();
    }

    @Override
    public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {

    }

    private class RecordThread extends Thread{
        private boolean mIsRun = false;
        RecordThread(){
        }

        public void setRun(boolean isRun) {
            this.mIsRun = isRun;
        }

        @Override
        public void run() {
            int bufferSize = AudioRecord.getMinBufferSize(mSampleRate, mChannel, mFormat);
            bufferSize = (bufferSize + (mChannelCount*mFormat-1))/(mChannelCount*mFormat)*(mChannelCount*mFormat);
            ar = new AudioRecord(mSource, mSampleRate, mChannel, mFormat, bufferSize);
            short[] buffer = new short[bufferSize/2];//bufferSize 对应的单位是byte，1short＝2byte。
            ar.startRecording();
            int size = 0;
            int pointsCount = (mMaxSurfaceV.getWidth() - mPadding * 2) / mXStep;//点的数量
            Log.i(TAG,"format : "+mFormat +" count:"+ mChannelCount);
            int totalRecordSize = 0;
            try {
                while (mIsRun && (size = ar.read(buffer,0, bufferSize/2)) != -1){//bufferSize 对应的单位是byte，1short＝2byte。
                    if (size==0) {
                        try {
                            Thread.sleep(33);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        continue;
                    }
                    if (size % (mFormat * mChannelCount) != 0) {
                        Log.e(TAG,"error size :  "+size +" not % by " +(mFormat * mChannelCount));
                        throw new Exception("error size");
                    }

                    if (recording) {
                        if (!writing) {
                            if (saveFileStream == null) {
                                saveFileStream = new ArrayList<FileOutputStream>();
                            }
                            saveFileStream.clear();
                            totalRecordSize = 0;
                            for (int i = 0; i < mChannelCount; i++) {
                                File f = saveFile.get(i);
                                FileOutputStream output = new FileOutputStream(f, false);
                                saveFileStream.add(output);
                                WavHeadWriter.writeHEAD(output,1,mSampleRate,audioFormat2int(ar.getAudioFormat()),1);
                            }
                            writing = true;
                        }
                        // write data
                        totalRecordSize += size*mFormat/mChannelCount;
                        for (int j = 0; j < mChannelCount; j++) {
                            FileOutputStream output = saveFileStream.get(j);
                            ByteBuffer buf = ByteBuffer.allocate(size*mFormat/mChannelCount).order(ByteOrder.LITTLE_ENDIAN);
                            for (int i = j; i < size; i = i + mChannelCount) {
                                buf.putShort(buffer[i]);
                            }
                            output.write(buf.array(),0,size*mFormat/mChannelCount);
                        }
                    } else if (writing) {
                        for (int i = 0; i < mChannelCount; i++) {
                            FileOutputStream output = saveFileStream.get(i);
                            output.close();
                            WavHeadWriter.adjustFileSize(saveFile.get(i).getPath(), totalRecordSize);
                        }
                        writing = false;
                    }

                    synchronized (mSurfaceLock) {
                        long[] sum = new long[mChannelCount];
                        int[] value = new int[mChannelCount];
                        for (int i = 0; i < size; i = i + (1 * mChannelCount) * mRateX) {//mForamt
                            for (int j = 0; j < mChannelCount; j++) {
                                value[j] = buffer[i + j];
                            }
                            for (int j = 0; j < mChannelCount; j++) {
                                sum[j] += Math.abs(value[j]);
                            }
                        }

                        for (int i = 0; i < mChannelCount; i++) {
                            while (mPcmBuffers[i].size() > pointsCount) {
                                mPcmBuffers[i].poll();
                            }
                            float result = sum[i] / (size /mChannelCount) / (5 + 10 * (mChannelCount - 1));// 界面布局兼容不同声道数
                            mPcmBuffers[i].add(result);
                        }
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void startShowRecord(){
        mRecordThread = new RecordThread();
        mRecordThread.setRun(true);
        mRecordThread.start();
    }

    private void doDraw(Canvas canvas){
        canvas.drawARGB(255, 239, 239, 239);
        int contentHeight = canvas.getHeight();
        Log.e("mylog", "contentHeight = "+contentHeight);
        int contentWidth = canvas.getWidth() - mPadding * 2;
        float baseline[] = new float[mChannelCount];
        for (int i = 0 ; i < baseline.length; i++){//横轴
            baseline[i] = contentHeight / baseline.length * (i+1);
            canvas.drawLine(mPadding, baseline[i], contentWidth + mPadding, baseline[i], paintLine);
        }

        canvas.drawLine(mPadding, 0, mPadding, canvas.getHeight(), paintLine);//纵轴
        for (int i = 0; i < mPcmBuffers.length; i++) {
            float preX = mPadding;
            float preY = baseline[i];
            float curX = 0;
            float curY = 0;
            for (int j = 0; j < mPcmBuffers[i].size(); j++) {
                curX = mXStep * j + mPadding;
                curY = (Float) mPcmBuffers[i].get(j) * (-1.5f) + baseline[i];
                canvas.drawLine(preX, preY, curX, curY, mPaint);
                preX = curX;
                preY = curY;
            }
            canvas.drawLine(curX, curY, curX, baseline[i], mPaint);
        }
    }

    private class DrawThread extends Thread {
        private SurfaceHolder mHolder;
        private boolean mIsRun = false;

        public DrawThread(SurfaceHolder holder) {
            super(TAG);
            mHolder = holder;
        }
        @Override
        public void run() {
            while(true) {
                synchronized (mSurfaceLock) {
                    if (!mIsRun) {
                        return;
                    }
                    Canvas canvas = mHolder.lockCanvas();
                    if (canvas != null) {
                        doDraw(canvas);  //这里做真正绘制的事情
                        mHolder.unlockCanvasAndPost(canvas);
                    }else {
                    	Log.e(TAG, "canvas = null");
                    }
                }
                try {
                    Thread.sleep(SLEEP_TIME);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

        public void setRun(boolean isRun) {
            this.mIsRun = isRun;
        }
    }
	
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		startShowRecord();
        mThread = new DrawThread(holder);
        mThread.setRun(true);
        mThread.start();
        Log.d(TAG, "surfaceCreated: ");
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder arg0) {
		Log.d("CaseCamera", "surfaceDestroyed");
		 synchronized (mSurfaceLock) {
	            mThread.setRun(false);
	            mRecordThread.setRun(false);
	        }
	}


    static int audioFormat2int(int format) {
     if (format == AudioFormat.ENCODING_PCM_16BIT) {
            return 16;
        } else if (format == AudioFormat.ENCODING_PCM_8BIT) {
            return 8;
        } else {
            return 0;
        }
    }
    static String audioFormat2String(int format) {
        if (format == AudioFormat.ENCODING_PCM_16BIT) {
            return "pcm_16bit";
        } else if (format == AudioFormat.ENCODING_PCM_8BIT) {
            return "pcm_8bit";
        } else {
            return "unknow format";
        }
    }

    static class WavHeadWriter {
        public static void writeHEAD(FileOutputStream output, int channel, int sampleRate, int nBitsPerSample, int encodeMode) {
            if (output==null) return;
            try {
            String uRiffFcc = "RIFF";
            byte[] b_uRiffFcc = uRiffFcc.getBytes("US-ASCII");
            String uWaveFcc = "WAVE";
            byte[] b_uWaveFcc = uWaveFcc.getBytes("US-ASCII");
            String uFmtFcc = "fmt ";
            byte[] b_uFmtFcc = uFmtFcc.getBytes("US-ASCII");
            String uDataFcc = "data";
            byte[] b_uDataFcc = uDataFcc.getBytes("US-ASCII");

                output.write(b_uRiffFcc,0,b_uRiffFcc.length);
                output.write(int2bytes(0x2c-8),0,4);
                output.write(b_uWaveFcc,0,b_uWaveFcc.length);
                output.write(b_uFmtFcc,0,b_uFmtFcc.length);

                output.write(int2bytes(16),0,4);
                output.write(int2bytes(encodeMode),0,2);
                output.write(int2bytes(channel),0,2);
                output.write(int2bytes(sampleRate),0,4);
                output.write(int2bytes(sampleRate*channel*(nBitsPerSample/8)),0,4);
                output.write(int2bytes(channel*(nBitsPerSample/8)),0,2);
                output.write(int2bytes(nBitsPerSample),0,2);
                output.write(b_uDataFcc,0,b_uDataFcc.length);
                output.write(int2bytes(0),0,4);
            } catch (Exception e) {
                Log.w("xxx", "write Wav head fail", e);
            }
        }
        public static void adjustFileSize(String fileName, int recordSize) {
            try {
            RandomAccessFile f = new RandomAccessFile(new File(fileName),"rw");
            long pos1 = 4;
            long pos2 = 40;
            f.seek(pos1);
            f.write(int2bytes(recordSize + 0x2c-8),0,4);
            f.seek(pos2);
            f.write(int2bytes(recordSize),0,4);
            } catch(Exception e) {
                Log.w("xxx", "adjustFileSize error:",e);
            }
        }
        public static byte[] int2bytes(int i) {
            byte[] x = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(i).array();
            //Log.i("xxx",String.format("4 0x%02x%02x%02x%02x",x[0],x[1],x[2],x[3]));
            return x;
        }
    }
    static void putShort(byte b[], short s, int index) {
        b[index + 1] = (byte) (s >> 8);
        b[index + 0] = (byte) (s >> 0);
    }

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		Button button;
		if(v instanceof Button)
			button =(Button)v;
		else
			return;
		int index = lstPlayMicButton.indexOf(button);
		playRecord(index);
	}

}
