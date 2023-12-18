package com.softwinner.update;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import com.bigbigcloud.devicehive.api.BbcDevice;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.UpdateInfoResponse;
import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.service.BbcDeviceService;
import com.bigbigcloud.devicehive.utils.Utils;
import com.google.gson.JsonObject;

/**
 * Created by Administrator on 2016/4/17.
 */
public class OtaService extends BbcDeviceService {
    private final String TAG = "OtaService";
    private static final String ACTION_CHECK = "check_for_update";
    private static final long CHECK_INTERNAL = 5 * 24 * 60 * 60 * 1000;

    private NotificationManager notificationManager;

    @Override
    public void onCreate() {
        super.onCreate();
        notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        Log.d(TAG, "===========onCreate============= ");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "===========onStartCommand============= ");
        if (intent != null) {
            String action = intent.getAction();
            if (action != null && action.equals(ACTION_CHECK)) {
                Log.d(TAG, "timer check for update");
                if (Utils.isNetworkConnected(this)) {
                    if (bbcDevice != null) {
                        checkUpdate();
                    }
                }
            }
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected boolean isRealtimeConnection() {
        return false;
    }

    @Override
    protected boolean isWebsocketPrefered() {
        return false;
    }

    @Override
    protected void onBbcDeviceReady(BbcDevice bbcDevice) {

    }

    @Override
    protected Device getDeviceInfo() {
        return UpdateDevice.getInstance(this);
    }

    public void onUpdateInfo(UpdateInfoResponse info) {
        long nextQueryTime = System.currentTimeMillis() + CHECK_INTERNAL + (long) (Math.random() * 60 * 1000);
/*        if(otaPersistData.getNextQueryTime() <= System.currentTimeMillis()){
        AlarmManager alarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        Intent i = new Intent(this, OtaService.class).setAction(ACTION_CHECK);
        PendingIntent pi = PendingIntent.getService(this, 0, new Intent(getApplicationContext(), OtaService.class), 0);
        alarmManager.set(AlarmManager.RTC_WAKEUP, nextQueryTime, pi); //设置服务器建议的下次查询时间
            otaPersistData.setNextQueryTime(nextQueryTime);//记录设置的下次定时查询的时间，在此定时时间之前，不再设置定时查询
        }*/
        if (info == null) {
            return;
        }
        Intent intent = new Intent(this, UpdateActivity.class).putExtra("updateInfo", info);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this)
                .setContentTitle("系统更新！！")
                .setContentText("发现新版本：" + info.getNewVersion())
                .setAutoCancel(true)
                .setWhen(System.currentTimeMillis())
                .setTicker("系统更新通知！！")
                .setDefaults(Notification.DEFAULT_SOUND)
//                .setSmallIcon(R.mipmap.ic_launcher)
                .setStyle(new NotificationCompat.BigTextStyle()
                        .bigText(info.toString())
                        .setBigContentTitle("更新详细信息"))
                .setContentIntent(pendingIntent);
        notificationManager.notify(1, builder.build());

        Log.d(TAG, "on update info , send notification， next query time " + nextQueryTime);
    }

    private void checkUpdate() {
        bbcDevice.inquireForUpdate(property.getDeviceGuid(), device, new IUiCallback() {
            @Override
            public void onSuccess(JsonObject data) {
                Log.d(TAG, "update info " + data.toString());
                if (data != null && !data.isJsonNull()) {
                    UpdateInfoResponse infoResponse = GsonFactory.createGson().fromJson(data, UpdateInfoResponse.class);
                    onUpdateInfo(infoResponse);
                }
            }

            @Override
            public void onError(int errorCode, String message) {
                Log.d(TAG, " update error " + message);
            }

            @Override
            public void onCancel() {

            }
        });
    }

}
