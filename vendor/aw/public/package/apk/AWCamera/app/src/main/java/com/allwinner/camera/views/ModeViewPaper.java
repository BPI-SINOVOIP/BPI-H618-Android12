package com.allwinner.camera.views;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import com.allwinner.camera.R;
import com.allwinner.camera.WeakReferenceHandler;
import com.allwinner.camera.ui.UIManager;

import java.lang.ref.WeakReference;

public class ModeViewPaper extends ViewPager implements Handler.Callback {
    private static final int MSG_SCALE_END = 1 ;
    private static final long SCALE_END_DELAY = 100 ;
    private static String TAG = "ModeViewPaper";
    private boolean mNeedTouch = false;
    GestureDetector mGestureDetector;
    ScaleGestureDetector mScaleGestureDetector;
    private OnEventListener mListener;
    private boolean mIsScale = false;
    private WeakReferenceHandler mWeakReferenceHandler;

    @Override
    public boolean handleMessage( Message message) {
        mIsScale = false;
        return false;
    }

    interface OnEventListener {
        void onDown(MotionEvent e);

        void onShowPress(MotionEvent e);

        void onSingleTapUp(MotionEvent e);

        void onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY);

        void onLongPress(MotionEvent e);

        void onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY);

        void onScale(ScaleGestureDetector detector);

        void onScaleBegin(ScaleGestureDetector detector);

        void onScaleEnd(ScaleGestureDetector detector);
    }

    public ModeViewPaper(Context context) {
        super(context);
        mWeakReferenceHandler =  new WeakReferenceHandler<ModeViewPaper>(this) ;
    }

    public ModeViewPaper(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
        mWeakReferenceHandler =  new WeakReferenceHandler<ModeViewPaper>(this) ;
    }

    public void setListner(OnEventListener listener) {
        mListener = listener;
    }

    @Override
    public void setCurrentItem(int item) {
        super.setCurrentItem(item, false);
    }

    @Override
    public void setCurrentItem(int item, boolean smoothScroll) {
        super.setCurrentItem(item, smoothScroll);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent arg0) {
        if (mNeedTouch) {
            return super.onInterceptTouchEvent(arg0);
        } else {
            return false;
        }

    }

    @Override
    public boolean onTouchEvent(MotionEvent arg0) {
        Log.i("ModeViewPaper", "onTouchEvent");
        if (mNeedTouch) {
            return super.onTouchEvent(arg0);
        } else {
            return false;
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        Log.i("ModeViewPaper", "dispatchTouchEvent ");
        mScaleGestureDetector.onTouchEvent(ev);
        mGestureDetector.onTouchEvent(ev);
        return super.dispatchTouchEvent(ev);
    }

    public void init(Context context) {
        mGestureDetector = new GestureDetector(context, new GestureDetector.OnGestureListener() {
            @Override
            public boolean onDown(MotionEvent e) {
                if (mListener != null) {
                    mListener.onDown(e);
                }
                return false;
            }

            @Override
            public void onShowPress(MotionEvent e) {
                if (mListener != null) {
                    mListener.onShowPress(e);
                }
            }

            @Override
            public boolean onSingleTapUp(MotionEvent e) {
                if (mListener != null) {
                    Log.e(TAG, "onSingleTapUp");
                    mListener.onSingleTapUp(e);
                }
                return false;
            }

            @Override
            public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                if (mListener != null) {
                    Log.e(TAG, "onScroll");
                    if (mIsScale) {
                        return true;
                    }
                    mListener.onScroll(e1, e2, distanceX, distanceY);
                }
                return false;
            }

            @Override
            public void onLongPress(MotionEvent e) {
                if (mListener != null) {
                    mListener.onLongPress(e);
                }
            }

            @Override
            public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                if (mListener != null) {
                    Log.e(TAG, "onFling");
                    if (mIsScale) {
                        return true;
                    }
                    Log.e(TAG, "onFling2");
                    mListener.onFling(e1, e2, velocityX, velocityY);
                }
                return false;
            }
        });

        mScaleGestureDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override
            public boolean onScale(ScaleGestureDetector detector) {
                if (mListener != null) {
                    mListener.onScale(detector);
                }
                return super.onScale(detector);
            }

            @Override
            public boolean onScaleBegin(ScaleGestureDetector detector) {
                mIsScale = true;
                if (mListener != null) {
                    mListener.onScaleBegin(detector);
                }
                return super.onScaleBegin(detector);
            }

            @Override
            public void onScaleEnd(ScaleGestureDetector detector) {

                if (mListener != null) {
                    mListener.onScaleEnd(detector);
                }
                if(mWeakReferenceHandler != null) {
                    mWeakReferenceHandler.removeMessages(MSG_SCALE_END);
                    mWeakReferenceHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            Message msg = new Message();
                            msg.what = MSG_SCALE_END;
                            mWeakReferenceHandler.sendMessage(msg);
                        }
                    }, SCALE_END_DELAY);
                }

                super.onScaleEnd(detector);
            }
        });
    }



}
