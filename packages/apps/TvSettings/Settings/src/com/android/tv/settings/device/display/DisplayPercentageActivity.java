/*************************************************************************
    > File Name: DisplayPercentageActivity.java
    > Author: bill
    > Mail: cjcbill@gmail.com
    > Created Time: 2019年11月29日 星期五 10时04分16秒
 ************************************************************************/

package com.android.tv.settings.device.display;
import com.android.tv.settings.R;
import android.os.Bundle;
import android.view.KeyEvent;
import android.util.Log;
import android.app.Activity;
import android.widget.ImageView;
import android.os.Handler;
import android.os.Message;

public class DisplayPercentageActivity extends Activity{
    private static final boolean DEBUG = true;
    private static final String TAG = "HdmiPercentageActivity";
    private DisplayOutputImpl mDisplayOutputImpl;
    private int horizontalValue;
    private int verticalValue;
    private final int MAX_VALUE = 100;
    private final int MIN_VALUE = 80;
    private static final int HANDLE_PRESS_LEFT = 0;
    private static final int HANDLE_PRESS_RIGHT = 1;
    private static final int HANDLE_PRESS_UP = 2;
    private static final int HANDLE_PRESS_DOWN = 3;
    private static final int HANDLE_PRESS_NONE = 4;
    private ImageView mImageView;

    @Override
    protected void onCreate(Bundle savedInstanceState){
        if(DEBUG){
            Log.d(TAG,"onCreate!");
        }
        super.onCreate(savedInstanceState);
        setContentView(R.layout.display_percentage);
        mDisplayOutputImpl = new DisplayOutputImpl();
        mImageView = (ImageView)findViewById(R.id.direction_key);
    }

    @Override
    protected void onResume(){
        if(DEBUG){
            Log.d(TAG,"onResume!");
        }
        super.onResume();
        int type = getIntent().getIntExtra("DISPLAY_TYPE", 0);
        if(type == DisplayOutputImpl.TYPE_HDMI){
            mDisplayOutputImpl.updateDisplayDevice(DisplayOutputImpl.TYPE_HDMI);
        }
        else if(type == DisplayOutputImpl.TYPE_CVBS){
            mDisplayOutputImpl.updateDisplayDevice(DisplayOutputImpl.TYPE_CVBS);
        }
        initValue();
    }

    private void initValue(){
        horizontalValue = mDisplayOutputImpl.getDisplayHorizontalPercent();
        verticalValue = mDisplayOutputImpl.getDisplayVerticalPercent();;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(DEBUG){
            Log.d(TAG,"horizonVale = " + horizontalValue + " verticalValue = " + verticalValue);
        }
        switch(keyCode){
            case KeyEvent.KEYCODE_DPAD_LEFT:
                if(DEBUG){
                    Log.d(TAG,"LEFT!");
                }
                if(horizontalValue > MIN_VALUE && horizontalValue <= MAX_VALUE){
                    mDisplayOutputImpl.setDisplayMarginHorizontalPercent(--horizontalValue);
                    findViewById(R.id.display_percentage).invalidate();
                }
                sendDirectMessage(HANDLE_PRESS_LEFT);
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                if(DEBUG){
                    Log.d(TAG,"RIGHT!");
                }
                if(horizontalValue >= MIN_VALUE && horizontalValue < MAX_VALUE){
                    mDisplayOutputImpl.setDisplayMarginHorizontalPercent(++horizontalValue);
                    findViewById(R.id.display_percentage).invalidate();
                }
                sendDirectMessage(HANDLE_PRESS_RIGHT);
                break;
            case KeyEvent.KEYCODE_DPAD_UP:
                if(DEBUG){
                    Log.d(TAG,"UP!");
                }
                if(verticalValue >= MIN_VALUE && verticalValue < MAX_VALUE){
                    mDisplayOutputImpl.setDisplayMarginVerticalPercent(++verticalValue);
                    findViewById(R.id.display_percentage).invalidate();
                }
                sendDirectMessage(HANDLE_PRESS_UP);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                if(DEBUG){
                    Log.d(TAG,"DOWN!");
                }
                if(verticalValue > MIN_VALUE && verticalValue <= MAX_VALUE){
                    mDisplayOutputImpl.setDisplayMarginVerticalPercent(--verticalValue);
                    findViewById(R.id.display_percentage).invalidate();
                }
                sendDirectMessage(HANDLE_PRESS_DOWN);
                break;
            default:
                return super.onKeyDown(keyCode, event);
        }
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch(keyCode){
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
                sendDelayedMessage(HANDLE_PRESS_NONE);
                break;
            default:
                return super.onKeyUp(keyCode, event);
        }
        return true;
    }

    private Handler mHandler = new Handler(){
        public void handleMessage(Message msg){
            switch(msg.what){
                case HANDLE_PRESS_LEFT:
                    mImageView.setImageDrawable(getResources().getDrawable(R.drawable.four_direction_left));
                    break;
                case HANDLE_PRESS_RIGHT:
                    mImageView.setImageDrawable(getResources().getDrawable(R.drawable.four_direction_right));
                    break;
                case HANDLE_PRESS_UP:
                    mImageView.setImageDrawable(getResources().getDrawable(R.drawable.four_direction_up));
                    break;
                case HANDLE_PRESS_DOWN:
                    mImageView.setImageDrawable(getResources().getDrawable(R.drawable.four_direction_down));
                    break;
                case HANDLE_PRESS_NONE:
                    mImageView.setImageDrawable(getResources().getDrawable(R.drawable.four_direction));
                    break;
            }
        }
    };

    private void sendDirectMessage(int msgType){
        Message message = new Message();
        message.what = msgType;
        mHandler.sendMessage(message);
    }
    private void sendDelayedMessage(int msgType){
        Message message = new Message();
        message.what = msgType;
        mHandler.sendMessageDelayed(message, 300);
    }
}
