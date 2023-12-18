package com.softwinner.agingdragonbox.engine;

import android.content.Context;
import android.content.res.Resources;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

import com.softwinner.agingdragonbox.Configuration;
import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.xml.Node;
import com.softwinner.agingdragonbox.xml.Parser;
import com.softwinner.agingdragonbox.xml.ParserException;
import com.softwinner.agingdragonbox.xml.parse.XmlPullParser;

/**
 * 流水化测试引擎
 *
 * @author huanglong
 */
public final class DFEngine {

    static final String               TAG                         = "DragonAging-DFEngine";
    static final boolean              DEBUG                       = false;

    public static final String        EXTSD_CASE_MP3_NAME
        = "/mnt/extsd/DragonBox/testbox.mp3";
    public static final String        USBHOST0_CASE_MP3_NAME
        = "/mnt/usbhost0/DragonBox/testbox.mp3";
    public static final String        USBHOST1_CASE_MP3_NAME
        = "/mnt/usbhost1/DragonBox/testbox.mp3";
    public static final String        USBHOST2_CASE_MP3_NAME
        = "/mnt/usbhost2/DragonBox/testbox.mp3";
    public static final String        USBHOST3_CASE_MP3_NAME
        = "/mnt/usbhost3/DragonBox/testbox.mp3";

    public static final String        SDCARD_CASE_MP3_NAME        = Environment
        .getExternalStorageDirectory().getPath() + "/DragonBox/testbox.mp3";

    /* 位于extsd上用于保存DragonBox配置信息的配置文件路径 */
    public static final String        EXTSD_CASE_FILE_NAME
        = "/mnt/extsd/DragonBox/custom_aging_cases.xml";
    public static final String        EXTSD_CASE_DIR
        = "/mnt/extsd/DragonBox/";
    /* 位于usbhost上用于保存DragonBox配置信息的配置文件路径 */
    public static final String        USBHOST0_CASE_FILE_NAME
        = "/mnt/usbhost0/DragonBox/custom_aging_cases.xml";
    public static final String        USBHOST0_CASE_DIR           = "/mnt/usbhost0/DragonBox/";

    public static final String        USBHOST1_CASE_FILE_NAME
        = "/mnt/usbhost1/DragonBox/custom_aging_cases.xml";
    public static final String        USBHOST1_CASE_DIR           = "/mnt/usbhost1/DragonBox/";

    public static final String        USBHOST2_CASE_FILE_NAME
        = "/mnt/usbhost2/DragonBox/custom_aging_cases.xml";
    public static final String        USBHOST2_CASE_DIR           = "/mnt/usbhost2/DragonBox/";

    public static final String        USBHOST3_CASE_FILE_NAME
        = "/mnt/usbhost3/DragonBox/custom_aging_cases.xml";
    public static final String        USBHOST3_CASE_DIR           = "/mnt/usbhost3/DragonBox/";

    /* 位于asset下用于保存DragonBox配置信息的配置文件名称 */
    public static final String        DEFAULT_CASE_FILE_NAME      = "default_cases.xml";

    /* assets包名 */
    public static final String        DEFAULT_CASE_SHARED_PACKAGE
        = "com.softwinner.agingdragonbox.res";

    public static final String        DRAGON_FILE_PATH            = Environment
        .getExternalStorageDirectory().getPath() + "/DragonBox/";
    public static final String        SMTLOG                      = Environment
        .getExternalStorageDirectory().getPath() + "/DragonBox/SMTLog.csv";
    public static final String        FATPLOG                     = Environment
        .getExternalStorageDirectory().getPath() + "/DragonBox/FATPLog.csv";
    // 开启启动flag
    public static final String        FLAG_PATH                   = Environment
        .getExternalStorageDirectory().getPath() + "/boot_flag";
    public static final String        FLAG_SMT                    = "SMT";
    public static final String        FLAG_FATP                   = "FATP";
    public static final String        PRIVATE_ROOT                = "/snsn/";

