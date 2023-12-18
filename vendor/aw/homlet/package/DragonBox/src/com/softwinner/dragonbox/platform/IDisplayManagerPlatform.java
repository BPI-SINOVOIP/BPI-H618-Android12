package com.softwinner.dragonbox.platform;

public interface IDisplayManagerPlatform {

    boolean getTvHotPlugStatus();
    boolean getHdmiHotPlugStatus();

    void changeToHDMI();
    void changeToCVBS();

}
