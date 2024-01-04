package com.softwinner.agingdragonbox.engine;

import java.util.ArrayList;
import java.util.List;

/**
 * 缓存队列
 *
 * @author zengsc
 * @version date 2013-5-7
 */
public class QueueBuffer {
    // 队列空位置，为了队列能够加入null而设立
    private static final Object QUEUE_END = '\0';
    private final int    mSize;      // 实际缓存数+1
    private Object[]     mBuffer;    // 缓存
    private int          mFront;
    private int          mRear;

    public QueueBuffer(int size) {
        mSize = size + 1;
        mBuffer = new Object[mSize];
        reset();
    }

    /**
     * 队列缓存长度
     */
    public int size() {
        return mSize - 1;
    }

    /**
     * 入列，当队列满后队头自动出列
     */
    public void add(Object obj) {
        mBuffer[mRear] = obj;
        mRear = next(mRear);
        if (mRear == mFront) {
            pop();
        }
    }

    /**
     * 出列
     */
    public Object pop() {
        Object obj = mBuffer[mFront];
        mBuffer[mFront] = QUEUE_END;
        if (obj == QUEUE_END && mFront == mRear) {
            mRear = next(mRear);
        }
        mFront = next(mFront);
        return obj;
    }

    /**
     * 获得队头数据
     */
    public Object peek() {
        return mBuffer[mFront];
    }

    /**
     * 获得队列数据
     */
    public List<Object> getQueue() {
        List<Object> queue = new ArrayList<Object>();
        for (int i = mFront; i != mRear; i = next(i)) {
            queue.add(mBuffer[i]);
        }
        return queue;
    }

    /**
     * 清空所有缓存
     */
    public void reset() {
        for (int i = 0; i < mSize; i++) {
            mBuffer[i] = QUEUE_END;
        }
        mFront = 0;
        mRear = 0;
    }

    private int next(int index) {
        return (index + 1) % mSize;
    }
}
