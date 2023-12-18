package com.softwinner.agingdragonbox.engine;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.view.View;

import java.util.ArrayList;

import com.softwinner.agingdragonbox.xml.Node;

public abstract class BaseCase {

    protected static final boolean                DEBUG              = true;

    protected static final String                 PASSABLE_NODE_NAME = "Passable";

    /* 未初始化状态 */
    protected static final int                    STATE_UNINITIALIZE = 0;
    /* 已初始化状态 */
    protected static final int                    STATE_INITIALIZE   = 1;
    /* 开始测试状态 */
    protected static final int                    STATE_STARTED      = 2;
    /* 测试结束状态 */
    protected static final int                    STATE_FINISHED     = 3;

    /* 结果为未测试或被重置 */
    public static final int                       RESULT_RESET       = 0;
    /* 结果为未通过 */
    public static final int                       RESULT_UNPASSED    = 1;
    /* 结果为已通过 */
    public static final int                       RESULT_PASSED      = 2;

    // 连续失败次数
    protected int                                 mFailedTimes       = 0;
    // 最大连续失败次数
    protected int                                 mMaxFailedTimes    = 3;

    /* 当结果改变时，回调这一列表中所有的监听器 */
    protected ArrayList<OnResultPublishListener>  mListenerList;
    protected ArrayList<OnPassableChangeListener> mPassableLsnList;

    /* 表明当前的状态 */
    protected int                                 mState             = STATE_INITIALIZE;

    /* 表明当前的测试结果 */
    protected int                                 mResult            = RESULT_RESET;

    protected Context                             mContext;

    /* 用于表现当前测试项目的View */
    protected View                                mView;
    /* 该项测试项目的名字 */
    protected String                              nName              = new String();
    /* 流水测试引擎 */
    protected DFEngine                            mEngine;

    /* 当前测试项目能否点击通过 */
    protected boolean                             mPassable          = false;

    /* 当前测试项目是否有自定义的防怠设置 */
    protected boolean                             mHasCustomPassable = false;

    protected BaseConfiguration                   mConfiguration;

    protected static Handler                      mUiHandler;

    public BaseCase() {
        mConfiguration = generateConfiguration();
        mConfiguration.setCase(this);
    }

    /**
     * 构建一个配置类，当某一测试项希望拥有配置能力时，请重写它
     */
    protected BaseConfiguration generateConfiguration() {
        BaseConfiguration configuration = new NoneConfiguration();
        return configuration;
    }

    /**
     * 获得该测试项目的配置类
     *
     * @return
     */
    public BaseConfiguration getConfiguration() {
        return mConfiguration;
    }

    /**
     * 获得当前测试项目的名称
     *
     * @return 该测试项目的名称
     */
    public String getName() {
        return nName;
    }

    /**
     * 设置当前项目的名称
     *
     * @param name
     *            项目名称
     */
    public void setName(String name) {
        nName = name;
    }

    /**
     * 设置当前项目的名称
     *
     * @param resId
     *            指向项目名称的资源id
     */
    public void setName(int resId) {
        nName = mContext.getString(resId);
    }

    /**
     * 获得当前测试项目所使用的view
     *
     * @return
     */
    public View getView() {
        return mView;
    }

    /**
     * 设置当前测试项目的View
     *
     * @param resId
     */
    public void setView(int resId) {
        View v = View.inflate(mContext, resId, null);
        setView(v);
    }

    /**
     * 设置当前测试项目的View
     *
     * @param v
     */
    public void setView(View v) {
        mView = v;
    }

    /**
     * 初始化该测试项目，该方法会导致onInitialize被回调
     *
     * @param context
     */
    public void initialize(Context context) {
        initialize(context, null);
    }

    /**
     * 初始化该测试项目，该方法会导致onInitialize被回调
     *
     * @param context
     * @param attr
     */
    public void initialize(Context context, Node attr) {
        mListenerList = new ArrayList<OnResultPublishListener>();
        mPassableLsnList = new ArrayList<OnPassableChangeListener>();
        mState = STATE_INITIALIZE;
        mContext = context;
        onInitialize(attr);
        Node passable = attr != null ? attr.getNode(PASSABLE_NODE_NAME) : null;
        if (passable != null) {
            mHasCustomPassable = true;
            onPassableInfo(passable);
        }
    }

    /**
     * 增加一个结果监听器
     *
     * @param listener
     */
    public void addResultPublishListener(OnResultPublishListener listener) {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        mListenerList.add(listener);
    }

    /**
     * 移除一个结果监听器
     *
     * @param listener
     */
    public void removeResultPublishListener(OnResultPublishListener listener) {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        mListenerList.remove(listener);
    }

    /**
     * 执行结果公布，本项目的测试结果将会投递给所有的OnResultPublishListener
     */
    protected void performResultPublishListener() {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        for (OnResultPublishListener listener : mListenerList) {
            listener.onResultPublish(this, mResult);
        }
    }

    /**
     * 添加一个用于监听可通过事件的监听器
     *
     * @param listener
     */
    public void addPassableChangeListener(OnPassableChangeListener listener) {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        mPassableLsnList.add(listener);
    }

