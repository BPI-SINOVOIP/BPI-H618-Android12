/******************************************************************
 *
 * CyberXML for Java
 *
 * Copyright (C) Satoshi Konno 2002
 *
 * File: Parser.java
 *
 * Revision;
 *
 * 11/26/2003 - first revision. 03/30/2005 - Change parse(String) to use
 * StringBufferInputStream instead of URL. 11/11/2009 - Changed Parser::parser()
 * to use ByteArrayInputStream instead of StringBufferInputStream because of
 * bugs in Android v1.6.
 *
 ******************************************************************/

package com.softwinner.agingdragonbox.xml;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;

public abstract class Parser {
    // //////////////////////////////////////////////
    // Constructor
    // //////////////////////////////////////////////

    public Parser() {
    }

    // //////////////////////////////////////////////
    // parse
    // //////////////////////////////////////////////

    public abstract Node parse(InputStream inStream) throws ParserException;

    // //////////////////////////////////////////////
    // parse (File)
    // //////////////////////////////////////////////

    public Node parse(File descriptionFile) throws ParserException {
        try {
            InputStream fileIn = new FileInputStream(descriptionFile);
            Node root = parse(fileIn);
            fileIn.close();
            return root;

        } catch (Exception e) {
            throw new ParserException(e);
        }
    }

    // //////////////////////////////////////////////
    // parse (Memory)
    // //////////////////////////////////////////////

    public Node parse(String descr) throws ParserException {
        try {
            InputStream decrIn = new ByteArrayInputStream(descr.getBytes());
            Node root = parse(decrIn);
            return root;
        } catch (Exception e) {
            throw new ParserException(e);
        }
    }

}
