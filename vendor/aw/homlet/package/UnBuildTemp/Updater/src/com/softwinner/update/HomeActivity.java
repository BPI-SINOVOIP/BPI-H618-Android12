package com.softwinner.update;


import com.softwinner.update.R.layout;
import com.softwinner.update.Widget.TextImageButton;
import com.softwinner.update.Widget.UToast;

import android.os.Bundle;
import android.os.IBinder;
import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.View;

public class HomeActivity extends Activity implements View.OnClickListener {

	private static final String TAG = "HomeActivity";
	
    private ServiceConnection mConn = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            //mUIController.setBinder(service);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {

        }
    };
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.home_activity);
		TextImageButton tb_local = (TextImageButton) this.findViewById(R.id.imageButton_local);
		tb_local.setOnClickListener(this);
		TextImageButton tb_online = (TextImageButton) this.findViewById(R.id.imageButton_online);
		tb_online.setOnClickListener(this);
		
	}

	@Override
	public void onClick(View arg0) {
		// TODO Auto-generated method stub
		Log.d(TAG,"onClick " + arg0.getId());
		switch(arg0.getId()){
		case R.id.imageButton_local:
			Log.d(TAG,"Local Update Clicked");
			startLocalRecovery();
			break;
		case R.id.imageButton_online:
			Log.d(TAG,"Online Update Clicked");
			startOnlineRecovery();
			break;
		default:
			break;
		}
	}

	private void startOnlineRecovery(){
    	Intent bdIntent = new Intent();
    	bdIntent.setClass(HomeActivity.this, UpdateActivity.class);
        startActivity(bdIntent);		
	}
	
    public boolean checkPackage(String packageName) {  
        if (packageName == null || "".equals(packageName))  
            return false;  
        try {  
            ApplicationInfo info = getPackageManager().getApplicationInfo(  
                    packageName, PackageManager.GET_UNINSTALLED_PACKAGES);  
            return true;  
        } catch (NameNotFoundException e) {  
            return false;  
        }  
    }  
    
	private void startLocalRecovery(){
		if(checkPackage("com.softwinner.settingsassist")&&checkPackage("com.softwinner.TvdFileManager")){
			Log.d(TAG,"apk exist!");
			Intent intent = new Intent("softwinner.intent.action.RECOVREY");
			startActivity(intent);
		}else{
		    //显示Toast
			UToast.makeUText( this , getString( R.string.not_support_local_update ) , UToast.LENGTH_SHORT ).show( );

		}
	}

}
