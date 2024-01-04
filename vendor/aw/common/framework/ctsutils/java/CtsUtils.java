/*************************************************************************
    > File Name: CtsUtils.java
    > Author: chenjuncong
    > Mail: chenjuncong@allwinnertech.com
    > Created Time: 2018年01月15日 星期一 19时37分28秒
 ************************************************************************/
package com.softwinner.cts;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.lang.StringBuilder;
import java.io.InputStreamReader;

import android.content.Context;

public class CtsUtils{
    Context mContext;
    public CtsUtils(Context context){
        mContext = context;
    }

    public static String getStaticMethodProcessName(int pid){
        StringBuilder processName = new StringBuilder();
        BufferedReader cmdlineReader = null;
        try {

            cmdlineReader = new BufferedReader(new InputStreamReader(
                        new FileInputStream(
                            "/proc/" + pid + "/cmdline"),
                        "iso-8859-1"));
            int c;
            while ((c = cmdlineReader.read()) > 0) {

                processName.append((char) c);
            }
        }
        catch(Exception e){
            e.printStackTrace();
        }
        finally {
            if (cmdlineReader != null) {
                try{
                    cmdlineReader.close();
                }
                catch(Exception e){
                    e.printStackTrace();
                }
            }
        }
        if(processName != null){
            return processName.toString();
        }
        else{
            return "";
        }
    }

    public String getMethodProcessNameByUidWithContext(int uid){
        String callingApp = mContext.getPackageManager().getNameForUid(uid);
        return callingApp;
    }

}
