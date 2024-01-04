/******************************************************************
 *
 * CyberXML for Java
 *
 * Copyright (C) Satoshi Konno 2002-2003
 *
 * File: XML.java
 *
 * Revision;
 *
 * 01/05/03 - first revision. 12/15/03 - Terje Bakken - Added escapeXMLChars()
 *
 ******************************************************************/

package com.softwinner.agingdragonbox.xml;

public class XML {
    public static final String CONTENT_TYPE = "text/xml; charset=\"utf-8\"";
    public static final String CHARSET_UTF8 = "utf-8";

    // //////////////////////////////////////////////
    // escapeXMLChars
    // //////////////////////////////////////////////

    private static String escapeXMLChars(String input, boolean quote) {
        if (input == null) {
            return null;
        }
        StringBuffer out = new StringBuffer();
        int oldsize = input.length();
        char[] old = new char[oldsize];
        input.getChars(0, oldsize, old, 0);
        int selstart = 0;
        String entity = null;
        for (int i = 0; i < oldsize; i++) {
            switch (old[i]) {
            case '&':
                entity = "&amp;";
                break;
            case '<':
                entity = "&lt;";
                break;
            case '>':
                entity = "&gt;";
                break;
            case '\'':
                if (quote) {
                    entity = "&apos;";
                    break;
                }
            case '"':
                if (quote) {
                    entity = "&quot;";
                    break;
                }
            default:
                break;
            }
            if (entity != null) {
                out.append(old, selstart, i - selstart);
                out.append(entity);
                selstart = i + 1;
                entity = null;
            }
        }
        if (selstart == 0) {
            return input;
        }
        out.append(old, selstart, oldsize - selstart);
        return out.toString();
    }

    public static String escapeXMLChars(String input) {
        return escapeXMLChars(input, true);
    }

    // //////////////////////////////////////////////
    // unescapeXMLChars
    // //////////////////////////////////////////////

    public static String unescapeXMLChars(String input) {
        if (input == null) {
            return null;
        }

        String outStr;

        outStr = input.replace("&amp;", "&");
        outStr = outStr.replace("&lt;", "<");
        outStr = outStr.replace("&gt;", ">");
        outStr = outStr.replace("&apos;", "\'");
        outStr = outStr.replace("&quot;", "\"");

        return outStr;
    }
}
