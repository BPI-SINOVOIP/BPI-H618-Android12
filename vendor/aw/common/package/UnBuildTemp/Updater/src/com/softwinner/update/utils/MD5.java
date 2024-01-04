package com.softwinner.update.utils;

import java.io.FileInputStream;
import java.io.InputStream;
import java.security.MessageDigest;

/**
 * md5类，支持直接对文件生成md5
 * @author hendysu
 *
 */
public final class MD5
{
    private final static char[] HEX_DIGITS = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    public static String toHexString(byte[] b) {
    	
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (int i = 0; i < b.length; i++) {
            sb.append(HEX_DIGITS[(b[i] & 0xf0) >>> 4]);
            sb.append(HEX_DIGITS[b[i] & 0x0f]);
        }
        return sb.toString();
    }

    public final static byte[] getMD5Bytes(byte[] source)
    {
        try
        {
            MessageDigest messageDigest = MessageDigest.getInstance("MD5");
            messageDigest.update(source);
            return (messageDigest.digest());
        }
        catch (Exception e)
        {
            return null;
        }
    }

    public final static byte[] getMD5Bytes(String source)
    {
        return getMD5Bytes(source.getBytes());
    }

    public final static String getMD5String(byte[] source)
    {
        byte[] md = getMD5Bytes(source);
        if (md != null)
        {
            return toHexString(md);
        }
        else
        {
            return null;
        }
    }

    public final static String getMD5String(String source)
    {
        return getMD5String(source.getBytes());
    }
    
    public final static String getMD5StringForFile(String filePath)
    {    	
        InputStream fis;
        byte[] buffer = new byte[1024];
        int numRead = 0;
        MessageDigest md5;
        try{
            fis = new FileInputStream(filePath);
            md5 = MessageDigest.getInstance("MD5");
            while((numRead=fis.read(buffer)) > 0) {
                md5.update(buffer,0,numRead);
            }
            fis.close();
            return toHexString(md5.digest());   
        } catch (Exception e) {
        	e.printStackTrace();
            return null;
        }
    }
}