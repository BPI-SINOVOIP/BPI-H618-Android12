package com.softwinner.update.Widget;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.softwinner.update.R;

/*
使用方法标准Android的Toast一样Toast的时候调用makeUText就可以。
UToast.makeUText(getApplicationContext(),R.string.confirm_delete_contacts,UToast.LENGTH_SHORT).show();
*/

public class UToast extends Toast
{
    Context mContext;
    static String mText = null;

    public UToast( Context context )
    {
	super( context );
	mContext = context;
    }

    @Override
    public void setText( CharSequence s )
    {
	mText = s.toString( );
    }

    @Override
    public void setText( int resId )
    {
	mText = mContext.getString( resId );
    }

    public void show()
    {
	View layout = LayoutInflater.from( mContext ).inflate( R.layout.utoast , null );
	TextView title = (TextView)layout.findViewById( R.id.UToastText );
	title.setText( mText );
	setView( layout );
	super.show( );
    }

    public static UToast makeUText( Context context , String str , int duration )
    {
	mText = str;
	UToast result = new UToast( context );
	result.setText( str );
	result.setDuration( duration );
	return result;
    }

    public static UToast makeUText( Context context , int strID , int duration )
    {
	return makeUText( context , context.getString( strID ) , duration );
    }
}
