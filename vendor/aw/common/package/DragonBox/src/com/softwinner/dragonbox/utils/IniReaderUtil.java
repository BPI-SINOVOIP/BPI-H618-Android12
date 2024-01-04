package com.softwinner.dragonbox.utils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import android.util.Log;

/**
 * Simple reader methods for ini formated file using java Using UTF-8 encoding.
 * Support DBCS, such as Chinese, Japanese
 *
 * @author Wenjun Yang
 * @email yang.rangerwolf@gmail.com 2011-10-30
 *
 */
public class IniReaderUtil {
    // section item value
    private static Map<String, HashMap<String, String>> sectionsMap = new HashMap<String, HashMap<String, String>>();
    // item value
    private static HashMap<String, String> itemsMap = new HashMap<String, String>();

    private static String currentSection = "";

    /**
     * Load data from target file
     *
     * @param file
     *            target file. It should be in ini format
     */
    private static void loadData(File file) {
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if ("".equals(line))
                    continue;
                if (line.startsWith("[") && line.endsWith("]")) {
                    // Ends last section
                    if (itemsMap.size() > 0
                            && !"".equals(currentSection.trim())) {
                        sectionsMap.put(currentSection, itemsMap);
                    }
                    currentSection = "";
                    itemsMap = null;

                    // Start new section initial
                    currentSection = line.substring(1, line.length() - 1);
                    itemsMap = new HashMap<String, String>();
                } else {
                    int index = line.indexOf("=");
                    if (index != -1) {
                        String key = line.substring(0, index);
                        String value = line.substring(index + 1, line.length());
                        itemsMap.put(key, value);
                    }
                }
            }

            sectionsMap.put(currentSection, itemsMap);
            reader.close();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
            }
        }
    }

    public static String getValue(String section, String item, File file) {
        loadData(file);

        HashMap<String, String> map = sectionsMap.get(section);
        if (map == null) {
            return "" + section;
        }
        String value = map.get(item);
        if (value == null) {
            return "" + item;
        }
        return value;
    }

    public static String getValue(String section, String item, String fileName) {
        File file = new File(fileName);
        return getValue(section, item, file);
    }

    public static List<String> getSectionNames(File file) {
        List<String> list = new ArrayList<String>();
        loadData(file);
        Set<String> key = sectionsMap.keySet();
        for (Iterator<String> it = key.iterator(); it.hasNext();) {
            list.add(it.next());
        }
        return list;
    }

    public static Map<String, String> getItemsBySectionName(String section,
                                                            File file) {
        loadData(file);
        return sectionsMap.get(section);
    }
}
