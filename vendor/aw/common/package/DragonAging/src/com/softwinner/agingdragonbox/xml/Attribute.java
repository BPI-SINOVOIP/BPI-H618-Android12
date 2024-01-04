/******************************************************************
 *
 * CyberXML for Java
 *
 * Copyright (C) Satoshi Konno 2002
 *
 * File: Attribute.java
 *
 * Revision;
 *
 * 11/27/02 - first revision.
 *
 ******************************************************************/

package com.softwinner.agingdragonbox.xml;

public class Attribute {
    private String mName  = new String();
    private String mValue = new String();

    public Attribute() {
    }

    public Attribute(String name, String value) {
        setName(name);
        setValue(value);
    }

    // //////////////////////////////////////////////
    // name
    // //////////////////////////////////////////////

    public void setName(String name) {
        this.mName = name;
    }

    public String getName() {
        return mName;
    }

    // //////////////////////////////////////////////
    // value
    // //////////////////////////////////////////////

    public void setValue(String value) {
        this.mValue = value;
    }

    public String getValue() {
        return mValue;
    }
}
