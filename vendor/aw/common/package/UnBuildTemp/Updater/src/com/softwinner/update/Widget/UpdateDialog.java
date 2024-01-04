package com.softwinner.update.Widget;

import com.softwinner.update.R;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;


public class UpdateDialog extends Dialog
{

    private Context mContext;
    private TextView mTipTextView;
    private TextView mTitleTextView;
    private CheckBox mIsToNoteBox;
    private LinearLayout mCheckBoxLayout;

    //设置title
    private String mTitle;
    private String mToastTip;
    private Button mLeftBtn;
    private String mLeftBtnTitle;

    private View.OnClickListener mLeftBtnListener;

    private View.OnClickListener mCheckBoxListener;

    private boolean b = false;
    
    private boolean mBackPressState = true;

    public UpdateDialog( Context context )
    {
	super( context );
	// TODO Auto-generated constructor stub
    }

    /**
     * param context 上下文
     * param tip 提示信息
     * param b  是否显示checked 按钮
     */

    public UpdateDialog( Context context , String tip , boolean b )
    {
	super( context , R.style.MyDialog );
	this.mContext = context;
	this.mToastTip = tip;
	this.b = b;
    }

    public void setTitle( String title )
    {
	if( title != null && !"".equals( title ) )
	{
	    mTitle = title;
	    mTipTextView.setText( mTitle );
	    mTitleTextView.setVisibility( View.VISIBLE );
	}
	else
	{
	    mTitleTextView.setVisibility( View.GONE );
	}
    }

    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
	super.onCreate( savedInstanceState );
	setContentView( R.layout.update_dialog );

	mTipTextView = (TextView)findViewById( R.id.tip );
	mTipTextView.setText( mToastTip );
	mTitleTextView = (TextView)findViewById( R.id.title );

    }

    public void setmTitle( String mTitle )
    {
	this.mTitle = mTitle;
    }
    public void setCustomBacPressed(boolean state){
    	mBackPressState = state;
    }
    @Override
    public void onBackPressed()
    {
	if( isShowing( ) && mBackPressState)
	{
	    return;
	}
	super.onBackPressed( );
    }

}
