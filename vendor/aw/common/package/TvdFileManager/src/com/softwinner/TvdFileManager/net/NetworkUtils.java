package com.softwinner.TvdFileManager.net;

import com.android.net.module.util.Inet4AddressUtils;
import java.lang.IllegalArgumentException;
import java.net.Inet4Address;

public class NetworkUtils {

    public static int prefixLengthToNetmaskInt(int prefixLength) throws IllegalArgumentException {
        return Inet4AddressUtils.prefixLengthToV4NetmaskIntHTL(prefixLength);
    }

    public static int inetAddressToInt(Inet4Address inetAddr)
            throws IllegalArgumentException {
        return Inet4AddressUtils.inet4AddressToIntHTL(inetAddr);
    }

}