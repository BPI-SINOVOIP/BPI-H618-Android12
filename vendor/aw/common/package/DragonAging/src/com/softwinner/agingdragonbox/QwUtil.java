package com.softwinner.agingdragonbox;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.ArrayList;

import android.R.string;
import android.util.Log;

public class QwUtil {
	private static final String TAG = "DragonBox-QwUtils";
	public static final String executeQw(String... commands) {
		String result = null;
		try {
			if (null != commands && commands.length > 0) {
				Process process = Runtime.getRuntime().exec("qw");
				DataOutputStream os = new DataOutputStream(process.getOutputStream());
				for (String currCommand : commands) {
					os.writeBytes(currCommand + "\n");
					//
				}
				os.writeBytes("exit\n");
				os.flush();
				
				BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
				int read;
				char[] buffer = new char[4096];
				StringBuffer output = new StringBuffer();
				while ((read = reader.read(buffer)) > 0) {
					output.append(buffer, 0, read);
				}
				reader.close();
				result = output.toString();
			}
		} catch (Exception ex) {
			ex.printStackTrace();
		}
		return result;
	}

	public static boolean haveRoot() {
		boolean ret = execQwCmdSilent("echo test"); // 通过执行测试命令来检测
		if (ret) {
			Log.i(TAG, "have qw!");
		} else {
			Log.i(TAG, "not have qw!");
		}
		return ret;
	}
	
	// 执行命令但不关注结果输出 
    public static boolean execQwCmdSilent(String cmd) { 
    	Log.e(TAG,"execQwCmdSilent: "+cmd);
    	boolean result = false;
    	Process proc = null;
    	BufferedWriter bufferedWriter = null;
    	 try {
    		 proc = Runtime.getRuntime().exec("qw");
             bufferedWriter = new BufferedWriter(new OutputStreamWriter(proc.getOutputStream()));
             bufferedWriter.write(cmd+"\n");
             bufferedWriter.flush();
             bufferedWriter.write("exit\n");
             bufferedWriter.flush();
             result = (proc.waitFor()==0);
         } catch (Exception e) {
             e.printStackTrace();
         } finally {
             if (proc != null) {
                 Log.e(TAG,"Proc destroy");
                 proc.destroy();
                 proc = null;
             }
             if (bufferedWriter != null) {
                 try {
                	 bufferedWriter.close();
                	 bufferedWriter = null;
                 } catch (IOException e) {
                     e.printStackTrace();
                 }
             }

         }
    	 return result;
     } 	
    
    public static boolean exec(String cmd) {
    	Log.w(TAG,"exec: "+cmd);
    	boolean result = false;
    	Process proc = null;
    	  try {
              proc = Runtime.getRuntime().exec(cmd);
              result = (proc.waitFor()==0);
              Log.w(TAG,"exec: "+cmd+" result is "+result);
          } catch (Exception e) {
              e.printStackTrace();
          }finally {
        	  if (proc != null) {
                  Log.e(TAG,"Proc destroy");
                  proc.destroy();
                  proc = null;
              }
          }
    	  return result;
    }

}
