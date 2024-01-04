package com.softwinner.dragonbox.platform;

public class GpioPlatform implements IGpioPlatform {
    public void writeGpio(int status) {
		/*char portType = 'X';
		int portNum = 1;
		com.softwinner.Gpio.writeGpio(portType, portNum, status);*/
        if(status > 0){
            com.softwinner.Gpio.setNormalLedOn(false);
            com.softwinner.Gpio.setStandbyLedOn(true);
        }else{
            com.softwinner.Gpio.setNormalLedOn(true);
            com.softwinner.Gpio.setStandbyLedOn(false);
        }
    }
}
