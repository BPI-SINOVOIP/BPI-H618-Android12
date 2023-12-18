package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.widget.AbsoluteLayout;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

import com.softwinner.dragonbox.R;

public class CaseTouchScreen extends IBaseCase {
    public static final String TAG = "DragonBox-CaseTouchScreen";
    FrameLayout mFrameLayout;
    AbsoluteLayout mAbsoluteLayout;
    ImageView mIvBackground;
    ImageView mIvCursor;
    TextView mTextView;//minView显示的信息
    Context mContext;
    private int mScreenWidth,mScreenHeight;//屏幕宽高
    private int mFrameLayoutWidth,mFrameLayoutHeight;//帧布局的宽高
    int mCursourWidth;//光标对应方块的边长
    boolean mDrag = false;//拖动方块

    public CaseTouchScreen(Context context) {
        super(context, R.string.case_touchscreen_name, R.layout.case_touchscreen_max,
                R.layout.case_touchscreen_min, TYPE_MODE_MANUAL);
        mContext = context;
    }

    public CaseTouchScreen(Context context, XmlPullParser xmlParser) {
        this(context);
        mFrameLayout = (FrameLayout)mMaxView.findViewById(R.id.case_touchscreen_fl);
        mIvBackground = (ImageView)mMaxView.findViewById(R.id.case_touchscreen_iv_bg);
        mIvCursor = (ImageView)mMaxView.findViewById(R.id.case_touchscreen_iv);
        mAbsoluteLayout = (AbsoluteLayout)mMaxView.findViewById(R.id.case_touchscreen_abs);
        mTextView = (TextView)mMinView.findViewById(R.id.case_touchscreen_tv);
        //获取屏幕大小
        DisplayMetrics dm = new DisplayMetrics();
        Activity activity = (Activity)mContext;
        activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
        mScreenWidth = dm.widthPixels;
        mScreenHeight = dm.heightPixels;
        Log.w(TAG,"screen width is "+mScreenWidth);
        Log.w(TAG,"screen height is "+mScreenHeight);

    }

    private void initView() {
        LayoutParams lFrameLayoutParams = mFrameLayout.getLayoutParams();
        lFrameLayoutParams.height = (int) (mScreenHeight*0.6);
        lFrameLayoutParams.width = (int) (mScreenWidth*0.6);
        Log.w(TAG,"mFrameLayout width is "+ lFrameLayoutParams.width+" height is "+lFrameLayoutParams.height);
        if((float)mScreenWidth/(float)mScreenHeight > (float)(16.0/9.0)) {
            lFrameLayoutParams.width = lFrameLayoutParams.height*16/9;
        }else {
            lFrameLayoutParams.height = lFrameLayoutParams.width*9/16;
        }
        Log.w(TAG,"mFrameLayout width is "+ lFrameLayoutParams.width+" height is "+lFrameLayoutParams.height);
        mFrameLayoutHeight = lFrameLayoutParams.height;
        mFrameLayoutWidth = lFrameLayoutParams.width;
        mFrameLayout.setLayoutParams(lFrameLayoutParams);

        //240 为vision绘图时背景图片的长，20为cursour的边长
        mCursourWidth = 20*lFrameLayoutParams.width/240;

        RestoreCursor();
        mAbsoluteLayout.setOnTouchListener(new View.OnTouchListener() {

            @SuppressWarnings("deprecation")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                // TODO Auto-generated method stub
                float x = event.getX();
                float y = event.getY();
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        if(x>0 && x< mCursourWidth && y>0 && y< mCursourWidth) {//触摸点再方块内部，允许拖拽
                            mDrag = true;
                        }
                        break;
                    case MotionEvent.ACTION_MOVE:
                        if(mDrag) {
                            picMove(x, y);
                        }
                        break;
                    case MotionEvent.ACTION_UP:
                        mDrag = false;
                        if(x>mFrameLayoutWidth-mCursourWidth && y>mFrameLayoutHeight-mCursourWidth) {
                            x=mFrameLayoutWidth-mCursourWidth;
                            y=mFrameLayoutHeight-mCursourWidth;
                            mIvCursor.setLayoutParams(new AbsoluteLayout.LayoutParams(mCursourWidth, mCursourWidth, (int) x, (int) y));
                            setDialogPositiveButtonEnable(true);
                            mMaxViewDialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
                        }else {
                            RestoreCursor();
                        }
                        break;

                    default:
                        break;
                }
                return true;
            }
        });
    }



    /* 移动图片的方法 */
    @SuppressWarnings("deprecation")
    private void picMove(float x, float y) {
        /* 默认微调图片与指针的相对位置 */
        float cursourX = x - (mCursourWidth / 2);
        float cursourY = y - (mCursourWidth / 2);

        /* 防图片超过屏幕的相关处理 */
        /* 防止屏幕向右超过屏幕 */
        if ((cursourX + mCursourWidth) > mFrameLayoutWidth) {
            cursourX = mFrameLayoutWidth - mCursourWidth;
        }
        /* 防止屏幕向左超过屏幕 */
        else if (cursourX < 0) {
            cursourX = 0;
        }
        /* 防止屏幕向下超过屏幕 */
        else if ((cursourY + mCursourWidth) > mFrameLayoutHeight) {
            cursourY = mFrameLayoutHeight - mCursourWidth;
        }
        /* 防止屏幕向上超过屏幕 */
        else if (cursourY < 0) {
            cursourY = 0;
        }
        /* 通过log 来查看图片位置 */
        //Log.w(TAG, Float.toString(cursourX) + "," + Float.toString(cursourY));
        /* 以setLayoutParams方法，重新安排Layout上的位置 */
        mIvCursor.setLayoutParams(new AbsoluteLayout.LayoutParams(mCursourWidth, mCursourWidth, (int) cursourX, (int) cursourY));
    }

    @SuppressWarnings("deprecation")
    private void RestoreCursor() {
        //当没有滑动到指定位置时，恢复光标至左上角
        mIvCursor.setLayoutParams(new AbsoluteLayout.LayoutParams(mCursourWidth, mCursourWidth, 0, 0));
    }
    @Override
    public void onStartCase() {
        Log.w(TAG,"CaseTouchScreen onStartCase!");
        setDialogPositiveButtonEnable(false);
        initView();

    }

    @Override
    public void onStopCase() {
        boolean testResult = getCaseResult();
        if(testResult) {
            mTextView.setText(R.string.case_touchscreen_testresult_true);
        }else {
            mTextView.setText(R.string.case_touchscreen_testresult_false);
        }
    }

    @Override
    public void reset() {
        super.reset();
    }
}
