package com.softwinner.update.utils;

import java.text.DecimalFormat;

public class StringUtils
{

    private static DecimalFormat SIZ_DF = new DecimalFormat( "###.00" );
    private final static String [] UNITS = new String [] { "GB" , "MB" , "KB" , "B" };
    private final static long [] SIZE_DIVIDERS = new long [] { 1024 * 1024 * 1024 , 1024 * 1024 , 1024 , 1 };

    public static String byteToString( final long size )
    {
	if( size < 1 )
	    return "0B";
	String res = null;
	for( int i = 0 ; i < SIZE_DIVIDERS.length ; i++ )
	{
	    final long divider = SIZE_DIVIDERS[i];
	    if( size >= divider )
	    {
		res = format( size , divider , UNITS[i] );
		break;
	    }
	}
	return res;
    }

    private static String format( final long value , final long divider , final String unit )
    {
	final double result = divider > 1 ? (double)value / (double)divider : (double)value;
	String str = SIZ_DF.format( result ) + unit;
	return str;
    }

    public static String getOriginalUrl( String url )
    {
	String URL = "";
	int index = url.indexOf( "?" );
	if( index > 0 )
	{
	    URL = url.substring( index );
	}
	System.out.println( "------ originalUrl = " + url );
	return URL;
    }

}
