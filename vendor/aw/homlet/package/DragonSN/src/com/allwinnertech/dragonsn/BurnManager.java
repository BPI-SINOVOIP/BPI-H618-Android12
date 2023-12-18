package com.allwinnertech.dragonsn;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.util.List;

import com.allwinnertech.dragonsn.entity.BindedColume;
import com.allwinnertech.dragonsn.jni.ReadPrivateJNI;


public class BurnManager {
    List<BindedColume> mBindedColume;
    ReadPrivateJNI mReadPriateJNI;
    RemoteDBUtils mRemoteDBUtils;
    Handler mHandler;

    public BurnManager(Handler handler, List<BindedColume> bindedColume, Config config) {
        mReadPriateJNI = new ReadPrivateJNI();
        mHandler = handler;
        mBindedColume = bindedColume;
        mRemoteDBUtils = new RemoteDBUtils(config);
    }

    public void loadLocalDatas() {
        for (BindedColume colume : mBindedColume) {
            if (!colume.isResultKey()) {
                colume.setLocalData(getLocalParameter(colume.getBurnName()));
            }
        }
    }

    public void resetData() {
        for (BindedColume colume : mBindedColume) {
            colume.setLocalData("");
            colume.setRemoteData("");
            colume.clearPrimValue();
        }
    }

    public BindedColume getPrimColume() {
        for (BindedColume colume : mBindedColume) {
            if (colume.isPrimaryKey()) {
                return colume;
            }
        }
        return null;
    }

    public boolean isAllRemoteDataValid() {
        for (BindedColume colume : mBindedColume) {
            if (!colume.isRemoteValid()) {
                return false;
            }
        }
        return true;
    }

    public void loadRemoteData() {
        new Thread(new Runnable() {
                @Override
                public void run() {
                boolean result = mRemoteDBUtils.queryData(mBindedColume);
                Message msg = mHandler.obtainMessage(DragonSNActivity.MSG_LOAD_FINISH);
                msg.arg1 = result ? 1 : 0;
                mHandler.sendMessage(msg);
                }
                }).start();
    }

    public boolean updataLocatData() {
        boolean result = true;
        for (BindedColume colume : mBindedColume) {
            if (!colume.isResultKey()) {
                boolean curResult = setLocalParameter(colume.getBurnName(),
                    colume.getRemoteData());
                Log.i(DragonSNActivity.TAG, "updataLocatData " + colume.getBurnName() + "="
                    + colume.getRemoteData());
                result = result && curResult;
            }
        }
        return result;
    }

    public void updateResult() {
        new Thread(new Runnable() {
                @Override
                public void run() {
                boolean result = mRemoteDBUtils.updateResult(mBindedColume);
                Message msg = mHandler.obtainMessage(DragonSNActivity.MSG_UPDATE_FINISH);
                msg.arg1 = result ? 1 : 0;
                mHandler.sendMessage(msg);
                }
                }).start();
    }

    public boolean checkAllColumeValidAndSetReturnResult() {
        for (BindedColume colume : mBindedColume) {
            if (colume.isResultKey()) {
                continue;
            }
            if (!colume.isAllValid()) {
                return false;
            }
        }

        for (BindedColume colume : mBindedColume) {
            if (colume.isResultKey()) {
                colume.setLocalData("1");
            }
        }
        return true;
    }

    public List<BindedColume> getBindedColumes() {
        return mBindedColume;
    }

    private boolean setLocalParameter(String name, String value) {
        return mReadPriateJNI.nativeSetParameter(name, value);
    }

    private String getLocalParameter(String name) {
        String result = null;
        try {
            result = mReadPriateJNI.nativeGetParameter(name);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

}
