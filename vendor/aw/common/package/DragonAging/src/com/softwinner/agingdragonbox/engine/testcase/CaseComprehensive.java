package com.softwinner.agingdragonbox.engine.testcase;

import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;

import java.util.ArrayList;

import com.softwinner.agingdragonbox.Main;
import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.engine.BaseConfiguration;
import com.softwinner.agingdragonbox.engine.Utils;
import com.softwinner.agingdragonbox.xml.Node;

/**
 * 综合测试
 *
 * @author huanglong
 *
 */
public class CaseComprehensive extends BaseCase implements BaseCase.OnPassableChangeListener {

    private static final String TAG = "DragonAging-CaseComprehensive";

    private LinearLayout        mLeft;
    private LinearLayout        mRight;
    private LinearLayout        mMidLeft;
    private LinearLayout        mMidRight;
    private LinearLayout        mRight0;
    private ArrayList<BaseCase> mBaseList;

    @Override
    protected void onInitialize(Node node) {
        boolean hasVersion = false;
        boolean hasMemory = false;
        boolean hasWifi = false;
        boolean hasThreeDimensional = false;
        boolean hasCvbs = false;
        boolean hasSpdif = false;
        boolean hasEltherner = false;
        boolean hasVideo = false;
        boolean hasCamera = false;

        Node nVersion = null;
        Node nMemory = null;
        Node nWifi = null;
        Node nThreeDimensional = null;
        Node nCvbs = null;
        Node nSpdif = null;
        Node nEltherner = null;
        Node nVideo = null;
        Node nCamera = null;

        if (node != null) {
            int nNode = node.getNNodes();
            for (int i = 0; i < nNode; i++) {
                String nodeName = node.getNode(i).getName();
                if (DEBUG) {
                    Log.v(TAG, "initialize the case " + nodeName);
                }
                if (CaseMemory.class.getSimpleName().equals(nodeName)) {
                    hasMemory = true;
                    nMemory = node.getNode(i);
                } else if (CaseThreeDimensional.class.getSimpleName().equals(nodeName)) {
                    hasThreeDimensional = true;
                    nThreeDimensional = node.getNode(i);
                } else if (CaseVideo.class.getSimpleName().equals(nodeName)) {
                    hasVideo = true;
                    nVideo = node.getNode(i);
                } else if (CaseCamera.class.getSimpleName().equals(nodeName)){
                    hasCamera = true;
                    nCamera = node.getNode(i);
                }
            }
        }

        mBaseList = new ArrayList<BaseCase>();
        setView(R.layout.case_comprehensive);
        setName(R.string.case_comprehensive);
        mLeft = (LinearLayout) getView().findViewById(R.id.left);
        // mRight = (LinearLayout) getView().findViewById(R.id.right);
        mMidLeft = (LinearLayout) getView().findViewById(R.id.mid_left);
        // mMidRight = (LinearLayout) getView().findViewById(R.id.mid_right);
        // mRight0 =(LinearLayout) getView().findViewById(R.id.right0);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT);
        lp.weight = 6;
        lp.gravity = Gravity.CENTER;

        if (hasVideo) {
            BaseCase caseVideo = new CaseVideo();
            caseVideo.setEngine(mEngine);
            mBaseList.add(caseVideo);
            caseVideo.initialize(mContext, nVideo);
            caseVideo.addPassableChangeListener(this);
            caseVideo.getView().setBackgroundResource(R.drawable.comprehensive_background);
            mLeft.addView(caseVideo.getView(), 0, lp);
        }
        // 初始化3D测试，add by zhengxiangna
        if (hasThreeDimensional) {
            BaseCase caseThreedimensional = new CaseThreeDimensional();
            caseThreedimensional.setEngine(mEngine);
            mBaseList.add(caseThreedimensional);
            caseThreedimensional.initialize(mContext, nThreeDimensional);
            caseThreedimensional.addPassableChangeListener(this);
            caseThreedimensional.getView().setBackgroundResource(R.drawable.comprehensive_background);
            mLeft.addView(caseThreedimensional.getView(), 0, lp);
        }

        // 初始化memory,add by zhengxiangna
        if (hasMemory) {
            BaseCase caseMemory = new CaseMemory();
            caseMemory.setEngine(mEngine);
            mBaseList.add(caseMemory);
            caseMemory.initialize(mContext, nMemory);
            caseMemory.addPassableChangeListener(this);
            caseMemory.getView().setBackgroundResource(R.drawable.comprehensive_background);
            mMidLeft.addView(caseMemory.getView(), 0, lp);
        }
        // 初始化camera,add by qujunzheng
        if(hasCamera){
            BaseCase caseCamera = new CaseCamera();
            caseCamera.setEngine(mEngine);
            mBaseList.add(caseCamera);
            caseCamera.initialize(mContext,nCamera);
            mMidLeft.addView(caseCamera.getView(), 0, lp);
        }
        if (mMidLeft.getChildCount() == 0) {
            mMidLeft.setVisibility(View.GONE);
            if (mLeft.getChildCount() == 1) {
                mLeft.setVisibility(View.GONE);
            }
        }
        if (mLeft.getChildCount() == 0) {
            mLeft.setVisibility(View.GONE);
        }
    }

