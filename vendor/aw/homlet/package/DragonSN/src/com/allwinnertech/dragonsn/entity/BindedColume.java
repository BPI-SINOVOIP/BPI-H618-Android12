package com.allwinnertech.dragonsn.entity;

public class BindedColume extends PrivateColume {

    private String primValue = "";
    private String localData = "";
    private String remoteData = "";

    public String getLocalData() {
        return localData;
    }

    public void setLocalData(String aLocalData) {
        if (aLocalData != null) {
            this.localData = aLocalData;
        }
    }
    public String getRemoteData() {
        return remoteData;
    }
    public void setRemoteData(String aRemoteData) {
        if (aRemoteData != null) {
            this.remoteData = aRemoteData;
        }
    }

    public String getPrimValue() {
        return primValue;
    }

    public boolean setPrimValue(String aPrimValue) {
        if (isPrimaryKey() && aPrimValue != null && getLength() == aPrimValue.length()) {
            this.primValue = aPrimValue;
            return true;
        }
        return false;
    }

    public void clearPrimValue() {
        this.primValue = "";
    }

    public boolean isRemoteValid() {
        if (remoteData == null || getLength() != remoteData.length() || "".equals(remoteData)) {
            return false;
        }
        return true;
    }

    public boolean isLocalValid() {
        if (localData == null || getLength() != localData.length() || "".equals(localData)) {
            return false;
        }
        return true;
    }

    public boolean isAllValid() {
        return isRemoteValid() && isLocalValid() && getRemoteData().equals(getLocalData());
    }

}
