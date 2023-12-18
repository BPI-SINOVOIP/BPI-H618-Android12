package com.softwinner.dragonbox.platform;

public interface BTPairCallback {

    void onDeviceConnectedEvent(String os_mac);

    void onDeviceDisConnectEvent();
}