    @Override
    protected boolean onCaseStarted() {
        Utils.startCtrlLedThread();
        Main.handler.sendEmptyMessageDelayed(Main.WHAT_UPDATE_AGING_TIME,Main.UPDATE_AGING_TIME_INTERNAL);//触发定时更新测试时长属性的逻辑
        for (BaseCase aCase : mBaseList) {
            aCase.startCase();
        }
        return false;
    }

    @Override
    protected void onCaseFinished() {
        for (BaseCase aCase : mBaseList) {
            aCase.setResult(mResult);
        }
    }

    @Override
    protected void onRelease() {
        for (BaseCase aCase : mBaseList) {
            aCase.release();
        }
        mBaseList.clear();
    }

    @Override
    public void onPassableChange(BaseCase inCase, boolean passable) {
        if (!passable) {
            setPassable(passable);
        } else {
            boolean allPassed = true;
            for (BaseCase aCase : mBaseList) {
                allPassed &= aCase.getPassable();
            }
            if (allPassed) {
                setPassable(allPassed);
            }
        }
    }

    @Override
    public String getDetailResult() {
        String result = "" + super.getDetailResult();
        for (BaseCase aCase : mBaseList) {
            if (!aCase.getPassable()) {
                result += aCase.getName() + ":" + aCase.getDetailResult() + ";";
            }
        }
        return result;
    }

    @Override
    protected BaseConfiguration generateConfiguration() {
        return new ComphsConfiguration();
    }

    private class ComphsConfiguration extends BaseConfiguration {

        private CheckBox enableVersion;
        private CheckBox enableWifi;
        private CheckBox enableMemory;
        private CheckBox enableThreeDimensional;
        private CheckBox enableSpdif;
        private CheckBox enableCvbs;
        private CheckBox enableElthernet;
        private CheckBox enableVideo;
        private CheckBox enableCamera;
        private boolean  hasVersion          = true;
        private boolean  hasMemory           = true;
        private boolean  hasWifi             = true;
        private boolean  hasThreeDimensional = true;
        private boolean  hasSpdif            = true;
        private boolean  hasCvbs             = true;
        private boolean  hasElthernet        = true;
        private boolean  hasVideo            = true;
        private boolean  hasCamera           = true;

        // 版本测试
        private EditText passableFirewareEdit;
        private Button   passableVersionDetail;
        private EditText passableMemoryEdit;
        private EditText passableMaxRSSIEdit;

        private String   passableFireware;
        private String   passableDisplay;
        private String   passableMinCpu;
        private String   passableMaxCpu;
        private String   passableModel;

        private boolean  autoGet             = false;

        private String   minMemory;
        private String   maxRSSI;

        private View     titleView1;
        private View     contentView1;

