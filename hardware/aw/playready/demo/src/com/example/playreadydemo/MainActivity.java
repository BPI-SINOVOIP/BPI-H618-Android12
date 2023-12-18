
package com.example.playreadydemo;

import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaDrm;
import android.media.NotProvisionedException;
import android.media.ResourceBusyException;
import android.media.MediaDrmException;


import android.os.Bundle;
import android.os.Parcel;
import android.app.Activity;
import android.util.Base64;
import android.util.Log;
import android.view.Menu;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Toast;
import android.os.Looper;


import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.UUID;


public class MainActivity extends Activity implements View.OnClickListener {

    private static final String TAG = "playready";
    private MediaPlayer mp;
    private SurfaceView sv;
    private Button pause;
    private Spinner spinner;
    // must stay in sync with mediaplayer.h
    static final int INVOKE_ID_PLAYREADY_DRM = 9900;
    static final int FUN_PROCESS_LICENSE = 1;
    private MediaPlayer mMediaPlayer;
    private Method mNewRequestMethod;
    private Method mInvokeMethod;
    private MediaDrm mDrm;
    private Looper mLooper;
    private Object mLock = new Object();
    private byte[] mSessionId;
    private boolean mHasgetId =false;
    static final UUID kPlayReadyScheme = new UUID(0x9a04f07998404286L, 0xab92e65be0885f95L);
    //static final UUID kPlayReadyScheme = new UUID(0x9A04F07998404286L, 0xAB92E65BE0885F95L);
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mp=new MediaPlayer();
        sv=(SurfaceView)findViewById(R.id.surfaceView1);
        pause =(Button)findViewById(R.id.pause);
        pause.setOnClickListener(this);
        Button btn = (Button)findViewById(R.id.start);
        btn.setOnClickListener(this);
        btn = (Button)findViewById(R.id.stop);
        btn.setOnClickListener(this);
        btn = (Button)findViewById(R.id.license);
        btn.setOnClickListener(this);
        btn = (Button)findViewById(R.id.debug);
        btn.setOnClickListener(this);
        
        ArrayAdapter<CharSequence> adapter = new ArrayAdapter<CharSequence>(this, android.R.layout.simple_spinner_item);
        // playready encrypted
        adapter.add("http://test.playready.microsoft.com/smoothstreaming/SSWSS720H264PR/SuperSpeedway_720.ism/Manifest");
        adapter.add("http://playready.directtaps.net/smoothstreaming/SSWSS720H264PR/SuperSpeedway_720.ism/Manifest");
        // normal, non encrypted
        adapter.add("http://playready.directtaps.net/smoothstreaming/SSWSS720H264/SuperSpeedway_720.ism/Manifest");
        spinner = (Spinner)findViewById(R.id.spinner);
        spinner.setAdapter(adapter);
        
        mp.setOnCompletionListener(new OnCompletionListener(){

            @Override
            public void onCompletion(MediaPlayer mp) {
                Toast.makeText(MainActivity.this, "video playback complete!", Toast.LENGTH_SHORT).show();
            }
        });
        
