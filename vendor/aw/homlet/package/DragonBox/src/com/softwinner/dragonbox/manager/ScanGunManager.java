package com.softwinner.dragonbox.manager;

import android.view.KeyEvent;

public class ScanGunManager {

    private static final String TAG = "DragonBox-ScanGunManager";

    private StringBuilder mResult;
    public OnScanValueListener mListener;
    private boolean mCaps;

    public interface OnScanValueListener {
        void onScanValue(String value);
    }

    public ScanGunManager(OnScanValueListener listener) {
        this.mListener = listener;
        this.mResult = new StringBuilder();
    }

    /**
     * 扫码设备事件解析
     */
    public void analysisKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        checkLetterStatus(event);
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            char aChar = getInputCode(mCaps, event.getKeyCode());
            if (aChar != 0) {
                mResult.append(aChar);
            }
            if (keyCode == KeyEvent.KEYCODE_ENTER) {
                if (mListener != null) {
                    mListener.onScanValue(mResult.toString());
                }
                mResult.delete(0, mResult.length());
            }
        }
    }

    /**
     * 判断大小写
     */
    private void checkLetterStatus(KeyEvent event) {
        int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT || keyCode == KeyEvent.KEYCODE_SHIFT_LEFT) {
            mCaps = event.getAction() == KeyEvent.ACTION_DOWN;
        }
    }

    /**
     * 将keyCode转为char
     *
     * @param caps    是不是大写
     * @param keyCode 按键
     * @return 按键对应的char
     */
    private char getInputCode(boolean caps, int keyCode) {
        if (keyCode >= KeyEvent.KEYCODE_A && keyCode <= KeyEvent.KEYCODE_Z) {
            return (char) ((caps ? 'A' : 'a') + keyCode - KeyEvent.KEYCODE_A);
        } else {
            return keyValue(caps, keyCode);
        }
    }

    /**
     * 按键对应的char表
     */
    private char keyValue(boolean caps, int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_0:
                return caps ? ')' : '0';
            case KeyEvent.KEYCODE_1:
                return caps ? '!' : '1';
            case KeyEvent.KEYCODE_2:
                return caps ? '@' : '2';
            case KeyEvent.KEYCODE_3:
                return caps ? '#' : '3';
            case KeyEvent.KEYCODE_4:
                return caps ? '$' : '4';
            case KeyEvent.KEYCODE_5:
                return caps ? '%' : '5';
            case KeyEvent.KEYCODE_6:
                return caps ? '^' : '6';
            case KeyEvent.KEYCODE_7:
                return caps ? '&' : '7';
            case KeyEvent.KEYCODE_8:
                return caps ? '*' : '8';
            case KeyEvent.KEYCODE_9:
                return caps ? '(' : '9';
            case KeyEvent.KEYCODE_NUMPAD_SUBTRACT:
                return '-';
            case KeyEvent.KEYCODE_MINUS:
                return '_';
            case KeyEvent.KEYCODE_EQUALS:
                return '=';
            case KeyEvent.KEYCODE_NUMPAD_ADD:
                return '+';
            case KeyEvent.KEYCODE_GRAVE:
                return caps ? '~' : '`';
            case KeyEvent.KEYCODE_BACKSLASH:
                return caps ? '|' : '\\';
            case KeyEvent.KEYCODE_LEFT_BRACKET:
                return caps ? '{' : '[';
            case KeyEvent.KEYCODE_RIGHT_BRACKET:
                return caps ? '}' : ']';
            case KeyEvent.KEYCODE_SEMICOLON:
                return caps ? ':' : ';';
            case KeyEvent.KEYCODE_APOSTROPHE:
                return caps ? '"' : '\'';
            case KeyEvent.KEYCODE_COMMA:
                return caps ? '<' : ',';
            case KeyEvent.KEYCODE_PERIOD:
                return caps ? '>' : '.';
            case KeyEvent.KEYCODE_SLASH:
                return caps ? '?' : '/';
            default:
                return 0;
        }
    }
}