        @Override
        public void onInitializeForConfig(Node node) {
            setName(R.string.case_comprehensive);
            // 加载配置信息
            if (node != null) {
                int nNode = node.getNNodes();
                hasVersion = false;
                hasMemory = false;
                hasWifi = false;
                hasThreeDimensional = false;
                hasSpdif = false;
                hasCvbs = false;
                hasElthernet = false;
                hasVideo = false;
                hasCamera = false;

                for (int i = 0; i < nNode; i++) {
                    String nodeName = node.getNode(i).getName();
                    if (CaseThreeDimensional.class.getSimpleName().equals(nodeName)) {
                        hasThreeDimensional = true;
                    } else if (CaseMemory.class.getSimpleName().equals(nodeName)) {
                        hasMemory = true;
                        Node passNode = node.getNode(i).getNode("Passable");
                        if (passNode != null) {
                            minMemory = passNode.getAttributeValue(CaseMemory.PASSABLE_MIN_CAP);
                        }
                    } else if (CaseVideo.class.getSimpleName().equals(nodeName)) {
                        hasVideo = true;
                    } else if (CaseCamera.class.getSimpleName().equals(nodeName)){
                        hasCamera = true;
                    }
                }
            }

            // 初始化用于配置的View
            mConfigView = View.inflate(mContext, R.layout.case_comprehensive_config, null);

            enableVersion = (CheckBox) mConfigView.findViewById(R.id.enable_version);
            enableWifi = (CheckBox) mConfigView.findViewById(R.id.enable_wifi);
            enableMemory = (CheckBox) mConfigView.findViewById(R.id.enable_memory);
            enableThreeDimensional = (CheckBox) mConfigView
                    .findViewById(R.id.enable_threedimensional);
            enableSpdif = (CheckBox) mConfigView.findViewById(R.id.enable_spdif);
            enableCvbs = (CheckBox) mConfigView.findViewById(R.id.enable_cvbs);
            enableElthernet = (CheckBox) mConfigView.findViewById(R.id.enable_elthernet);
            enableVideo = (CheckBox) mConfigView.findViewById(R.id.enable_video);
            enableCamera = (CheckBox) mConfigView.findViewById(R.id.enable_camera);
            enableVersion.setChecked(hasVersion);
            enableWifi.setChecked(hasWifi);
            enableMemory.setChecked(hasMemory);
            enableThreeDimensional.setChecked(hasThreeDimensional);
            enableSpdif.setChecked(hasSpdif);
            enableCvbs.setChecked(hasCvbs);
            enableElthernet.setChecked(hasElthernet);
            enableVideo.setChecked(hasVideo);
            enableCamera.setChecked(hasCamera);

            // enableVersion.requestFocus();

            passableVersionDetail = (Button) mConfigView.findViewById(R.id.auto_get);
            passableVersionDetail.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    autoGet = true;
                    passableVersionDetail
                            .setText(R.string.case_comprehensive_config_passable_version_ok);
                }
            });
            passableFirewareEdit = (EditText) mConfigView.findViewById(R.id.edit_fireware);
            passableMemoryEdit = (EditText) mConfigView.findViewById(R.id.edit_min_memory);
            passableMaxRSSIEdit = (EditText) mConfigView.findViewById(R.id.edit_max_rssi);

            if (hasVersion) {
                passableFirewareEdit.setText(passableFireware);
            }

            if (hasMemory) {
                passableMemoryEdit.setText(minMemory);
            }

            if (hasWifi) {
                passableMaxRSSIEdit.setText(maxRSSI);
            }

            titleView1 = mConfigView.findViewById(R.id.textView1);
            contentView1 = mConfigView.findViewById(R.id.linearLayout1);
            titleView1.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View arg0) {
                    if (contentView1.getVisibility() == View.VISIBLE) {
                        contentView1.setVisibility(View.GONE);
                    } else {
                        contentView1.setVisibility(View.VISIBLE);
                    }
                }
            });
        }

        public String getConfigInfo() {
            StringBuilder sb = new StringBuilder();

            sb.append(mContext.getString(R.string.case_comprehensive_config_prefix));
            if (hasVersion) {
                sb.append(mContext.getString(R.string.case_version_name));
            }
            if (hasMemory) {
                sb.append(mContext.getString(R.string.case_memory_name));
            }
            if (hasWifi) {
                sb.append(mContext.getString(R.string.case_wifi_name));
            }
            if (hasThreeDimensional) {
                sb.append(mContext.getString(R.string.case_threedimensional_name));
            }
            if (hasSpdif) {
                sb.append(mContext.getString(R.string.case_spdif_name));
            }
            if (hasCvbs) {
                sb.append(mContext.getString(R.string.case_cvbs_name));
            }
            if (hasVideo) {
                sb.append(mContext.getString(R.string.case_video_name));
            }
            if (hasElthernet) {
                sb.append(mContext.getString(R.string.case_elthernet_name));
            }
            if (hasCamera){
                sb.append(mContext.getString(R.string.case_camera_name));
            }
            return sb.toString();
        }

        @Override
        protected void onGetConfigNode(Node node) {

            if (hasMemory) {
                Node child = new Node(CaseMemory.class.getSimpleName());
                Node passable = new Node(PASSABLE_NODE_NAME);
                if (minMemory != null) {
                    passable.addAttribute(CaseMemory.PASSABLE_MIN_CAP, minMemory);
                }
                child.addNode(passable);
                node.addNode(child);
            }

            if (hasThreeDimensional) {
                node.addNode(new Node(CaseThreeDimensional.class.getSimpleName()));
            }

            if (hasVideo) {
                node.addNode(new Node(CaseVideo.class.getSimpleName()));
            }
            if (hasCamera){
                node.addNode(new Node(CaseCamera.class.getSimpleName()));
            }

        }

        @Override
        public void saveConfig() {
            hasVersion = enableVersion.isChecked();
            hasMemory = enableMemory.isChecked();
            hasWifi = enableWifi.isChecked();
            hasThreeDimensional = enableThreeDimensional.isChecked();
            hasSpdif = enableSpdif.isChecked();
            hasCvbs = enableCvbs.isChecked();
            hasElthernet = enableElthernet.isChecked();
            hasVideo = enableVideo.isChecked();
            hasCamera = enableCamera.isChecked();

            String fireware = passableFirewareEdit.getText().toString();
            if (!TextUtils.isEmpty(fireware)) {
                passableFireware = fireware;
            }
            String minMemoryStr = passableMemoryEdit.getText().toString();
            if (minMemoryStr != null && !minMemoryStr.equals("")) {
                minMemory = minMemoryStr;
            }
            String rssi = passableMaxRSSIEdit.getText().toString();
            if (rssi != null && rssi.length() > 0) {
                maxRSSI = rssi;
            } else {
                maxRSSI = "65";
            }
        }

    }

}
