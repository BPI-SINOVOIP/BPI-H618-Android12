package com.softwinner.dragonbox.platform;

public class GpioPlatform implements IGpioPlatform {
    public void writeGpio(int status) {
        char portType = 'b';
        int portNum = 2;
        android.os.Gpio.writeGpio(portType, portNum, status);
    }
}
