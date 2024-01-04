package com.softwinner.dragonbox.entity;

public class NetReceivedResult {
    public String sResult="";//网络操作结果
    public String sReason="";//网络操作结果原因
    public String sCommand="";//操作命令
    public int iCode = 0;//服务器返回http命令
    @Override
    public String toString() {
        return "NetReceivedResult: \nCommand: "+sCommand+
                "\nResult: "+sResult+"\nReason: "+sReason+
                "\nCode: "+iCode;
    }
}
