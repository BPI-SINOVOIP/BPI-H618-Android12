package com.softwinner.dragonbox.manager;

import com.softwinner.dragonbox.platform.GpioPlatform;

public class LedManager {
    private boolean mRunning = false;
    GpioPlatform mGpioPlatform;
    public LedManager() {
        mGpioPlatform = new GpioPlatform();
    }

    public void startFlashLed(){
        if (mRunning) {
            return;
        }
        mRunning = true;
        new Thread(new Runnable() {
            @Override
            public void run() {

                while (mRunning) {
                    try {
                        mGpioPlatform.writeGpio(1);
                        Thread.sleep(100);
                        mGpioPlatform.writeGpio(0);
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }

            }
        }).start();
    }

    public void stopFlashLed(){
        mRunning = false;
    }

}
