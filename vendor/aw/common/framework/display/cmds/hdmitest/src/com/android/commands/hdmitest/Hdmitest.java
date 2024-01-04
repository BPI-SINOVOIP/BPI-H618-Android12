/*
**
** Copyright 2013, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/


package com.android.commands.hdmitest;

import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.AndroidException;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.IWindowManager;
import com.android.internal.os.BaseCommand;

import java.io.PrintStream;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.os.IBinder;
import android.os.DisplayOutputManager;

public class Hdmitest extends BaseCommand {

    private DisplayOutputManager mManager;

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        (new Hdmitest()).run(args);
    }

    @Override
    public void onShowUsage(PrintStream out) {
        out.println("====");
    }

    @Override
    public void onRun() throws Exception {

        mManager = new DisplayOutputManager();

        String op = nextArgRequired();

        if (op.equals("set")) {
            runSetAutoMode();
        } else if (op.equals("dump")) {
            runDumpInfo();
        }
    }

    private void runSetAutoMode() throws Exception {
        mManager.setDisplayOutputMode(Display.DEFAULT_DISPLAY, 255);
        return;
    }

    private void runDumpInfo() throws Exception {
        DisplayOutputManager.SinkInfo info = mManager.getSinkInfo(Display.DEFAULT_DISPLAY);
        System.err.println("Type: " + info.Type);
        System.err.println("SupportedModes: " + info.SupportedModes.length);
        System.err.println("Current Mode: " + info.CurrentMode);
        System.err.println("User setting: " + info.UserSetting);
        System.err.println("Is Native: " + info.IsNative);
    }
}
