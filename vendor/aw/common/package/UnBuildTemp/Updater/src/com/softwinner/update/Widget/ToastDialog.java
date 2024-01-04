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


public class ToastDialog extends Dialog
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

    private Button mRightBtn;
    private String mRightBtnTitle;

    private View.OnClickListener mLeftBtnListener;
    private View.OnClickListener mRightBtnListener;

    private View.OnClickListener mCheckBoxListener;

    private boolean b = false;
    
    private boolean mBackPressState = true;

    public ToastDialog( Context context )
    {
	super( context );
	// TODO Auto-generated constructor stub
    }

    /**
     * param context 上下文
     * param tip 提示信息
     * param b  是否显示checked 按钮
     */

    public ToastDialog( Context context , String tip , boolean b )
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
	setContentView( R.layout.toast_dialog );

	mTipTextView = (TextView)findViewById( R.id.tip );
	mTipTextView.setText( mToastTip );
	mTitleTextView = (TextView)findViewById( R.id.title );

	mIsToNoteBox = (CheckBox)findViewById( R.id.is_to_note );
	mCheckBoxLayout = (LinearLayout)findViewById( R.id.check_box_layout );
	mCheckBoxLayout.setVisibility( b ? View.VISIBLE : View.GONE );
	mIsToNoteBox.setOnClickListener( mCheckBoxListener == null ? mCheckBoxDefaultListener : mCheckBoxListener );

	mLeftBtn = (Button)findViewById( R.id.left );
	mLeftBtn.setText( mLeftBtnTitle == null ? mContext.getString( R.string.more ) : mLeftBtnTitle );
	mRightBtn = (Button)findViewById( R.id.right );
	mRightBtn.setText( mRightBtnTitle == null ? mContext.getString( R.string.cancel ) : mRightBtnTitle );
	mLeftBtn.setOnClickListener( mLeftBtnListener == null ? mLeftBtnDefautListener : mLeftBtnListener );
	mRightBtn.setOnClickListener( mRightBtnListener == null ? mRightBtnDefautListener : mRightBtnListener );

    }

    private View.OnClickListener mLeftBtnDefautListener = new View.OnClickListener( )
    {

	@Override
	public void onClick( View v )
	{
	    dismiss( );

	}
    };

    private View.OnClickListener mRightBtnDefautListener = new View.OnClickListener( )
    {

	@Override
	public void onClick( View v )
	{
	    dismiss( );
	}
    };

    private View.OnClickListener mCheckBoxDefaultListener = new View.OnClickListener( )
    {

	@Override
	public void onClick( View v )
	{
	    if( !mIsToNoteBox.isChecked( ) )
	    {
		mIsToNoteBox.setChecked( false );
	    }
	    else
	    {
		mIsToNoteBox.setChecked( true );
	    }
	}
    };

    public void addLeftBtnListener( View.OnClickListener listener )
    {
	this.mLeftBtnListener = listener;
    }

    public void addRightBtnListener( View.OnClickListener listener )
    {
	this.mRightBtnListener = listener;
    }

    public void addCheckBoxListener( View.OnClickListener listener )
    {
	this.mCheckBoxListener = listener;
    }

    public void setmTitle( String mTitle )
    {
	this.mTitle = mTitle;
    }

    public void setmLeftBtnTitle( String mLeftBtnTitle )
    {
	this.mLeftBtnTitle = mLeftBtnTitle;
    }

    public void setmRightBtnTitle( String mRightBtnTitle )
    {
	this.mRightBtnTitle = mRightBtnTitle;
    }
    public void setCustomBacPressed(boolean state){
    	mBackPressState = state;
    }
    public void setButtonVisabilty(boolean leftOrRight,int visabilty){
    	if(leftOrRight)
    		mLeftBtn.setVisibility(visabilty);
    	else
    		mRightBtn.setVisibility(visabilty);
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