    /**
     * 移除一个用于监听可通过事件的监听器
     *
     * @param listener
     */
    public void removePassableChangeListener(OnPassableChangeListener listener) {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        mPassableLsnList.remove(listener);
    }

    /**
     * 当可通过事件发生时，该方法将会被调用
     */
    protected void performPassableChangeListener() {
        // if(mState == STATE_UNINITIALIZE)
        // throw new IllegalStateException("This test case is uninitialize.");
        for (OnPassableChangeListener listener : mPassableLsnList) {
            listener.onPassableChange(this, getPassable());
        }
    }

    /**
     * 启动测试，当前状态会跳转到STATE_STARTED，并回调onCaseStarted
     */
    public void startCase() {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        if (mState == STATE_STARTED) {
            throw new IllegalStateException("This test case has been started.");
        }
        mState = STATE_STARTED;
        mPassable = false;
        // 隐藏passed按钮，防怠
        Message msg = mUiHandler.obtainMessage(DFEngine.MSG_HIDE_PASSED);
        mUiHandler.sendMessage(msg);

        onCaseStarted();
    }

    /**
     * 为项目测试设置结果，同时状态会转跳到STATE_FINISHED，并回调onCaseFinished
     *
     * @param result
     */
    public void setResult(int result) {
        if (mState != STATE_STARTED) {
            throw new IllegalStateException("This test case is not in STARTED STATE.");
        }
        mState = STATE_FINISHED;
        mResult = result;
        performResultPublishListener();
        onCaseFinished();

        // 将测试结果保存到报表
        if (mEngine != null) {
            mEngine.saveResultToReport(this);
        }
    }

    /**
     * 释放所有资源，状态会转跳到STATE_UNINITIALIZE，并回调onRelease
     */
    public void release() {
        if (mState == STATE_UNINITIALIZE) {
            throw new IllegalStateException("This test case is uninitialize.");
        }
        mListenerList.clear();
        mListenerList = null;
        mState = STATE_UNINITIALIZE;
        onRelease();
    }

    protected abstract void onInitialize(Node attr);

    protected abstract boolean onCaseStarted();

    protected abstract void onCaseFinished();

    protected abstract void onRelease();

    public void setEngine(DFEngine engine) {
        mEngine = engine;
        if (mEngine != null) {
            mUiHandler = mEngine.getUiHandler();
        }
    }

    protected DFEngine getEngine() {
        return mEngine;
    }

    /**
     * 获得当前测试项目的测试结果
     *
     * @return
     */
    public int getResult() {
        return mResult;
    }

    /**
     * 设置当前能否点击通过，用于防怠
     *
     * @param passable
     */
    public void setPassable(boolean passable) {
        if (!mPassable && !passable) {
            mFailedTimes++;
            int times = mFailedTimes;
            // System.out.println(getName() + "连续失败次数：" + times);
            if (mMaxFailedTimes > 0 && times >= mMaxFailedTimes) {
                System.out.println(getName() + "连续失败次数超限：" + mMaxFailedTimes
                    + ",失败次数：" + times);
                // FailedPopupWindow fpw = new FailedPopupWindow(mContext);
                // fpw.setCase(this);
                // fpw.showAtLocation(getView(), Gravity.CENTER, 0, 0);
            }
        } else {
            mFailedTimes = 0;
        }
        mPassable = passable;
        // 仅当状态为STATE_STARTED时，且测试项目为当前项目时，投递UI更新消息
        if (mEngine != null && this == mEngine.getCurCase() && mState == STATE_STARTED) {
            Handler handler = mEngine.getUiHandler();
            Message msg = passable ? handler.obtainMessage(DFEngine.MSG_SHOW_PASSED) : handler
                    .obtainMessage(DFEngine.MSG_HIDE_PASSED);
            handler.sendMessage(msg);
        }
        performPassableChangeListener();
        onPassableChange();
    }

    /**
     * 测试结果（出错原因等）
     */
    public String getDetailResult() {
        return "failtimes:" + mFailedTimes;
    }

    protected void onPassableChange() {
    }

    protected void onPassableInfo(Node node) {
    }

    /**
     * 当前是否能够点击通过，用于防怠
     *
     * @return
     */
    public boolean getPassable() {
        return mPassable;
    }

    private Runnable         mSdlRunnable;
    private static final int SECHEDULE_MILLIS = 5 * 1000;

    /**
     * 默认的防怠计划，即5秒后设置通过状态为可通过
     */
    protected void sechedule() {
        Handler handler = mEngine.getWorkerHandler();
        mSdlRunnable = new Runnable() {
            @Override
            public void run() {
                setPassable(true);
            }
        };
        handler.postAtTime(mSdlRunnable, SECHEDULE_MILLIS);
    }

    /**
     * 取消当前的默认防怠计划
     */
    protected void cancelSechedule() {
        Handler handler = mEngine.getWorkerHandler();
        handler.removeCallbacks(mSdlRunnable);
    }

    public interface OnPassableChangeListener {
        void onPassableChange(BaseCase aCase, boolean passable);
    }

    public interface OnResultPublishListener {
        void onResultPublish(BaseCase aCase, int result);
    }

}
