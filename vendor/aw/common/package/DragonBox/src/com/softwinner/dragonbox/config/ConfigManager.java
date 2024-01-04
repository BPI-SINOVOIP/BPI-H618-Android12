package com.softwinner.dragonbox.config;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.testcase.CaseAging;
import com.softwinner.dragonbox.testcase.CaseBattery;
import com.softwinner.dragonbox.testcase.CaseBluetooth;
import com.softwinner.dragonbox.testcase.CaseCPU;
import com.softwinner.dragonbox.testcase.CaseMicRecordGraphic;
import com.softwinner.dragonbox.testcase.CaseCamera;
import com.softwinner.dragonbox.testcase.CaseCouple;
import com.softwinner.dragonbox.testcase.CaseCvbs;
import com.softwinner.dragonbox.testcase.CaseCvbsFool;
import com.softwinner.dragonbox.testcase.CaseEthernet;
import com.softwinner.dragonbox.testcase.CaseGPU;
import com.softwinner.dragonbox.testcase.CaseHdmi;
import com.softwinner.dragonbox.testcase.CaseHdmiFool;
import com.softwinner.dragonbox.testcase.CaseHeadSet;
import com.softwinner.dragonbox.testcase.CaseKey;
import com.softwinner.dragonbox.testcase.CaseLed;
import com.softwinner.dragonbox.testcase.CaseMicRecord;
import com.softwinner.dragonbox.testcase.CasePerformance;
import com.softwinner.dragonbox.testcase.CaseRF;
import com.softwinner.dragonbox.testcase.CaseResetKey;
import com.softwinner.dragonbox.testcase.CaseRid;
import com.softwinner.dragonbox.testcase.CaseSDVolume;
import com.softwinner.dragonbox.testcase.CaseSIM;
import com.softwinner.dragonbox.testcase.CaseSpdif;
import com.softwinner.dragonbox.testcase.CaseSpeaker;
import com.softwinner.dragonbox.testcase.CaseSql;
import com.softwinner.dragonbox.testcase.CaseTMSN;
import com.softwinner.dragonbox.testcase.CaseTouchScreen;
import com.softwinner.dragonbox.testcase.CaseUsbVolume;
import com.softwinner.dragonbox.testcase.CaseVersion;
import com.softwinner.dragonbox.testcase.CaseVideo;
import com.softwinner.dragonbox.testcase.CaseWifi;
import com.softwinner.dragonbox.testcase.CaseYSTLan;
import com.softwinner.dragonbox.testcase.CaseYSTMicRecord;
import com.softwinner.dragonbox.testcase.CaseYSTMicRecord2;
import com.softwinner.dragonbox.testcase.CaseYSTVersion;
import com.softwinner.dragonbox.testcase.CaseYSTWan;
import com.softwinner.dragonbox.testcase.CaseYSTWifi;
import com.softwinner.dragonbox.testcase.IBaseCase;
import com.softwinner.dragonbox.utils.PreferenceUtil;
import com.softwinner.dragonbox.utils.Utils;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.util.Xml;
import android.os.SystemProperties;
import android.widget.Toast;

public class ConfigManager {
    private Context mContext;
    private static final String TAG = "DragonBox-ConfigManager";
    private static ConfigManager sConfigManager;
    public static final String CONFIG_DRAGON_BOX = "/DragonBox/custom_cases.xml";
    public static final String CONFIG_DRAGON_SN = "/DragonBox/DragonInt.txt";
    public static final String CONFIG_DRAGON_AGING = "/DragonBox/custom_aging_cases.xml";
    public static final String CONFIG_USB_DEVICE = "/DragonBox/openusbdevice";

    public static final String DRAGON_BOX_PKG = "com.softwinner.dragonbox";
    public static final String DRAGON_BOX_MAIN_CLASS = "com.softwinner.dragonbox.ui.DragonBoxMain";
    public static final String DRAGON_BOX_BTPAIR_CLASS = "com.softwinner.dragonbox.ui.BTPairActivity";

    public static final String DRAGON_SN_PKG = "com.allwinnertech.dragonsn";
    public static final String DRAGON_SN_CLASS = "com.allwinnertech.dragonsn.DragonSNActivity";
    public static final String DRAGON_AGING_PKG = "com.softwinner.agingdragonbox";
    public static final String DRAGON_AGING_CLASS = "com.softwinner.agingdragonbox.Main";
    public static final String[][] UPLOAD_KEY = {
            {"CaseResetKey","Res"},
            {"CaseVersion","Ver"},
            {"CaseAging","Ag"},
            {"CaseCPU","CPU"},
            {"CaseKey","IR"},
            {"CaseHdmi","Hdmi"},
            {"CaseCvbs","Cvbs"},
            {"CaseUsbVolume","Usb"},
            {"CaseWifi","Wifi"},//
            {"CaseHdmiFool","Hdmi"},
            {"CaseCvbsFool","Cvbs"},
            {"CaseRid","Rid"},
            {"CaseVideo","Vid"},
            {"CaseGPU","GPU"},
            {"CaseEthernet","Ethe"},
            {"CaseTMSN","TMSN"},
            {"CaseCouple","Coup"},
            {"CaseRF","RF"},
            {"CaseLed","Led"},
    };

