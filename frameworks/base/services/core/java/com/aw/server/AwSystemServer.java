/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.aw.server;

// AW:Added for BOOTEVENT
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import android.util.Log;
import android.os.SystemProperties;

public final class AwSystemServer {
    private static AwSystemServer sAwSystemServer;
    public static AwSystemServer getInstance() {
        synchronized (AwSystemServer.class) {
            if (sAwSystemServer == null) {
                sAwSystemServer = new AwSystemServer();
            }
            return sAwSystemServer;
        }
    }
    // AW: Added for BOOTEVENT
    private boolean sBootEventenable = SystemProperties.getBoolean("persist.sys.bootevent", true);
    public void logBootEvent(String bootevent) {
        if (!sBootEventenable) {
            return ;
        }
        FileOutputStream fos =null;
        try {
            fos = new FileOutputStream("/proc/bootevent");
            fos.write(bootevent.getBytes());
            fos.flush();
        } catch (FileNotFoundException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent,not found!",e);
        } catch (java.io.IOException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent entry",e);
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    Log.e ("BOOTEVENT","Failure close /proc/bootevent entry",e);
                }
            }
        }
    }
}