        setupInvoke();
    }
    
    @Override
    public void onClick(View v) {
        switch (v.getId())
        {
            case R.id.license:
                doLicense(spinner.getSelectedItem().toString());
                break;
            case R.id.start:
                mp.reset();
                try {
                    //mp.setDataSource("http://playready.directtaps.net/smoothstreaming/SSWSS720H264PR/SuperSpeedway_720.ism/Manifest");
                    String source = spinner.getSelectedItem().toString();
					Log.d(TAG, "gqy=======source= " + source);
                    mp.setDataSource(source);
                    mp.setDisplay(sv.getHolder());
                    mp.prepare();
                    mp.start();
                    pause.setText("pause");
                    pause.setEnabled(true);
                }catch(IOException e) {
                    e.printStackTrace();
                }
                break;
            case R.id.pause:
                if(mp.isPlaying()){
                    mp.pause();
                    ((Button)v).setText("resume");
                }else{
                    mp.start();
                    ((Button)v).setText("pause");
                }
                break;
            case R.id.stop:
                if(mp.isPlaying()){
                    mp.stop();
                    pause.setEnabled(false);
                }
                break;
            case R.id.debug:
                Log.d(TAG, spinner.getSelectedItem().toString());
                break;
            default:
                break;
        }
    }


    @Override
    protected void onDestroy() {
        if(mp.isPlaying()){
            mp.stop();
        }
        mp.release();
        if(mHasgetId == true){
            mDrm.closeSession(mSessionId);
            stopDrm(mDrm);
            mHasgetId = false;
        }
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    private MediaDrm startDrm() {

        new Thread() {
            @Override
            public void run() {
                // Set up a looper to handle events
                Looper.prepare();

                // Save the looper so that we can terminate this thread
                // after we are done with it.
                mLooper = Looper.myLooper();

                try {
                    mDrm = new MediaDrm(kPlayReadyScheme);
                } catch (MediaDrmException e) {
                    Log.e(TAG, "Failed to create MediaDrm", e);
                    return;
                }

                synchronized(mLock) {

                    mDrm.setOnEventListener(new MediaDrm.OnEventListener() {
                            @Override
                            public void onEvent(MediaDrm md, byte[] sessionId, int event,
                                                int extra, byte[] data) {
                                if (event == MediaDrm.EVENT_PROVISION_REQUIRED) {
                                    Log.i(TAG, "Provisioning is required");
                                } else if (event == MediaDrm.EVENT_KEY_REQUIRED) {
                                    Log.i(TAG, "MediaDrm event: Key required");
                                } else if (event == MediaDrm.EVENT_KEY_EXPIRED) {
                                    Log.i(TAG, "MediaDrm event: Key expired");
                                } else if (event == MediaDrm.EVENT_VENDOR_DEFINED) {
                                    Log.i(TAG, "MediaDrm event: Vendor defined: " + event);
                                }
                            }
                        });
                    mLock.notify();
                }

                Looper.loop();  // Blocks forever until Looper.quit() is called.
            }
        }.start();

        // wait for mDrm to be created
        synchronized(mLock) {
            try {
                mLock.wait(1000);
            } catch (Exception e) {
            }
        }

        if (mDrm == null) {
            Log.e(TAG, "Failed to create drm");
        }

        return mDrm;
    }
    private void stopDrm(MediaDrm drm) {
        if (drm != mDrm) {
            Log.e(TAG, "Invalid drm specified in stopDrm");
        }
        mLooper.quit();
    }
    private void sleep(int msec) {
        try {
            Thread.sleep(msec);
        } catch (InterruptedException e) {
        }
    }
    private byte[] openSession(MediaDrm drm) {
        byte[] sessionId = null;
        int retryCount = 3;
		drm.setPropertyString("DeviceStoreName","/data/vendor/mediadrm/playready/pr.hds");
        while (--retryCount > 0) {
            try {
                sessionId = drm.openSession();
                break;
            } catch (NotProvisionedException e) {
                Log.e(TAG, "playready no need provision but open session failed!!!");
            } catch (ResourceBusyException e) {
                Log.w(TAG, "Resource busy in openSession, retrying...");
                sleep(1000);
            }
        }

        if (retryCount == 0) {
            Log.e(TAG, "Failed to provision device");
        }
        return sessionId;
    }
    private boolean getKeys(MediaDrm drm, byte[] sessionId,String wrmheader) {
        final byte[] initData = wrmheader.getBytes();
        KeyRequester keyRequester = new KeyRequester(initData);
        boolean result = keyRequester.doTransact(drm, sessionId);
        if (!result) {
            Log.e(TAG,"Failed to get keys from license server!");
        }
        return result;
    }
    private String getLAURL(String wrmheader)
    {
        String LA_URL = null;
        try {
            XmlPullParserFactory xmlFactoryObject = XmlPullParserFactory.newInstance();
            XmlPullParser xmlparser = xmlFactoryObject.newPullParser();
            xmlparser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false);
            xmlparser.setInput(new StringReader(wrmheader));

            String text = null;
            int event = xmlparser.getEventType();
            while (event != XmlPullParser.END_DOCUMENT) {
                String name = xmlparser.getName();
                switch (event) {
                    case XmlPullParser.START_TAG:
                        if (name.equals("LA_URL")) {
                            LA_URL = null;
                        }
                        break;

                    case XmlPullParser.TEXT:
                        text = xmlparser.getText();
                        break;

                    case XmlPullParser.END_TAG:
                        if (name.equals("LA_URL")) {
                            Log.d(TAG, "LA_URL:" + text);
                            LA_URL = text;
                        }
                        break;
                }
                event = xmlparser.next();
            }
            Log.d(TAG, "parse xml end.");

        } catch (XmlPullParserException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return LA_URL;
    }
    
    private void setupInvoke()
    {
        // get mediaplayer
        mMediaPlayer = new MediaPlayer();
        Class<MediaPlayer> mpcls = MediaPlayer.class;
        Class<?> params[] = new Class[2];
        params[0] = Parcel.class;
        params[1] = Parcel.class;
        Class<?> no_params[] = null;
        try {
            mMediaPlayer.setDataSource("http://impossible.com/trythis.xml");
            mInvokeMethod = mpcls.getDeclaredMethod("invoke", params);
            if (mInvokeMethod != null) {
                Log.i(TAG, "Got the invoke method: "+mInvokeMethod);
            }
            mNewRequestMethod = mpcls.getDeclaredMethod("newRequest", no_params);
            if (mNewRequestMethod != null) {
                Log.i(TAG, "Got the Newrequest method: " + mNewRequestMethod);
            }
        } catch (Exception x) {
            Log.e(TAG, "Exception: "+x+", "+x.getLocalizedMessage());
        }
    }
    private int ackAndProcessLicense(String wrmheader)
    {
        // do the stuff using mediaplayer invoke
        Log.d(TAG, "ackAndProcessLicense");
		MediaDrm drm = startDrm();
		if(drm != null){
			Log.d(TAG,"we use meidadrm to get license!");
			//we shouldn't close session before video play. playready crypto plugin
			//depend on drm plugin.
			if(mHasgetId == false){
				mSessionId = openSession(drm);
				mHasgetId = true;
			}
			if (getKeys(drm, mSessionId,wrmheader)!= true) {
				Log.d(TAG,"get keys failed!!");
                return -1;
        	}
			return 0;
		}else{
        	String LA_URL = getLAURL(wrmheader);
        	try {
            	Parcel request = (Parcel)mNewRequestMethod.invoke(mMediaPlayer, 
                	                                              (Object[])null);
            	Parcel response = Parcel.obtain();
            	request.writeInt(INVOKE_ID_PLAYREADY_DRM);
            	request.writeInt(FUN_PROCESS_LICENSE);
            	request.writeByteArray(wrmheader.getBytes());
            	request.writeByteArray(LA_URL.getBytes());
            	mInvokeMethod.invoke(mMediaPlayer, request, response);
            	return response.readInt();
        	} catch (Exception x) {
            	Log.e(TAG, "Exception: "+x+", "+x.getLocalizedMessage());
            	return -1;
        	}
		}
    }
    
    private void ShowText(final String text) {
        Log.i(TAG, text);
        //Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
        runOnUiThread(new Runnable() {
            public void run() {
              Toast.makeText(MainActivity.this, text, Toast.LENGTH_SHORT).show();
            }
        });
    }
    
    private void doLicense(final String manifest)
    {
        Log.d(TAG, "doLicense, manifest: " + manifest);
        Thread thread = new Thread(new Runnable(){
            @Override
            public void run() {
                String WRMHeader = null;
                InputStream stream = null;
                try {
                    
                    // 1. get base64-encoded wrmheader from network
                    URL url = new URL(manifest);
                    HttpURLConnection conn = (HttpURLConnection)url.openConnection();
                 
                    conn.setReadTimeout(10000 /* milliseconds */);
                    conn.setConnectTimeout(15000 /* milliseconds */);
                    conn.setRequestMethod("GET");
                    conn.setDoInput(true);
                    conn.connect();
                 
                    stream = conn.getInputStream();
                    XmlPullParserFactory xmlFactoryObject = XmlPullParserFactory.newInstance();
                    XmlPullParser xmlparser = xmlFactoryObject.newPullParser();
                    
                    xmlparser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false);
                    xmlparser.setInput(stream, null);
                    
                    Log.d(TAG, "parse xml begin.");
                    String ProtectionHeader = null;
                    String text = null;
                    int event = xmlparser.getEventType();
                    while (event != XmlPullParser.END_DOCUMENT) {
                        String name = xmlparser.getName();
                        switch (event) {
                            case XmlPullParser.START_TAG:
                                if (name.equals("ProtectionHeader")) {
                                    ProtectionHeader = null;
                                }
                                break;

                            case XmlPullParser.TEXT:
                                text = xmlparser.getText();
                                break;

                            case XmlPullParser.END_TAG:
                                if (name.equals("ProtectionHeader")) {
                                    //Log.d(TAG, "ProtectionHeader:" + text);
                                    ProtectionHeader = text;
                                }
                                break;
                        }
                        event = xmlparser.next();
                    }
                    Log.d(TAG, "parse xml end.");
                    
                    // 2. base64decode wrmheader
                    if (ProtectionHeader != null) {
                        WRMHeader = new String(Base64.decode(ProtectionHeader, Base64.DEFAULT), "UTF-16LE");
                        WRMHeader = WRMHeader.substring(WRMHeader.indexOf("<WRMHEADER"));
                    }
                    Log.d(TAG, "WRMHeader=" + WRMHeader);
                    
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    if (stream != null) {
                        try {
                            stream.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
                
                // 3. ack and process license using wrmheader. 
                int ret = -1;
                if (WRMHeader != null) {
                    ret = ackAndProcessLicense(WRMHeader);
                }
                
                // 4. show text in screen.
                if (ret == 0) {
                    ShowText("license succeed.");
                } else {
                    ShowText("license failed.");
                }
            }
        });
        thread.start();
    }
}
