package com.softwinner.dragonbox.platform;

public class GpioPlatform implements IGpioPlatform {
    public void writeGpio(int status) {
        char portType = 'A';
        int portNum = 15;
        com.softwinner.Gpio.writeGpio(portType, portNum, status);
    }
}
