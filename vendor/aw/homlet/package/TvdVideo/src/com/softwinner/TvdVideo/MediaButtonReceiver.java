package com.softwinner.TvdVideo;

import android.accounts.NetworkErrorException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.KeyEvent;

//这个类还必须静态注册，然后在AudioManager中注册才行
public class MediaButtonReceiver extends BroadcastReceiver{
    private static String TAG = "MediaButtonReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        String intentAction = intent.getAction();
        KeyEvent keyEvent = (KeyEvent)intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
        if(keyEvent != null){
            Log.v(TAG, "onReceive intentAction="+intentAction+",keyEvent="+keyEvent.toString());
            int keyCode = keyEvent.getKeyCode(); //键值
            int keyAction = keyEvent.getAction(); //按下或弹起的动作
            // long downtime = keyEvent.getEventTime(); //获取事件的时间

            if(Intent.ACTION_MEDIA_BUTTON.equals(intentAction) && keyAction == KeyEvent.ACTION_DOWN){
                Log.v(TAG, "onReceive sendBroadcast to TvdVideoActivity");
                Intent intent2 = new Intent("android.intent.action.MEDIABUTTON_BROADCAST");
                intent2.putExtra("keycode",keyCode);
                context.sendBroadcast(intent2);
            }
        }
    }
}
