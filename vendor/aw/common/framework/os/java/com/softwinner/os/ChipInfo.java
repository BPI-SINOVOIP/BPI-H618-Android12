
package com.softwinner.os;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class ChipInfo {
    
    private static final String CPU_INFO = "/proc/cpuinfo";
    private static final String SERIAL = "serial";
    private static final String SPLIT = ":";

    /**
     * Get the Chip ID in string of hex
     * @return the chip id string of hex
     */
    public static String getChipIDHex() {
        
        String chipSerial = null;
        FileReader fr = null;
        BufferedReader br = null;
        try {
            fr = new FileReader(CPU_INFO);
            br = new BufferedReader(fr);

            String readline = null;

            while ((readline = br.readLine()) != null) {
                if (readline.trim().toLowerCase().startsWith(SERIAL)) {
                    chipSerial = readline;
                    break;
                }
            }
        } catch (IOException io) {
            return null;
        } finally {
            if (fr != null) {
                try {
                    fr.close();
                } catch (IOException e) {
                }
            }
            if (br != null) {
                try {
                    br.close();
                } catch (IOException e) {
                }
            }
        }

        String[] serialSplit = chipSerial != null ? chipSerial.split(SPLIT) : null;
        if (serialSplit != null &&  serialSplit.length == 2) {
            return serialSplit[1].trim();
        }
        return null;
    }

    /**
     * Get the Chip ID in string of 128 bits
     * @return the chip id string of 128 bits
     */
    public static String getChipID() {
        StringBuilder chipId = new StringBuilder();
        int intValue = 0;

        try {
            String hexString = getChipIDHex();
            int hexLen = hexString.length();
            for (int i = 0; i < hexLen; i++) {
                int k;
                intValue = Integer.parseInt(hexString.substring(i, i + 1), 16);

                k = (intValue & 8) >> 3;
                chipId.append(k);

                k = (intValue & 4) >> 2;
                chipId.append(k);

                k = (intValue & 2) >> 1;
                chipId.append(k);

                k = intValue & 1;
                chipId.append(k);
            }
        } catch (Exception e) {
            return null;
        }

        return chipId.toString();
    }
}
