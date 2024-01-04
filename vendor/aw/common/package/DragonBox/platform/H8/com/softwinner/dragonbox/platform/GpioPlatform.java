package com.softwinner.dragonbox.platform;

public class GpioPlatform implements IGpioPlatform {
    public void writeGpio(int status) {
        char portType = 'X';
        int portNum = 1;
        com.softwinner.Gpio.writeGpio(portType, portNum, status);
    }
}
