/*
 * Copyright (c) 2011-2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
package com.allwinnertech.socs;

import static android.location.LocationManager.GPS_PROVIDER;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.R.drawable;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.Criteria;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class GPSTest extends Activity {

    String TAG = "GPS";
    private Context mContext;
    TextView mTextView;
    Button stopButton;
    EditText mEditText;
    ListView mListView = null;
    private Location location;
    LayoutInflater mInflater = null;
    LocationManager mLocationManager = null;
    final int OUT_TIME = 20 * 1000;
    final int MIN_SAT_NUM = 1;
    private String testname;
    private boolean isPass = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {

        mContext = this;
        mInflater = LayoutInflater.from(mContext);
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        setContentView(R.layout.gps);
        getService();
        bindView();
        mCountDownTimer.start();
    }

    CountDownTimer mCountDownTimer = new CountDownTimer(OUT_TIME, 3000) {

        @Override
        public void onTick(long arg0) {

        }

        @Override
        public void onFinish() {

            fail("测试超时");
        }
    };

    CountDownTimer mShowTimer = new CountDownTimer(5 * 1000, 5 * 1000) {

        @Override
        public void onTick(long arg0) {

        }

        @Override
        public void onFinish() {
            finishTest();
        }
    };

    void startGPS() {

        if (!mLocationManager.isProviderEnabled(android.location.LocationManager.GPS_PROVIDER)) {
            toast(getString(R.string.gps_enable_first));
            Intent intent = new Intent();

            // Entry the "LocationSettingsActivity" to turn
            // on the "GPS" setting.
            intent.setClassName("com.android.settings",
                    "com.android.settings.Settings$LocationSettingsActivity");
            //startActivity(intent);
            //return;
        }
        Criteria criteria;
        criteria = new Criteria();
        criteria.setAccuracy(Criteria.ACCURACY_FINE);
        criteria.setAltitudeRequired(false);
        criteria.setBearingRequired(false);
        criteria.setCostAllowed(true);
        criteria.setPowerRequirement(Criteria.POWER_LOW);
        
        String provider = mLocationManager.getBestProvider(criteria, true);
        if (provider == null || !provider.equals(GPS_PROVIDER)) {
            fail("Fail to get GPS Provider!");
            return;
        }
        try{
            mLocationManager.requestLocationUpdates(provider, 500, 0, mLocationListener);
            mLocationManager.addGpsStatusListener(gpsStatusListener);
        }catch(Exception e){
            e.printStackTrace();
        }

        location = mLocationManager.getLastKnownLocation(provider);
        setLocationView(location);
    }

    @Override
    protected void onPause() {

        stopGPS();
        super.onPause();
    }

    private void setLocationView(Location location) {

        if (location != null) {
            double latitude = location.getLatitude();
            double longitude = location.getLongitude();
            double speed = location.getSpeed();
            double altitude = location.getAltitude();
            double bearing = location.getBearing();
            mTextView.setText("latitude:" + latitude + '\n' + "longitude:" + longitude + '\n'
                    + "speed:" + speed + "m/s" + '\n' + "altitude:" + altitude + "m" + '\n'
                    + "bearing:" + bearing + '\n');
        } else {
            mTextView.setText("Location unknown or locating...");
        }
    }

    LocationListener mLocationListener = new LocationListener() {

        public void onLocationChanged(Location location) {

            setLocationView(location);
            pass();
        }

        public void onProviderDisabled(String provider) {

            setLocationView(null);
        }

        public void onProviderEnabled(String provider) {

            toast("Provider enabled");

        }

        public void onStatusChanged(String provider, int status, Bundle extras) {

        }
    };

    private GpsStatus mGpsStatus;
    private Iterable<GpsSatellite> mSatellites;
    List<String> satelliteList = new ArrayList<String>();
    GpsStatus.Listener gpsStatusListener = new GpsStatus.Listener() {

        public void onGpsStatusChanged(int arg0) {

            switch (arg0)
            {
            case GpsStatus.GPS_EVENT_STARTED:
                toast("GPS Start");
                setProgressBarIndeterminateVisibility(true);
                break;
            case GpsStatus.GPS_EVENT_STOPPED:
                // toast("GPS Stop");
                setProgressBarIndeterminateVisibility(false);
                break;
            case GpsStatus.GPS_EVENT_FIRST_FIX:
                toast("Locate sucess");
                pass();
                break;
            case GpsStatus.GPS_EVENT_SATELLITE_STATUS:
                mGpsStatus = mLocationManager.getGpsStatus(null);
                mSatellites = mGpsStatus.getSatellites();
                Iterator<GpsSatellite> it = mSatellites.iterator();
                int count = 0;
                satelliteList.clear();
                while (it.hasNext()) {
                    GpsSatellite gpsS = (GpsSatellite) it.next();
                    satelliteList.add(count++, "Prn" + gpsS.getPrn() + " Snr:" + gpsS.getSnr());
                }
                updateAdapter();
                if (count >= MIN_SAT_NUM)
                    pass();
                break;
            default:
                break;
            }

        }

    };

    public void updateAdapter() {

        mAdapter.notifyDataSetChanged();
    }

    void stopGPS() {

        try {
            mLocationManager.removeUpdates(mLocationListener);
            mLocationManager.removeGpsStatusListener(gpsStatusListener);
            setProgressBarIndeterminateVisibility(true);
        } catch (Exception e) {
            android.util.Log.d(TAG, "stopGPS fail");
            e.printStackTrace();
        }
    }

    void bindView() {
        mTextView = (TextView) findViewById(R.id.gps_hint);
        mListView = (ListView) findViewById(R.id.gps_list);
        mListView.setAdapter(mAdapter);
        registerForContextMenu(mListView);

    }

    private void finishTest() {
        finish();
    }

    @Override
    public void finish() {
        mShowTimer.cancel();
        stopGPS();
        Intent returnIntend =new Intent();
        setResult(RESULT_OK,returnIntend);
        super.finish();
    };

    void getService() {

        mLocationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        if (mLocationManager == null) {
            fail("Fail to get LOCATION_SERVICE!");
        }
    }

    @Override
    protected void onDestroy() {

        if (mCountDownTimer != null)
            mCountDownTimer.cancel();
        super.onDestroy();
    }

    protected void onResume() {

        startGPS();
        super.onResume();
    };

    BaseAdapter mAdapter = new BaseAdapter() {

        public Object getItem(int arg0) {

            return null;
        }

        public long getItemId(int arg0) {

            return 0;
        }

        public View getView(int index, View convertView, ViewGroup parent) {

            if (convertView == null)
                convertView = mInflater.inflate(R.layout.gps_item, null);
            TextView mText = (TextView) convertView.findViewById(R.id.gps_text);
            ImageView mImage = (ImageView) convertView.findViewById(R.id.gps_image);
            mText.setText(satelliteList.get(index));
            mImage.setImageResource(drawable.presence_online);
            return convertView;
        }

        public int getCount() {

            if (satelliteList != null)
                return satelliteList.size();
            else
                return 0;
        }

    };

    void fail(String msg) {
        mCountDownTimer.cancel();
        toast(msg);
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle("测试失败");
        builder.setMessage(msg);
        builder.setPositiveButton("确定", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            }
        });
        builder.create().show();
    }

    void pass() {
        if (isPass)
            return;
        isPass = true;
        mCountDownTimer.cancel();
        mShowTimer.start();
    }

    public void toast(Object s) {

        if (s == null)
            return;
        Toast.makeText(this, s + "", Toast.LENGTH_SHORT).show();
    }
}
