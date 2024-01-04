package com.softwinner.view;

import android.content.Context;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.widget.TextView;

public class MTextView extends TextView {
    int delta;
    Marquee m;

    public MTextView(Context context) {
        this(context, null);
    }

    public MTextView(Context context, AttributeSet attrs) {
        this(context, attrs, com.android.internal.R.attr.textViewStyle);
    }

    public MTextView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public MTextView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }


    public final void setText2(CharSequence text) {
        setText(text);
        stopScroll();
        startScroll();
    }
    public void startScroll() {

        if (m == null) {
            m = new Marquee();
        }
        m.start();
    }
    public void stopScroll() {
        if (m != null) {
            m.stop();
        }
    }

    private final class Marquee extends Handler {
        private static final int START = 0;
        private static final int TICK = 1;
        private static final int RESTART = 2;
        private static final int MARQUEE_RESOLUTION = 1000 / 30;
        private static final int MARQUEE_PIXELS_PER_SECOND = 30;
        private int viewHeight;
        private int contentHeight;
        private int mScrollUnit = 3;
        private boolean started = false;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case START:
                    marqueeStart();
                    break;
                case TICK:
                    tick();
                    break;
                case RESTART:
                    marqueeRestart();
                    break;
                default:
                    break;
            }
        }
        public void start() {
            if (started) {
                return;
            }
            Rect b = new Rect();
            MTextView.this.getLineBounds(MTextView.this.getLineCount() - 1, b);
            contentHeight = b.bottom;
            viewHeight = MTextView.this.getHeight();
            if (contentHeight > viewHeight) {
                started = true;
                sendEmptyMessageDelayed(Marquee.START, 3000);
            }
        }
        public void stop() {
            if (started) {
                started = false;
                MTextView.this.delta = 0;
                MTextView.this.invalidate();
                removeMessages(START);
                removeMessages(RESTART);
                removeMessages(TICK);
            }
        }
        private void marqueeStart() {
            MTextView.this.delta = 0;
            started = true;
            sendEmptyMessageDelayed(TICK, MARQUEE_RESOLUTION);
        }
        private void tick() {
            if (!started) {
                return;
            }
            MTextView.this.delta += mScrollUnit;
            MTextView.this.scrollTo(0, delta);
            MTextView.this.invalidate();
            if (MTextView.this.delta < (contentHeight + viewHeight)) {
                sendEmptyMessageDelayed(TICK, MARQUEE_RESOLUTION);
            } else {
                sendEmptyMessageDelayed(RESTART, MARQUEE_RESOLUTION);
            }
        }
        private void marqueeRestart() {
            if (!started) {
                return;
            }
            MTextView.this.delta = -viewHeight;
            MTextView.this.scrollTo(0, delta);
            MTextView.this.invalidate();
            sendEmptyMessageDelayed(TICK, MARQUEE_RESOLUTION);
        }
    }
}