    public static final String        INTERNAL_TEST_CONFIG_HEAD   = "@internal:";
    public static final String        INTERNAL_GPUSTRESS_TEST     = "CaseGPUStress";

    public static final int           MSG_SHOW_PASSED             = 1;
    public static final int           MSG_HIDE_PASSED             = 2;
    public static final int           MSG_SHOW_FAILED             = 3;
    public static final int           MSG_HIDE_FAILED             = 4;

    /* 流水测试模式 */
    public static final int           MODE_FLOWING                = 1;
    /* 回归测试模式 */
    public static final int           MODE_REVIEWS                = 2;
    /* 自由测试模式 */
    public static final int           MODE_FREEDOM                = 3;
    private int                       mMode                       = MODE_FLOWING;

    /* 用于保存流水化测试每一项测试项目的列表 */
    private final ArrayList<BaseCase> mCaseList                   = new ArrayList<BaseCase>();
    /* 当前测试的项目序号 */
    private int                       mCurCaseIndex               = -1;
    /* 进行回归测试时的测试序号，该序号在启动某项回归测试时为该项测试项目的序号，当结束回归测试时，该序号为-1 */
    private int                       mReviewsIndex               = -1;
    /* 解析xml时所用的解析器 */
    private Parser                    mParser;
    /* 用于保存Context对象 */
    private Context                   mContext;

    /* UI回调接口 */
    private WeakReference<UICallback> mCallback                   = new WeakReference<UICallback>(
                                                                          null);

    /* DFEngine 使用的是单例模式 */
    private static DFEngine           mInstance;

    private static HandlerThread      sWorker;
    private static Handler            sHandler;
    private static Handler            sUiHandler;

    private boolean                   isSaveReport;
    private String                    mConfigPath                 = null;

    public DFEngine(Context context, String configPath) {
        if (mInstance == null) {
            mInstance = this;
            sWorker = new HandlerThread("Engine-working-thread");
            sWorker.start();
            sHandler = new Handler(sWorker.getLooper());
            sUiHandler = new UiHandler();
            mConfigPath = configPath;
            initialize(context);
        } else {
            Log.e(TAG, "The instance can not be instantized twice!");
        }
    }

    public static DFEngine getInstance() {
        if (mInstance == null) {
            Log.e(TAG, "The instance maybe has not been instantized!");
        }
        return mInstance;
    }

    public void setUICallback(UICallback uicallback) {
        mCallback = new WeakReference<UICallback>(uicallback);
    }

