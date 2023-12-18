package com.allwinnertech.socs;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.BufferedReader;

import android.R.string;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;


public class ChipidInfo {


    private final String cpid_file = "/sys/class/sunxi_info/sys_info";
    private final String TAG = "MainActivity";






    /*
     *                SUNXI                          ALLWINNER

               |-- sun4i                        |-- cortex-a8
               |    |--sun4iw1                  |    |--1623
               |    |     |--sun4iw1p1          |    |     |---A10
               |    |--sun4iw2                  |    |--1625
               |          |--sun4iw2p1          |          |---A13
               |          |--sun4iw2p2          |          |---A12
               |          |--sun4iw2p3          |          |---A10S
               |                                |
               |-- sun8i                        |-- cortex-a7-smp
               |    |--sun8iw1                  |    |--1633
               |    |     |--sun8iw1p1          |    |     |---A31
               |    |     |--sun8iw1p2          |    |     |---A31S
               |    |--sun8iw2                  |    |--1651
               |    |     |--sun8iw2p1          |    |     |---A20
               |    |     |--sun8iw2p2          |    |     |---A2X
               |    |--sun8iw3                  |    |--1650
               |    |     |--sun8iw3p1          |    |     |---A23
               |    |     |--sun8iw3p2          |    |     |---AXX
               |    |--sun8iw5                  |    |--1667
               |    |     |--sun8iw5p1          |    |     |---A33
               |    |     |--sun8iw5p2          |    |     |---AXX
               |    |--sun8iw6                  |    |--1673
               |    |     |--sun8iw6p1          |    |     |---A83
               |    |     |--sun8iw6p2          |    |     |---AXX
                                                                    Aw1673的chipid为ID序列的bit[15:10]表示，其中：
                                                                    芯片    合法chip id [15:10]
                                                                    H8    0x0、0x03
                                                                    A83T    0x01
                                                                    R58    0x04、0x05
                                                                    T8    0x09
               |    |--sun8iw7                  |    |--1718
               |    |     |--sun50iw2p1          |    |     |---h5 // Aw1718的chipid为ID序列的bit[7:0]表示，其中：芯片    合法chip id [7:0]
                                                                       //H2+    0x42、0x83
                                                                        * h5    0x00、0x81
               |    |--sun8iw8                  |    |--1681
               |    |     |--sun8iw8p1          |    |     |---V3/V3s
               |    |--sun8iw9                  |    |--1677
               |          |--sun8iw9p1          |          |---xxx
                                                                    Aw1667的chipid为ID序列的bit[127:110]表示，其中：
                                                                    芯片    合法chip id [127:110]
                                                                    A33    000000 000000 000000
                                                                    R16    011011 000001 000110
                                                                    R35    100011 000011 000101
               |    |--sun8iw10                 |    |--1699
               |          |--sun8iw10p1         |          |---B100
               |    |--sun8iw11                 |    |--1701
               |          |--sun8iw11p1         |          |---A20E
               |
               |-- sun9i                        |-- cortex-a15-hmp
               |    |--sun9iw1                  |    |--1639
               |    |     |--sun9iw1p1                   |---A80
               |
               |-- sun50i                       |-- cortex-a53-smp
               |    |--sun50iw1                 |    |--1689
               |    |     |--sun50iw1p1         |        |---A64
     *
     *
     *
     * */
    public  String[] getChipType(){

        String[] retStr=new String[5];

        String platform = null;
        String chipid = null;
        String ictype = null;
        String reason = null;
        String cust = null;
        String custtype=null;

        SysIntf.runRootCmd("/system/bin/chmod 777 /sys/class/sunxi_info/sys_info");

        try
        {
            BufferedReader br = new BufferedReader(new FileReader(cpid_file));
            String data = br.readLine();
            int count = 0;
            while(data!=null)
            {
                  Log.d(TAG,"get online = " + data);
                  if(data.contains("chipid"))
                  {
                     chipid = data.substring(data.indexOf(":")+2);
                     chipid.replaceAll(" ", "");
                     chipid.replaceAll("　", "");
                     chipid.replaceAll(" ", "");
                     chipid.trim();
                     Log.d(TAG,"get chipid = " + chipid);
                  }
                  else if(data.contains("platform"))
                  {
                      platform = data.substring(data.indexOf(":")+2);
                      platform.replaceAll(" ", "");
                      platform.replaceAll("　", "");
                      platform.replaceAll(" ", "");
                      platform.trim();
                      Log.d(TAG,"get platform = " + platform);
                  }
                  else if(data.contains("customerid"))
                  {
                      cust = data.substring(data.indexOf(":")+2);
                      cust.replaceAll(" ", "");
                      cust.replaceAll("　", "");
                      cust.replaceAll(" ", "");
                      cust.trim();
                      Log.d(TAG,"get cust = " +cust);
                  }

                  data = br.readLine();
                  count++;
                  if(count > 10000)
                  {
                      break;
                  }
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }




        if(chipid != null && platform != null && chipid.length() == 32)
        {
            if(platform.equalsIgnoreCase("sun4iw1p1"))
            {
                ictype = "1623: A10";
                reason = "sun4iw1p1";
            }
            else if(platform.equalsIgnoreCase("sun4iw2p1"))
            {
                ictype = "1625: A13";
                reason = "sun4iw2p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw1p1"))
            {
                ictype = "1633: A31";
                reason = "sun8iw1p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw2p1"))
            {
                ictype = "1651: A20";
                reason = "sun8iw2p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw3p1"))
            {
                ictype = "1650: A23";
                reason = "sun8iw3p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw5p1"))
            {
                ictype = "1667: A33";
                reason = "sun8iw5p1";
            }
            else if(platform.equalsIgnoreCase("sun50iw6p1"))
            {
                Log.d(TAG,"get ictype = 1728");
                if(chipid.substring(6,8).equals("01"))
                {
                    ictype = "1728: H6-V200-AWIN";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("02"))
                {
                    ictype = "1728: H6-V200-DT00";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }

                else if(chipid.substring(6,8).equals("03"))
                {
                    ictype = "1728: H6-CV200-AWIN/H6-CV200-OS";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("04"))
                {
                    ictype = "1728: H6-CV200-D000";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("05"))
                {
                    ictype = "1728: H6-CV201-AWIN";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("06"))
                {
                    ictype = "1728: H6-CV201-D000";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("07"))
                {
                    ictype = "1728: H6-V200-AI";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("11"))
                {
                    ictype = "1728: H603";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }

                else
                {
                    ictype = "Unknown";
                    reason = "sun50iw6p1,chip id: 0x" + chipid.substring(6,8);
                }

//                ictype = "1728: H6";
//                reason = "平台: sun50iw6p1";
            }
            else if(platform.equalsIgnoreCase("sun50iw9")){
                reason = "sun50iw9p1,chip id: 0x" + chipid.substring(4,8);
                String ids = chipid.substring(4,8);
                String subtypes;
                if("5000".equalsIgnoreCase(ids)){
                    subtypes = "H616";
                }else if("5400".equalsIgnoreCase(ids)){
                    subtypes = "VMP1002";
                }else if("5C00".equalsIgnoreCase(ids)){
                    subtypes = "H313";
                }else if("6C00".equalsIgnoreCase(ids)){
                    subtypes = "H700";
                }else if("7400".equalsIgnoreCase(ids)){
                    subtypes = "T507";
                }else if("7C00".equalsIgnoreCase(ids)){
                    subtypes = "T503";
                }else if("2400".equalsIgnoreCase(ids)){
                    subtypes = "T517";
                }else if("2C00".equalsIgnoreCase(ids)){
                    subtypes = "T513";
                }else{
                    subtypes = "unknown";
                }
                ictype = "1823:" + " "  + subtypes;
            }
            else if(platform.equalsIgnoreCase("sun50iw10")){
                reason = "sun50iw10,chip id: 0x" + chipid.substring(4,8);
                String ids = chipid.substring(4,8);
                String subtypes;
                if("0400".equalsIgnoreCase(ids)){
                        subtypes = "A100";
                }else if("0800".equalsIgnoreCase(ids)){
                        subtypes = "T509";
                }else{
                        subtypes = "unknown";
                }
                ictype = "1855:" + " "  + subtypes;
            }
            else if(platform.equalsIgnoreCase("sun8iw6p1"))
            {
                Log.d(TAG,"get ictype = 1673");
                String tmp1 = chipid.substring(28, 29);
                String tmp2 = chipid.substring(29, 30);
                int type = ((Hex2int(tmp1)& 0x03)<<2) | ((Hex2int(tmp2)& 0x0C)>>2);
                Log.d(TAG,"get Hex2int(tmp1) = " + ((Hex2int(tmp1)& 0x03)<<2));
                Log.d(TAG,"get Hex2int(tmp1) = " + ((Hex2int(tmp2)& 0x0C)>>2));

                if(Hex2int(tmp1) < 0 || Hex2int(tmp2) < 0)
                {
                    ictype = "1673: XXX";
                    reason = "sun8iw6p1,chipid [15:10] not a hex";
                }
                else if(type == 0x0 || type == 0x03)
                {
                    ictype = "1673: H8";
                    reason = "sun8iw6p1,chip id: 0x" + Integer.toHexString(type);
                }
                else if(type == 0x1)
                {
                    ictype = "1673: A83T";
                    reason = "sun8iw6p1,chip id: 0x" + Integer.toHexString(type);
                }
                else if(type == 0x4 || type == 0x5)
                {
                    ictype = "1673: R58";
                    reason = "sun8iw6p1,chip id: 0x" + Integer.toHexString(type);
                }
                else if(type == 0x9)
                {
                    ictype = "1673: T8";
                    reason = "sun8iw6p1,chip id: 0x" + Integer.toHexString(type);
                }
                else
                {
                    ictype = "1673: XXX";
                    reason = "sun8iw6p1, invaild chip id: 0x" + Integer.toHexString(type);
                }
            }

            else if(platform.equalsIgnoreCase("sun50iw2p1"))
            {
                Log.d(TAG,"get ictype = 1718");
                if(chipid.substring(6,8).equals("01"))
                {
                    ictype = "1718: h5";
                    reason = "sun50iw2p1,chip id: 0x" + chipid.substring(6,8);
                }
                else if(chipid.substring(6,8).equals("58"))
                {
                    ictype = "1718: h5D";
                    if (cust.substring(0).equals("5"))
                        custtype="5 ,客户:易视腾_云之善";
                    else  if (cust.substring(0).equals(" x"))
                        custtype="2,客户:矩正";
                    else  if (cust.substring(0).equals("x"))
                        custtype="1，客户:协创";
                    else
                        custtype="无,客户:未知";
                    reason = "平台: sun50iw2p1,chip id: 0x" + chipid.substring(6,8);
                }
                else
                {
                    ictype = "1718: XXX";
                    reason = "sun50iw2p1,invaild chip id: 0x" + chipid.substring(6,8);
                }
                }

            else if(platform.equalsIgnoreCase("sun8iw8p1"))
            {
                 ictype = "1681: V3/V3S";
                reason = "sun8iw8p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw9p1"))
            {
                Log.d(TAG,"get ictype = 1677");

                if(chipid.substring(0, 4).equalsIgnoreCase("0000")){
                    ictype = "1677: A33";
                    reason = "sun8iw9p1,chip id£º 0x" + chipid.substring(0, 4);
                }else if(chipid.substring(0, 4).equalsIgnoreCase("6c11")){
                    //取前面16位足够判断了
                    ictype = "1677: R16";
                    reason = "sun8iw9p1,chip id£º 0x" + chipid.substring(0, 4);
                }else if(chipid.substring(0, 4).equalsIgnoreCase("8c31")){
                    //取前面16位足够判断了
                    ictype = "1677: R35";
                    reason = "sun8iw9p1,chip id£º 0x" + chipid.substring(0, 4);
                }else{
                    ictype = "1677: XXX";
                    reason = "sun8iw9p1,ÎÞÐ§chip id£º 0x" + chipid.substring(0, 4);
                }
            }
            else if(platform.equalsIgnoreCase("sun8iw10p1"))
            {
                ictype = "1699: B100";
                reason = "sun8iw10p1";
            }
            else if(platform.equalsIgnoreCase("sun8iw11p1"))
            {
                ictype = "1701: A20E";
                reason = "sun8iw11p1";
            }
            else if(platform.equalsIgnoreCase("sun9iw1p1"))
            {
                ictype = "1639: A80";
                reason = "sun9iw1p1";
            }
            else if(platform.equalsIgnoreCase("sun50iw1p1")){
                ictype = "1689: A64";
                reason = "sun50iw1p1";
            }
            else
            {
                ictype = "Unknown";
                reason = "At " + cpid_file + " no more information";
            }
        }
        else
        {
            ictype = "Unknown";
            reason = "At " + cpid_file + " no more information or file not exist";
        }



        retStr[0]=platform;
        retStr[1]=chipid;
        retStr[2]=ictype;
        retStr[3]=reason;
        retStr[4]=custtype;

        String countchipid=retStr[0]+",  "+retStr[1]+",  "+retStr[2]+",  "+retStr[3];

        String chipid_history = Environment.getExternalStorageDirectory()
                + File.separator + "h5 _chipid.txt";
        String u_file = "/mnt/usbhost/Storage01/"+File.separator+ "h5_chipid.txt";
     File file = new File(chipid_history);
     File ufile = new File(u_file);
     if (!file.exists()) {
            try {
                file.createNewFile();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    if (!ufile.exists()) {
        try {
            file.createNewFile();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

try{
    FileInputStream fis = new FileInputStream(file);
    Reader bis = new InputStreamReader(fis);
    BufferedReader bufferedReader = new BufferedReader(bis);
    String line = new String();

    if((line =  bufferedReader.readLine())==null){
        try {
                 FileWriter pw = new FileWriter(file, true);
                 pw.write(countchipid+"\n");
                 pw.flush();
                 pw.close();
                 FileWriter upw = new FileWriter(ufile, true);
                  upw.write(countchipid+"\n");
                  upw.flush();
                  upw.close();
             } catch (IOException e) {
                  e.printStackTrace();
             }
        return retStr;
    }
    if ( (line.contains(chipid)))    {return retStr;}

    while ((line =  bufferedReader.readLine()) != null) {

        if (line.contains(chipid))
        {
            return retStr;
        }
    }
        try {
         //    if (!file.exists()) {
             //    file.createNewFile();
             //}
             FileWriter pw = new FileWriter(file, true);
             pw.write(retStr[1]+"\n");
             pw.flush();
             pw.close();
             FileWriter upw = new FileWriter(ufile, true);
              upw.write(retStr[1]+"\n");
              upw.flush();
              upw.close();
         } catch (IOException e) {
              e.printStackTrace();
         }
    }
    catch(Exception e){e.printStackTrace();}

        return retStr;
      //this.platform.setText("平台:   " + platform);
      //this.chipid.setText("chip id 0x:   " + chipid);
  //    this.ictype.setText("ictype:   " + ictype);
      //this.reason.setText("依据:   " + reason);
    }


    private int Hex2int(String hex){
        if(hex.compareTo("0") >= 0 && hex.compareTo("0") <= 9){
            return hex.compareTo("0");
        }else if(hex.compareTo("a") >= 0 && hex.compareTo("a") <= 5){
            return hex.compareTo("a") + 10;
        }else if(hex.compareTo("A") >= 0 && hex.compareTo("A") <= 5){
            return hex.compareTo("A") + 10;
        }else{
            return -1;//error,not a hex
        }
    }
}
