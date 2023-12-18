package com.allwinnertech.dragonsn.entity;

public class PrivateColume {

    public static final String PRIMARY_KEY_STR = "prim";
    public static final String RESULT_KEY_STR = "result";

    private String showName;
    private String colName;
    private String type;
    private int length;

    private String burnName;

    public String getShowName() {
        return showName;
    }
    public void setShowName(String aShowName) {
        this.showName = aShowName;
    }
    public String getColName() {
        return colName;
    }
    public void setColName(String aColName) {
        this.colName = aColName;
    }
    public String getType() {
        return type;
    }
    public void setType(String aType) {
        this.type = aType;
    }
    public int getLength() {
        return length;
    }
    public void setLength(int aLength) {
        this.length = aLength;
    }

    public String getBurnName() {
        return burnName;
    }
    public void setBurnName(String aBurnName) {
        this.burnName = aBurnName;
    }
    public boolean isPrimaryKey() {
        return PRIMARY_KEY_STR.equalsIgnoreCase(getType());
    }

    public boolean isResultKey() {
        return RESULT_KEY_STR.equalsIgnoreCase(getType());
    }

}
