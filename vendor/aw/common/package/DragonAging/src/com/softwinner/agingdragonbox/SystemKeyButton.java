package com.softwinner.agingdragonbox;

import android.content.Context;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.IWindowManager;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Button;

public final class SystemKeyButton extends Button {

    private IWindowManager mIWindowManager;

    private long           mDownTime;

    private int            mCode = KeyEvent.KEYCODE_BACK;

    private SystemKeyButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mIWindowManager = IWindowManager.Stub.asInterface(ServiceManager
                .getService(Context.WINDOW_SERVICE));
    }

    public boolean onTouchEvent(MotionEvent ev) {
        final int action = ev.getAction();
        boolean ret = super.onTouchEvent(ev);
        switch (action) {
        case MotionEvent.ACTION_DOWN:
            mDownTime = SystemClock.uptimeMillis();
            if (mCode != 0) {
                sendEvent(KeyEvent.ACTION_DOWN, 0, mDownTime);
            }
            break;
        case MotionEvent.ACTION_UP:
            if (mCode != 0) {
                sendEvent(KeyEvent.ACTION_UP, 0);
            }
            break;
        case MotionEvent.ACTION_CANCEL:
            if (mCode != 0) {
                sendEvent(KeyEvent.ACTION_UP, KeyEvent.FLAG_CANCELED);
            }
            break;
        default:
            break;
        }
        return super.onTouchEvent(ev);
    }

    void sendEvent(int action, int flags) {
        sendEvent(action, flags, SystemClock.uptimeMillis());
    }

    void sendEvent(int action, int flags, long when) {
        final int repeatCount = (flags & KeyEvent.FLAG_LONG_PRESS) != 0 ? 1 : 0;
        final KeyEvent ev = new KeyEvent(mDownTime, when, action, mCode, repeatCount, 0,
                KeyCharacterMap.VIRTUAL_KEYBOARD, 0, flags | KeyEvent.FLAG_FROM_SYSTEM
                        | KeyEvent.FLAG_VIRTUAL_HARD_KEY, InputDevice.SOURCE_KEYBOARD);
        /*
         * try { mIWindowManager.injectInputEventNoWait(ev); } catch
         * (RemoteException ex) { }
         */
    }
}
