package com.softwinner.agingdragonbox.engine;

import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.os.IBinder;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupWindow;
import android.widget.TextView;

import com.softwinner.agingdragonbox.R;

/**
 * 测试失败弹出窗口
 *
 * @author zengsc
 * @version date 2013-4-28
 */
public class FailedPopupWindow extends PopupWindow implements View.OnClickListener {
    private Context  mContext;
    private View     mView;
    private TextView mFailedTip;
    private Button   mConfirmButton;
    private BaseCase mCase;

    public FailedPopupWindow(Context context) {
        super(context);
        mContext = context;
        init();
    }

    private void init() {
        mView = View.inflate(mContext, R.layout.failed_window, null);
        mFailedTip = (TextView) mView.findViewById(R.id.test_failed_tip);
        mConfirmButton = (Button) mView.findViewById(R.id.test_failed_button);
        mConfirmButton.setOnClickListener(this);
        setContentView(mView);

        setBackgroundDrawable(new ColorDrawable());
        // 设置PopupWindow可获得焦点
        setFocusable(true);
        // 设置PopupWindow可触摸
        setTouchable(true);
        // 设置非PopupWindow区域可触摸
        setOutsideTouchable(false);

        setWidth(ViewGroup.LayoutParams.MATCH_PARENT);
        setHeight(ViewGroup.LayoutParams.MATCH_PARENT);
    }

    @Override
    public void showAtLocation(IBinder token, int gravity, int x, int y) {
        beforeShow();
        super.showAtLocation(token, gravity, x, y);
    }

    @Override
    public void onClick(View v) {
        if (v == mConfirmButton) {
            dismiss();
        }
    }

    private void beforeShow() {
        mFailedTip.setText(mCase.getName() + mContext.getString(R.string.test_failed_tip)
                + "\nDetails:" + mCase.getDetailResult());
    }

    public void setCase(BaseCase testCase) {
        mCase = testCase;
    }
}