    private static final boolean FOR_TEST = true;

    private ConfigManager(Context context) {
        mContext = context;
    }

    private static final Map<String, Class<? extends IBaseCase>> mAllValidCases = new HashMap<String, Class<? extends IBaseCase>>() {
        private static final long serialVersionUID = 1L;
        {
            put(CaseHdmi.class.getSimpleName(), CaseHdmi.class);
            put(CaseCvbs.class.getSimpleName(), CaseCvbs.class);
            put(CaseHdmiFool.class.getSimpleName(), CaseHdmiFool.class);
            put(CaseCvbsFool.class.getSimpleName(), CaseCvbsFool.class);
            put(CaseLed.class.getSimpleName(), CaseLed.class);
            put(CasePerformance.class.getSimpleName(), CasePerformance.class);
            put(CaseSpdif.class.getSimpleName(), CaseSpdif.class);
            put(CaseUsbVolume.class.getSimpleName(), CaseUsbVolume.class);
            put(CaseVersion.class.getSimpleName(), CaseVersion.class);
            put(CaseVideo.class.getSimpleName(), CaseVideo.class);
            put(CaseSDVolume.class.getSimpleName(), CaseSDVolume.class);
            put(CaseEthernet.class.getSimpleName(), CaseEthernet.class);
            put(CaseBluetooth.class.getSimpleName(), CaseBluetooth.class);
            put(CaseWifi.class.getSimpleName(), CaseWifi.class);
            //put(CaseMixPlay.class.getSimpleName(), CaseMixPlay.class);
            put(CaseMicRecord.class.getSimpleName(), CaseMicRecord.class);
            put(CaseYSTVersion.class.getSimpleName(), CaseYSTVersion.class);
            put(CaseCamera.class.getSimpleName(), CaseCamera.class);
            put(CaseResetKey.class.getSimpleName(), CaseResetKey.class);

            put(CaseYSTWan.class.getSimpleName(), CaseYSTWan.class);
            put(CaseYSTWifi.class.getSimpleName(), CaseYSTWifi.class);
            put(CaseYSTLan.class.getSimpleName(), CaseYSTLan.class);
            put(CaseSpeaker.class.getSimpleName(), CaseSpeaker.class);
            put(CaseYSTMicRecord.class.getSimpleName(), CaseYSTMicRecord.class);
            put(CaseYSTMicRecord2.class.getSimpleName(), CaseYSTMicRecord2.class);
            put(CaseMicRecordGraphic.class.getSimpleName(), CaseMicRecordGraphic.class);
            put(CaseKey.class.getSimpleName(), CaseKey.class);
            put(CaseRid.class.getSimpleName(), CaseRid.class);
            put(CaseBattery.class.getSimpleName(), CaseBattery.class);
            put(CaseTouchScreen.class.getSimpleName(), CaseTouchScreen.class);
            put(CaseSIM.class.getSimpleName(), CaseSIM.class);
            put(CaseHeadSet.class.getSimpleName(), CaseHeadSet.class);
            put(CaseCPU.class.getSimpleName(), CaseCPU.class);
            put(CaseAging.class.getSimpleName(), CaseAging.class);
            put(CaseGPU.class.getSimpleName(), CaseGPU.class);
            put(CaseTMSN.class.getSimpleName(), CaseTMSN.class);
            put(CaseSql.class.getSimpleName(), CaseSql.class);
            put(CaseRF.class.getSimpleName(), CaseRF.class);
            put(CaseCouple.class.getSimpleName(), CaseCouple.class);
        }
    };

    public static ConfigManager getInstence(Context context) {
        if (sConfigManager == null) {
            sConfigManager = new ConfigManager(context);
        }
        return sConfigManager;
    }

