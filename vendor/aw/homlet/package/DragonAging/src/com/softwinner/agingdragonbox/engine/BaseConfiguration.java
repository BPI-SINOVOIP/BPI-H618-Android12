package com.softwinner.agingdragonbox.engine;

import android.content.Context;
import android.view.View;

import com.softwinner.agingdragonbox.xml.Node;

/**
 * @author huanglong
 */
public abstract class BaseConfiguration {

    /* 配置所使用的View */
    protected View     mConfigView;

    /* 具体的配置信息，通过字符串表示 */
    protected String   mConfigInfo;

    /* 该配置项的名称 */
    protected String   mName;

    protected BaseCase mCase;

    protected Context  mContext;

    /**
     * 初始出配置，该方法会回调onInitializeForConfig
     *
     * @param context
     */
    public void initializeForConfig(Context context) {
        initializeForConfig(context, null);
    }

    /**
     * 初始化配置，该方法会回调onInitializeForConfig
     *
     * @param context
     * @param attr
     */
    public void initializeForConfig(Context context, Node attr) {
        mContext = context;
        onInitializeForConfig(attr);
    }

    protected abstract void onInitializeForConfig(Node attr);

    void setCase(BaseCase aCase) {
        mCase = aCase;
    }

    /**
     * 能否被配置
     *
     * @return
     */
    public boolean configurable() {
        return getConfigInfo() != null;
    }

    /**
     * 获得该测试项的具体配置信息，通过字符串表示
     *
     * @return
     */
    public String getConfigInfo() {
        return mConfigInfo;
    }

    /**
     * 获得配置之后的XML节点，该方法会回调onGetConfigNode
     *
     * @return
     */
    public Node getConfigNode() {
        Node node = new Node();
        node.setName(mCase.getClass().getSimpleName());
        onGetConfigNode(node);
        return node;
    }

    protected void onGetConfigNode(Node node) {
    }

    /**
     * 保存配置信息
     */
    public void saveConfig() {
    }

    /**
     * 获得当前用于配置的View
     *
     * @return
     */
    public View getConfigView() {
        return mConfigView;
    }

    /**
     * 获得当前配置项目的名称
     *
     * @return 该测试项目的名称
     */
    public String getName() {
        return mName;
    }

    /**
     * 设置配置项目的名称
     *
     * @param name
     *            项目名称
     */
    public void setName(String name) {
        mName = name;
    }

    /**
     * 设置配置项目的名称
     *
     * @param resId
     *            指向项目名称的资源id
     */
    public void setName(int resId) {
        mName = mContext.getString(resId);
    }
}
