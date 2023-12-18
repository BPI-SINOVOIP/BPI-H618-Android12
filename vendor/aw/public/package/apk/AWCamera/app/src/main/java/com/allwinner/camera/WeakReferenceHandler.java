package com.allwinner.camera;

import android.os.Handler;
import android.os.Message;

import java.lang.ref.Reference;
import java.lang.ref.WeakReference;

/**
 * 类说明：此类主要用来解决Handler引起的内存溢出
 */
public class WeakReferenceHandler<T extends Handler.Callback> extends Handler {
    private T handlerContainer;
    private Reference<T> handlerReference;

    public WeakReferenceHandler(T handlerContainer) {
        handlerReference = new WeakReference<>(handlerContainer);
    }

    public T getHandlerContainer() {
        return handlerReference.get();
    }

    @Override
    public void handleMessage(Message msg) {
        super.handleMessage(msg);
        Callback container = getHandlerContainer();
        if (container != null) {
            container.handleMessage(msg);
        }
    }
}