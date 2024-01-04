package com.softwinner.update.utils;

/**
 * 全局变量定义
 */
public class Constants
{
    public static String GlobalTAG = "OtaUpdate";
    public static String SERVICE_CMD = "ServiceCmd";
    public static String DOWNLOAD_INFO_SHARE_PERFS = "down_information";
    public static int SERVICE_CMD_ERROR = -1;

    public static final String KEY_START_COMMAND = "start_command";
    public static final int START_COMMAND_START_CHECKING = 101;
    public static final int START_COMMAND_START_CONNECT_CHANGE = 102;
    public static final int START_COMMAND_START_BOOT_COMPLET = 103;
    public static final int START_COMMAND_START_CHECK_ACTIVITY_CONNECTED = 104;

    //最低电量值
    public static int LOW_BATTERY_VALUE = 30;

    //升级包正确
    public static final int PACK_NO_ERROR = 0;
    //升级包不存在
    public static final int PACK_NOT_EXISTS = -1;
    //升级包没有完成
    public static final int PACK_NOT_FINSH = -2;
    //升级包MD5不一致
    public static final int PACK_MD5_NOT_MATCH = -3;
    //是否打开调试
    public static final boolean DEBUG = true;
    //OTA 包下载地址
    public static final String DOWNLOAD_PATH = "/cache/tmp/ota";
    //data分区
    public static final String DATA_PARITION = "/data";
    //OTA保存升级信息
    public static final String MSG_SAVE_FILE = "MSG.txt";
    // 最低允许的电量
    public static final int MIN_BATTERY_LEVEL = 30;

    // 检查升级结果区分
    public static final int CHECK_UPDATE_SUCC = 0;
    public static final int CHECK_UPDATE_FAIL = 1;

    // 下载结果区分
    public static final int DOWNLOAD_SUCC = 0;
    public static final int DOWNLOAD_FAIL = 1;

    /* 消息定义 */
    // Service连接上后通知Activity
    public static final int MSG_SERVICE_CONNECTED = 0;
    // 检查更新
    public static final int MSG_CHECK_UPDATE = 1;
    //开始下载
    public static final int MSG_START_DOWN_PACK = 2;
    //恢复下载
    public static final int MSG_RESUME_DOWNLOAD = 3;
    //重新下载
    public static final int MSG_AFRESH_DOWNLOAD = 4;
    //暂停下载
    public static final int MSG_PAUSE_DOWN_PACK = 5;
    //下载完毕
    public static final int MSG_FINSH_DOWN_PACK = 6;
    //Activity退出
    public static final int MSG_ACTIVITY_EXIT = 7;
    //获取电量值
    public static final int MSG_ACTIVITY_RECONNECT_DOWNLOAD = 8;
    //清除下载包
    public static final int MSG_CLEAN_DOWNLOAD = 11;

    public static final String UPDATE_GET_NEW_VERSION = "com.android.ota.UPDATE_GET_NEW_VERSION";

    public static final int REGIST_SERVER_CALLBACK = 9; //向service注册server_callbace消息

    public static final int REGIST_REQUEST_CALLBACK = 10;//向service注册REQUEST_CALLBACK消息

    //检测更新Action
    public static final String ACTION_CHECK = "com.softwinner.update.ACTION_CHECK";
    //下载中Action
    public static final String ACTION_DOWNLOAD = "com.softwinner.update.ACTION_DOWNLOAD";
    //下载暂停Action
    public static final String ACTION_DOWNLOAD_PAUSE = "com.softwinner.update.ACTION_DOWNLOAD_PAUSE";
    //下载完毕Action
    public static final String ACTION_DOWNLOAD_FINSH = "com.softwinner.update.ACTION_DOWNLOAD_FINSH";
    //下载出现错误
    public static final String ACTION_DOWNLOAD_FAIL = "com.softwinner.update.ACTION_DOWNLOAD_FAIL";
    //主界面
    public static final String ACTION_MAIN = "android.intent.action.MAIN";

    //一天查询一次
    //public static final int CHECK_REPEATE_TIME=2 * 60 * 1000;
    public static final int CHECK_REPEATE_TIME = 1 * 24 * 60 * 60 * 1000;
    public static final int CHECK_CYCLE_DAY = 1;
    //通知栏ID
    public static final int CUSTOM_NOTIFICATION_CHECK_AVAIABLE = 0x2013;
    public static final int CUSTOM_NOTIFICATION_CHECK_AVAIABLE_REQ = 1;
    public static final int CUSTOM_NOTIFICATION_DOWN = 0x3013;
    public static final int CUSTOM_NOTIFICATION_DOWN_REQ = 2;
    //状态栏下载信息和Activity同步相关的标识符
    //下载中
    public static final int NOTIF_TO_ACTIVITY_DOWNLOADING = 0;
    //下载暂停
    public static final int NOTIF_TO_ACTIVITY_DOWN_PAUSE = 1;
    /* 更新状态定义 */
    // 初始化状态
    public static final int STATE_READY = 0;
    // 正在检查状态
    public static final int STATE_CHECKING = 1;
    // 正在下载状态
    public static final int STATE_DOWNLOADING = 2;
    // 下载完成状态
    public static final int STATE_DOWNLOADED = 3;
    // 下载暂停状态
    public static final int STATE_DOWNLOADPAUSE = 4;
    // Activity类型区分
    public static final int ACTIVITY_CHECK_UPDATE = 0x1;
    // 保存更新包信息的shared preference的名称
    public static final String PACKAGE_PREF_NAME = "download_package";
	
}
