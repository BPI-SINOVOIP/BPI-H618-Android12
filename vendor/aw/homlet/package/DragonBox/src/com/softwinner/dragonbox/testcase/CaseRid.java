package com.softwinner.dragonbox.testcase;

import java.io.File;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.utils.Utils;

public class CaseRid extends IBaseCase {
    public static final String TAG = "DragonBox-CaseRid";
    TextView mMinRIDStatus;
    TextView mMaxRIDStatus;

    public CaseRid(Context context) {
        super(context, R.string.case_rid_name, R.layout.case_rid_max,
                R.layout.case_rid_min, TYPE_MODE_AUTO);
        mMinRIDStatus = (TextView) mMinView.findViewById(R.id.case_rid_status);
        mMaxRIDStatus = (TextView) mMaxView.findViewById(R.id.case_rid_max_info);
    }

    public CaseRid(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseRid");
        Utils.writeRidtoPersistSecure();
        File ridFile = new File(Utils.RID_FILE_PATH);
        Boolean ridCheckPass = ridFile.exists()&&(ridFile.length()==Utils.RID_FILE_LENGTH);
        mMinRIDStatus.setText(Utils.readFile(Utils.RID_FILE_PATH));
        mMaxRIDStatus.setText(Utils.readFile(Utils.RID_FILE_PATH));
        stopCase();
        setCaseResult(ridCheckPass);
        Log.w(TAG,"CaseRid test over ,test result is "+getCaseResult());
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void reset() {
        super.reset();
        mMinRIDStatus.setText(R.string.case_rid_status_text);
    }

}