    public static boolean startConfigAPK(Context context, String configFile,
                                         boolean showToast) {
        String packageName = null;
        String className = null;
        String configPath = null;
        if (!isConfigFileExist(context, configFile)) {
            if (showToast) {
                Toast.makeText(context, R.string.start_tools_error,
                        Toast.LENGTH_SHORT).show();
            }
            if(SystemProperties.getBoolean("persist.sys.dragonaging",false)&&ConfigManager.CONFIG_DRAGON_AGING.equals(configFile)) {

            }else {
                return false;
            }
        }
        configPath = Utils.getFileAbsolutePath(context, configFile);
        if (ConfigManager.CONFIG_DRAGON_BOX.equals(configFile)) {
            packageName = ConfigManager.DRAGON_BOX_PKG;
            className = ConfigManager.DRAGON_BOX_MAIN_CLASS;
            // FOR special Logic
            if (isConfigFileExist(context, ConfigManager.CONFIG_DRAGON_SN)&&
                    PreferenceUtil.readFactory(context).equals(PreferenceUtil.FACTORY_SUCCESS)) {
                return false;
            }

        } else if (ConfigManager.CONFIG_DRAGON_SN.equals(configFile)) {
            packageName = ConfigManager.DRAGON_SN_PKG;
            className = ConfigManager.DRAGON_SN_CLASS;

        } else if (ConfigManager.CONFIG_DRAGON_AGING.equals(configFile)) {
            packageName = ConfigManager.DRAGON_AGING_PKG;
            className = ConfigManager.DRAGON_AGING_CLASS;
        } else {
            if (showToast) {
                Toast.makeText(context, R.string.start_tools_error,
                        Toast.LENGTH_SHORT).show();
            }
            return false;
        }

        if (packageName != null && className != null) {
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(packageName, className));
            intent.putExtra("configPath",configPath);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(intent);
            } catch (Exception e) {
                e.printStackTrace();
                if (showToast) {
                    Toast.makeText(context, R.string.start_tools_error,
                            Toast.LENGTH_SHORT).show();
                }
                return false;
            }
        }
        return true;
    }

    public static void startBTPairActivity(Context context) {
        String packageName = ConfigManager.DRAGON_BOX_PKG;
        String className = ConfigManager.DRAGON_BOX_BTPAIR_CLASS;
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(packageName, className));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public static boolean isConfigFileExist(Context context, String filePath) {
        String confiFile = Utils.getFileAbsolutePath(context, filePath);
        if (confiFile == null || "".equals(confiFile)) {
            return false;
        }
        return true;
    }

    public List<IBaseCase> parseConfig() throws Exception {
        String configFile = Utils.getFileAbsolutePath(mContext,
                CONFIG_DRAGON_BOX);
        IBaseCase curCase = null;
        List<IBaseCase> cases = new ArrayList<IBaseCase>();

        InputStream inputStream = null;
        XmlPullParser xmlParser = Xml.newPullParser();

        // just for test ,easy to launcher that app.
        if (FOR_TEST && (configFile == null || "".equals(configFile))) {
            //cases.add((IBaseCase)(new CaseLed(mContext)));
            //cases.add((IBaseCase)(new CaseWifi(mContext, -65, "DIRECT-AW-MiniMax-nty3", "12345678")));
            //cases.add((IBaseCase)(new CaseCamera(mContext)));
            //cases.add((IBaseCase)(new CaseBluetooth(mContext)));
            cases.add((IBaseCase)(new CaseMicRecordGraphic(mContext,2)));
            return cases;
        }

        inputStream = new FileInputStream(new File(configFile));
        xmlParser.setInput(inputStream, "utf-8");

        int evtType = xmlParser.getEventType();
        String tag = "";
        while (evtType != XmlPullParser.END_DOCUMENT) {
            tag = xmlParser.getName();
            switch (evtType) {
                case XmlPullParser.START_TAG:
                    if (mAllValidCases.containsKey(tag)) {
                        try {
                            Class<? extends IBaseCase> caseClass = mAllValidCases
                                    .get(tag);
                            Log.i(TAG, "parseConfig caseClass=" + caseClass);
                            Class[] paraType = new Class[2];
                            paraType[0] = Context.class;
                            paraType[1] = XmlPullParser.class;
                            Constructor<? extends IBaseCase> constructor = caseClass
                                    .getConstructor(paraType);
                            curCase = (IBaseCase) constructor.newInstance(mContext, xmlParser);
                            for(int i=0;i<UPLOAD_KEY.length;i++){
                                if(tag.equals(UPLOAD_KEY[i][0])){
                                    curCase.mTestName = UPLOAD_KEY[i][1];
                                }
                            }
                            Log.e(TAG, ""+curCase.mTestName);
                        } catch (Exception e) {
                            e.printStackTrace();
                            Toast.makeText(mContext, "init xml error", Toast.LENGTH_LONG).show();
                        }
                    }
                    break;

                case XmlPullParser.END_TAG:
                    if (mAllValidCases.containsKey(tag) && curCase != null) {
                        cases.add(curCase);
                        curCase = null;
                    }
                    break;
                default:
                    break;
            }

            evtType = xmlParser.next();
        }
        inputStream.close();

        return cases;
    }
}