    private void initialize(Context context) {
        mContext = context;
        mParser = new XmlPullParser();
        String reportFile = null;
        String idPath = null;

        int msgID = 0;
        loadingCustom(mConfigPath);
        if (mConfigPath.indexOf("extsd") > 0) {
            msgID = R.string.use_external_config_file;
            reportFile = SMTLOG;
            idPath = "bd.txt";
            setRunFlag(FLAG_SMT);
        } else {
            msgID = R.string.use_usbhost_config_file;
            reportFile = FATPLOG;
            idPath = "sn.txt";
            setRunFlag(FLAG_FATP);
        }
        /*
         * if (loadingCustom(EXTSD_CASE_FILE_NAME)) { msgID =
         * R.string.use_external_config_file; reportFile = SMTLog; idPath =
         * "bd.txt"; setRunFlag(FLAG_SMT); } else if
         * (loadingCustom(USBHOST0_CASE_FILE_NAME)) { msgID =
         * R.string.use_usbhost_config_file; reportFile = FATPLog; idPath =
         * "sn.txt"; setRunFlag(FLAG_FATP); } else if
         * (loadingCustom(USBHOST1_CASE_FILE_NAME)) { msgID =
         * R.string.use_usbhost_config_file; reportFile = FATPLog; idPath =
         * "sn.txt"; setRunFlag(FLAG_FATP); } else if
         * (loadingCustom(USBHOST2_CASE_FILE_NAME)) { msgID =
         * R.string.use_usbhost_config_file; reportFile = FATPLog; idPath =
         * "sn.txt"; setRunFlag(FLAG_FATP); } else if
         * (loadingCustom(USBHOST3_CASE_FILE_NAME)) { msgID =
         * R.string.use_usbhost_config_file; reportFile = FATPLog; idPath =
         * "sn.txt"; setRunFlag(FLAG_FATP); }
         */
        if (mCaseList.size() <= 0) {
            return;
        }
        Toast.makeText(mContext, msgID, Toast.LENGTH_SHORT).show();

        if (isSaveReport) {
            String deviceId = Settings.Secure.getString(mContext.getContentResolver(),
                    Settings.Secure.ANDROID_ID);
            System.out.println("DeviceId" + deviceId);
            // RootCmd.execRootCmdSilent("mkdir " + PRIVATE_ROOT);
            // RootCmd.execRootCmdSilent("mount -t msdos /dev/block/private " +
            // PRIVATE_ROOT);
            File file = null;
            File outFile = null;
            BufferedReader reader = null;
            BufferedWriter writer = null;
            String line = null;
            if (idPath != null) {
                file = new File(PRIVATE_ROOT + idPath);
            }
            if (file != null && file.exists()) {
                outFile = new File(DRAGON_FILE_PATH + idPath);
                if (!outFile.exists()) {
                    try {
                        outFile.createNewFile();
                    } catch (IOException e1) {
                        e1.printStackTrace();
                    }
                }
                try {
                    reader = new BufferedReader(new FileReader(file));
                    writer = new BufferedWriter(new FileWriter(outFile));
                    deviceId = reader.readLine();
                    writer.write(deviceId);
                    writer.write("\n");
                    while ((line = reader.readLine()) != null) {
                        writer.write(line);
                        writer.write("\n");
                    }
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                } finally {
                    if (reader != null) {
                        try {
                            reader.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    if (writer != null) {
                        try {
                            writer.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
            // RootCmd.execRootCmdSilent("umount " + PRIVATE_ROOT);
            System.out.println("DeviceId" + deviceId);
            // mReport = new Report(reportFile, deviceId);

        }
    }

    private void setRunFlag(String part) {
        File flagFile = new File(FLAG_PATH);
        BufferedWriter writer = null;
        if (!flagFile.exists()) {
            try {
                flagFile.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        try {
            writer = new BufferedWriter(new FileWriter(flagFile));
            writer.write(part);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public Handler getWorkerHandler() {
        return sHandler;
    }

    /* called in package */Handler getUiHandler() {
        return sUiHandler;
    }

    /**
     * 根据标记，载入配置文件
     *
     * @param tag
     * @return 成功载入返回true，失败则返回false
     */
    private boolean loadingCustom(String path) {
        try {
            /*
             * if (EXTSD_CASE_FILE_NAME.equals(tag)) { InputStream fis =
             * getFileInputStream(EXTSD_CASE_FILE_NAME); createCase(fis);
             * fis.close(); } else if (USBHOST0_CASE_FILE_NAME.equals(tag)) {
             * InputStream fis = getFileInputStream(USBHOST0_CASE_FILE_NAME);
             * createCase(fis); fis.close(); } else if
             * (USBHOST1_CASE_FILE_NAME.equals(tag)) { InputStream fis =
             * getFileInputStream(USBHOST1_CASE_FILE_NAME); createCase(fis);
             * fis.close(); } else if (USBHOST2_CASE_FILE_NAME.equals(tag)) {
             * InputStream fis = getFileInputStream(USBHOST2_CASE_FILE_NAME);
             * createCase(fis); fis.close(); } else if
             * (USBHOST3_CASE_FILE_NAME.equals(tag)) { InputStream fis =
             * getFileInputStream(USBHOST3_CASE_FILE_NAME); createCase(fis);
             * fis.close(); } else if (DEFAULT_CASE_FILE_NAME.equals(tag)) {
             * AssetManager assetManager = new SharedContext(mContext,
             * DEFAULT_CASE_SHARED_PACKAGE).getAssets(); InputStream in =
             * assetManager.open(DEFAULT_CASE_FILE_NAME); createCase(in);
             * in.close(); }
             */
            InputStream fis = getFileInputStream(path);
            createCase(fis);
            fis.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            Log.w(TAG, "File \"" + path + "\" not found.");
        } catch (ParserException e) {
            e.printStackTrace();
            Log.e(TAG, "File \"" + path + "\" parser error.");
        }
        return false;
    }

    private InputStream getFileInputStream(String fileName) {
        Log.e(TAG, "get config xml filename: " + fileName);
        if (fileName.startsWith(INTERNAL_TEST_CONFIG_HEAD)) {
            String rawName = fileName.substring(INTERNAL_TEST_CONFIG_HEAD.length());
            Log.e(TAG, "use internal test: " + rawName);
            if (rawName.equals(INTERNAL_GPUSTRESS_TEST)) {
                try {
                    return mContext.getResources().openRawResource(R.raw.case_gpu_stress);
                } catch (Resources.NotFoundException e) {
                    Log.e(TAG, "not fount: " + rawName);
                    return null;
                }
            }
        } else {
            try {
                File file = new File(fileName);
                if (!file.exists()) {
                    return null;
                }
                if (file.length() == 0) {
                    return null;
                }
                FileInputStream fis = new FileInputStream(file);
                byte[] buffer = new byte[(int) file.length()];
                fis.read(buffer);
                fis.close();
                ByteArrayInputStream bais = new ByteArrayInputStream(buffer);
                return bais;
            } catch (FileNotFoundException e1) {
                e1.printStackTrace();
                return null;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        }
        return null;
    }

    /**
     * 设置当前的测试结果
     *
     * @param result
     */
    public void setCurCaseResult(int result) {
        if (mMode == MODE_FLOWING) { // 当前模式为流水测试模式时
            if (mCurCaseIndex >= 0 && mCurCaseIndex < mCaseList.size()) {
                BaseCase mCase = mCaseList.get(mCurCaseIndex);
                mCase.setResult(result);
            }
        } else if (mMode == MODE_REVIEWS) { // 当前模式为回归测试测试模式时
            if (mReviewsIndex != -1) {
                BaseCase mCase = mCaseList.get(mReviewsIndex);
                mCase.setResult(result);
                mReviewsIndex = -1;
            }
        }
    }

    public void saveResultToReport(BaseCase mCase) {

    }

    /**
     * 启动前一个测试项目。 当前模式为流水模式时，将启动前一个测试项目； 当前模式为回归模式时，将通知UI，跳转到结果输出画面；
     */
    public void startPreCase() {
        UICallback uicallback = mCallback.get();
        if (mMode == MODE_FLOWING) {
            if (mCurCaseIndex > 0 && mCurCaseIndex < mCaseList.size()) {
                BaseCase mCase = mCaseList.get(mCurCaseIndex);
                mCase.setResult(mCase.getResult());
                mCurCaseIndex--;
                BaseCase previousCase = mCaseList.get(mCurCaseIndex);
                if (uicallback != null) {
                    uicallback.setCaseContent(previousCase.getView());
                }
                previousCase.startCase();
            }
        } else if (mMode == MODE_REVIEWS) {
            if (uicallback != null) {
                uicallback.onCaseCompleted();
            }
        }
    }

    /**
     * 启动当前流水模式的测试项目
     */
    public void startCurCaseAtFlowing() {
        if (mMode == MODE_FLOWING) {
            BaseCase mCase = mCaseList.get(mCurCaseIndex);
            UICallback uicallback = mCallback.get();
            if (uicallback != null) {
                uicallback.setCaseContent(mCase.getView());
                mCase.startCase();
            }
        }
    }

    /**
     * 启动下一个测试项目 当前模式为流水模式，同时下一个项目存在时将启动下一个存在项目，否则跳转到结果输出画面，并进入回归模式。
     * 当前模式为回归模式时，自动跳转到结果输出画面。
     */
    public void startNextCase() {
        UICallback uicallback = mCallback.get();
        if (mMode == MODE_FLOWING) {
            if (mCurCaseIndex < mCaseList.size() - 1) {
                mCurCaseIndex++;
                BaseCase mCase = mCaseList.get(mCurCaseIndex);
                if (uicallback != null) {
                    uicallback.setCaseContent(mCase.getView());
                    mCase.startCase();
                }
            } else {
                mMode = MODE_REVIEWS;
                if (uicallback != null) {
                    uicallback.onCaseCompleted();
                }
            }
        } else if (mMode == MODE_REVIEWS) {
            if (uicallback != null) {
                uicallback.onCaseCompleted();
            }
        }
    }

    /**
     * 当当前模式为回归模式时，启动指定位置的测试项目
     *
     * @param index
     *            该位置的测试项目
     */
    public void startCaseAtReviews(int index) {
        if (mMode == MODE_REVIEWS) {
            if (index < 0 || index >= mCaseList.size()) {
                return;
            }
            UICallback uicallback = mCallback.get();
            mReviewsIndex = index;
            BaseCase mCase = mCaseList.get(index);
            if (uicallback != null) {
                uicallback.setCaseContent(mCase.getView());
                mCase.startCase();
            }
        }
    }

    public void startCaseAtFreedom(int index) {
        if (index < 0 || index >= mCaseList.size()) {
            return;
        }
        UICallback uicallback = mCallback.get();
        mReviewsIndex = index;
        BaseCase mCase = mCaseList.get(index);
        if (uicallback != null) {
            uicallback.setCaseContent(mCase.getView());
            mCase.startCase();
        }
    }

    /**
     * 获得当前测试项目的数量
     *
     * @return 当前测试项目的数量
     */
    public int getCaseSize() {
        return mCaseList.size();
    }

    /**
     * 获得当前正在测试的项目
     *
     * @return 当前正在测试的项目
     */
    public BaseCase getCurCase() {
        if (mMode == MODE_FLOWING) {
            if (mCurCaseIndex >= 0 && mCurCaseIndex < mCaseList.size()) {
                return mCaseList.get(mCurCaseIndex);
            }
        } else if (mMode == MODE_REVIEWS) {
            if (mReviewsIndex >= 0 && mReviewsIndex < mCaseList.size()) {
                return mCaseList.get(mReviewsIndex);
            }
        }
        return null;
    }

    /**
     * 获得测试标题
     *
     * @return
     */
    public String getTitle() {
        if (mMode == MODE_FLOWING) {
            if (mCurCaseIndex >= 0 && mCurCaseIndex < mCaseList.size()) {
                BaseCase mCase = mCaseList.get(mCurCaseIndex);
                return "<font size=\"12\">"
                        + mContext.getString(R.string.title_previous)
                        + (mCurCaseIndex > 0 ? mCaseList.get(mCurCaseIndex - 1).getName()
                                : mContext.getString(R.string.title_no))
                        + "</font>"
                        + "<font size=\"20\" color=\"#00DBDB\">"
                        + "&nbsp;&nbsp;&nbsp;&nbsp;"
                        + mContext.getString(R.string.title_current)
                        + mCase.getName()
                        + "&nbsp;&nbsp;&nbsp;&nbsp;"
                        + "</font>"
                        + "<font size=\"12\">"
                        + mContext.getString(R.string.title_next)
                        + (mCurCaseIndex < mCaseList.size() - 1 ? mCaseList.get(mCurCaseIndex + 1)
                                .getName() : mContext.getString(R.string.title_no)) + "</font>";
            }
        } else if (mMode == MODE_REVIEWS) {
            if (mReviewsIndex >= 0 && mReviewsIndex < mCaseList.size()) {
                BaseCase aCase = mCaseList.get(mReviewsIndex);
                return "<font size=\"20\" color=\"#00DBDB\">" + "&nbsp;&nbsp;&nbsp;&nbsp;"
                        + aCase.getName() + "&nbsp;&nbsp;&nbsp;&nbsp;" + "</font>";
            }
        }
        return "";
    }

    public int getMode() {
        return mMode;
    }

    public void createCase(File xml) {
        Node node = null;
        try {
            node = mParser.parse(xml);
        } catch (ParserException e) {
            e.printStackTrace();
            return;
        }
        parseNode(node);
    }

    public void createCase(String xml) {
        Node node = null;
        try {
            node = mParser.parse(xml);
        } catch (ParserException e) {
            e.printStackTrace();
            return;
        }
        if (node == null) {
            return;
        }
        parseNode(node);
    }

    public void createCase(InputStream xml) throws ParserException {
        Node node = null;
        node = mParser.parse(xml);
        parseNode(node);
    }

    private void parseNode(Node node) {
        if (node.getAttribute(Configuration.SAVE_REPORT) != null) {
            isSaveReport = true;
        }

        int n = node.getNNodes();
        for (int i = 0; i < n; i++) {
            Node child = node.getNode(i);
            String caseName = child.getName();
            Log.v(TAG,"caseName = "+caseName);
            BaseCase mCase = Utils.createCase(caseName);
            if (child.hasNodes() || child.hasAttributes()) {
                Log.v(TAG, "child:" + child.getName() + "|" + child.getNNodes());
                addCase(mCase, child);
            } else {
                addCase(mCase);
            }
        }
    }

    /**
     * 增加一个测试项目
     *
     * @param mCase
     */
    public void addCase(BaseCase mCase) {
        if (mCase != null) {
            synchronized (mCaseList) {
                mCase.setEngine(this);
                mCase.initialize(mContext);
                mCaseList.add(mCase);
            }
        }
    }

    /**
     * 增加一个测试项目
     *
     * @param mCase
     * @param attr
     */
    public void addCase(BaseCase mCase, Node attr) {
        if (mCase != null) {
            synchronized (mCaseList) {
                mCase.setEngine(this);
                mCase.initialize(mContext, attr);
                mCaseList.add(mCase);
            }
        }
    }

    /**
     * 移除一个测试项目
     *
     * @param mCase
     */
    public void removeCase(BaseCase mCase) {
        if (mCase != null) {
            synchronized (mCaseList) {
                mCase.setEngine(null);
                mCaseList.remove(mCase);
            }
        }
    }

    /**
     * 获得指定位置的测试项目
     *
     * @param index
     *            期望访问的位置
     * @return 该位置的测试项目实例
     */
    public BaseCase getCase(int index) {
        return mCaseList.get(index);
    }

    /**
     * 释放所有资源，并还原所有相关设置
     */
    public void release() {
        synchronized (mCaseList) {
            for (BaseCase mCase : mCaseList) {
                // 注意调用的顺序
                mCase.release();
                mCase.setEngine(null);
            }
            mParser = null;
            mCaseList.clear();
        }
        sWorker.getLooper().quit();
        sUiHandler = null;
        sWorker = null;
        sHandler = null;
        mInstance = null;
    }

    /**
     * 用于处理UI事件的Handler
     *
     * @author huanglong
     */
    private class UiHandler extends Handler {
        UiHandler() {
            super(Looper.getMainLooper());
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            UICallback callback = mCallback.get();
            if (msg.what == MSG_SHOW_PASSED) {
                callback.setUiVisible(UICallback.UI_COMPONENT_BTN_PASSED, View.VISIBLE);
            } else if (msg.what == MSG_HIDE_PASSED) {
                callback.setUiVisible(UICallback.UI_COMPONENT_BTN_PASSED, View.INVISIBLE);
            } else if (msg.what == MSG_SHOW_FAILED) {
                callback.setUiVisible(UICallback.UI_COMPONENT_BTN_FAILED, View.VISIBLE);
            } else if (msg.what == MSG_HIDE_FAILED) {
                callback.setUiVisible(UICallback.UI_COMPONENT_BTN_FAILED, View.INVISIBLE);
            }
        }
    }

}
