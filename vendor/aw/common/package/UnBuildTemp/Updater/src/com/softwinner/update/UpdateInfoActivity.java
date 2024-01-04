package com.softwinner.update;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.webkit.WebView;
import android.widget.ImageView;
import android.widget.TextView;

import com.lidroid.xutils.util.LogUtils;

/**
 * OTA更新说明显示
 * @author greatzhang
 *
 */
public class UpdateInfoActivity extends Activity
{

    private ImageView mBackBtn;
    private WebView mWebView;
    private TextView mUpdateTxt = null;

    private String mDescription; //描叙
    private String mUpdateRom;

    public static final String CONTENT_DESCRIPTION = "Description";
    public static final String ROM_VERSION = "rom_version";

    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
	// TODO Auto-generated method stub
	super.onCreate( savedInstanceState );
	requestWindowFeature( Window.FEATURE_NO_TITLE );
	setContentView( R.layout.update_info_activity );
	mDescription = getIntent( ).getStringExtra( CONTENT_DESCRIPTION );
	mUpdateRom = getIntent( ).getStringExtra( ROM_VERSION );

	initViews( );
    }

    private void initViews()
    {
	mBackBtn = (ImageView)findViewById( R.id.ud_back );
	mWebView = (WebView)findViewById( R.id.ud_webview );
	mUpdateTxt = (TextView)findViewById( R.id.update_log_txt );
	if( mUpdateRom != null && !mUpdateRom.equals( "UNKNOWN" ) && !mUpdateRom.equals( "" ) )
	{
	    String log = mUpdateRom + " " + getResources( ).getString( R.string.version_description_title );
	    LogUtils.i( "log = " + log );
	    mUpdateTxt.setText( log );
	}

	mWebView.loadDataWithBaseURL( null , mDescription , "text/html" , "utf-8" , null );

	mBackBtn.setOnClickListener( new OnClickListener( )
	{

	    @Override
	    public void onClick( View arg0 )
	    {
		UpdateInfoActivity.this.finish( );
	    }

	} );
    }

}
