
package com.example.testad;

import android.widget.Button;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import com.softwinner.AdManager;
import android.util.Log;
import android.net.Uri;
import android.content.Intent;
import android.content.ActivityNotFoundException;

public class MainActivity extends Activity  {

    /**
     * The {@link android.support.v4.view.PagerAdapter} that will provide
     * fragments for each of the sections. We use a
     * {@link android.support.v4.app.FragmentPagerAdapter} derivative, which
     * will keep every loaded fragment in memory. If this becomes too memory
     * intensive, it may be best to switch to a
     * {@link android.support.v4.app.FragmentStatePagerAdapter}.
     */

    Button logoButton;
    Button animationButton;
    TextView text;

    int REQUEST_LOGO=0;
    int REQUEST_ANIMATION=1;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        logoButton=(Button) findViewById(R.id.buttonSetLogo);
        animationButton=(Button) findViewById(R.id.buttonSetAnimation);
        logoButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Intent i = new Intent("com.softwinner.action.GET_FILE");
                try {
                    startActivityForResult(i, REQUEST_LOGO);
                } catch (ActivityNotFoundException e) {
                    e.printStackTrace();
                    Toast.makeText(MainActivity.this, "fail activitynotfount exception", Toast.LENGTH_SHORT).show();
                } catch (SecurityException e) {
                    e.printStackTrace();
                    Toast.makeText(MainActivity.this, "fail secureity exception", Toast.LENGTH_SHORT).show();
                }
            }
        });
        animationButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Intent i = new Intent("com.softwinner.action.GET_FILE");
                try {
                    startActivityForResult(i, REQUEST_ANIMATION);
                } catch (ActivityNotFoundException e) {
                    e.printStackTrace();
                    Toast.makeText(MainActivity.this, "fail activitynotfount exception", Toast.LENGTH_SHORT).show();
                } catch (SecurityException e) {
                    e.printStackTrace();
                    Toast.makeText(MainActivity.this, "fail secureity exception", Toast.LENGTH_SHORT).show();
                }
            }
        });
        AdManager.setBootLogo("/sdcard/bootlogo.bmp");
     }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == REQUEST_LOGO && resultCode == RESULT_OK){
            Uri uri = data.getData();
            if(uri != null && uri.getScheme().equals("file") ){
                String path=uri.getPath();
                //if (path.startsWith("/storage/emulated/0"))
                //{
                //    path=path.replace("/storage/emulated/0","/storage/emulated/legacy");
                //}
                AdManager.setBootLogo(path);
                Toast.makeText(MainActivity.this, "set BootLogo", Toast.LENGTH_SHORT).show();
            }
        }
        if(requestCode == REQUEST_ANIMATION && resultCode == RESULT_OK){
            Uri uri = data.getData();
            if(uri != null && uri.getScheme().equals("file") ){
                String path=uri.getPath();
                if (path.startsWith("/storage/emulated/0"))
                    path=path.replaceAll("/storage/emulated/0","/storage/emulated/legacy");
                AdManager.setBootAnimation(path);
                Toast.makeText(MainActivity.this, "set BootAnimation", Toast.LENGTH_SHORT).show();
            }
        }
    }

}
