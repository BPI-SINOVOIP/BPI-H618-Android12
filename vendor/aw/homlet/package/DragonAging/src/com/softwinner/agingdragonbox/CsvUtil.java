package com.softwinner.agingdragonbox;

import android.os.Environment;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

/**
 * CSV操作类 CSV转map
 *
 * @author maizirong
 *
 */
public class CsvUtil {
    private String         mFilename       = null;
    private BufferedReader bufferedreader = null;
    private List           mList           = new ArrayList();

    public CsvUtil() {
    }

    public CsvUtil(String aFileName) throws IOException {
        mFilename = aFileName;
        InputStreamReader isr = new InputStreamReader(new FileInputStream(new File(Environment
                .getExternalStorageDirectory().getPath() + aFileName)), "UTF-8");
        bufferedreader = new BufferedReader(isr);
        String stemp;
        while ((stemp = bufferedreader.readLine()) != null) {
            mList.add(stemp);
        }
    }

    public List getList() throws IOException {
        return mList;
    }

    // 得到csv文件的行数
    public int getRowNum() {
        return mList.size();
    }

    // 得到csv文件的列数
    public int getColNum() {
        if (!mList.toString().equals("[]")) {
            if (mList.get(0).toString().contains(",")) { // csv文件中，每列之间的是用','来分隔的
                return mList.get(0).toString().split(",").length;
            } else if (mList.get(0).toString().trim().length() != 0) {
                return 1;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }

    // 取得指定行的值
    public String getRow(int index) {
        if (this.mList.size() != 0) {
            return (String) mList.get(index);
        } else {
            return null;
        }
    }

    // 取得指定列的值
    public String getCol(int index) {
        if (this.getColNum() == 0) {
            return null;
        }
        StringBuffer scol = new StringBuffer();
        String temp = null;
        int colnum = this.getColNum();
        if (colnum > 1) {
            for (Iterator it = mList.iterator(); it.hasNext();) {
                temp = it.next().toString();
                scol = scol.append(temp.split(",")[index] + ",");
            }
        } else {
            for (Iterator it = mList.iterator(); it.hasNext();) {
                temp = it.next().toString();
                scol = scol.append(temp + ",");
            }
        }
        String str = new String(scol.toString());
        str = str.substring(0, str.length() - 1);
        return str;
    }

    // 取得指定行，指定列的值
    public String getString(int row, int col) {
        String temp = null;
        int colnum = this.getColNum();
        if (colnum > 1) {
            temp = mList.get(row).toString().split(",")[col];
        } else if (colnum == 1) {
            temp = mList.get(row).toString();
        } else {
            temp = null;
        }
        return temp;
    }

    public void csvClose() throws IOException {
        this.bufferedreader.close();
    }

    public List readCvs(String filename) throws IOException {
        CsvUtil cu = new CsvUtil(filename);
        List list = cu.getList();

        return list;
    }

    /*
     * public void createCsv(String biao, List list, String path) throws
     * IOException { List tt = list; String data = ""; SimpleDateFormat
     * dataFormat = new SimpleDateFormat("yyyyMMdd"); Date today = new Date();
     * String dateToday = dataFormat.format(today); File file = new File(path +
     * "resource/expert/" + dateToday + "importerrorinfo.csv"); if
     * (!file.exists()) file.createNewFile(); else file.delete(); String str[];
     * StringBuilder sb = new StringBuilder(""); sb.append(biao);
     * FileOutputStream writerStream = new FileOutputStream(file, true);
     * BufferedWriter output = new BufferedWriter(new OutputStreamWriter(
     * writerStream, "UTF-8")); for (Iterator itt = tt.iterator();
     * itt.hasNext();) { String fileStr = itt.next().toString();
     * sb.append(fileStr + "\r\n"); } output.write(sb.toString());
     * output.flush(); output.close(); }
     */

    public ArrayList<HashMap<String, Object>> getDate() {
        ArrayList<HashMap<String, Object>> listItem = new ArrayList<HashMap<String, Object>>();
        // 列数
        int col = getColNum();
        // 行数
        int row = getRowNum();
        System.out.println("col==========================" + Integer.toString(col));
        System.out.println("row==========================" + Integer.toString(row));
        for (int i = 0; i < col; i++) {
            // for (int j = 0; j < row; j++) {
            HashMap<String, Object> map = new HashMap<String, Object>();
            System.out.println("================" + getString(0, i) + "==" + getString(1, i));
            map.put(getString(0, i), getString(1, i));
            listItem.add(map);
            // }
        }
        return listItem;
    }
}
