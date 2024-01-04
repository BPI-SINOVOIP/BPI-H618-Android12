
package com.android.tv.launcher.device.bluetooth;

import java.util.Comparator;


public class BluetoothDeviceComparator implements Comparator<CachedBluetoothDevice> {

    @Override
    public int compare(CachedBluetoothDevice o1, CachedBluetoothDevice o2) {
        short rssi1 = o1.getRssi();
        short rssi2 = o2.getRssi();

        if (rssi1 < rssi2)
            return 1;
        else if (rssi1 > rssi2)
            return -1;
        else
            return 0;
    }

}
