/*
**
** Copyright 2021, AllWinner
**
*/


package com.android.commands.dispctl;

import android.content.Context;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.AndroidException;
import android.view.IWindowManager;
import com.android.internal.os.BaseCommand;

import java.io.PrintStream;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import vendor.display.DisplayOutputManager;

public class dispdclient extends BaseCommand {

    private DisplayOutputManager mClient;

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        (new dispdclient()).run(args);
    }

    @Override
    public void onShowUsage(PrintStream out) {
        out.println(
                "usage: displayoutput-test [subcommand] [options]\n" +
                "       displayoutput-test print-modes\n" +
                "       displayoutput-test print-snr\n"   +
                "       displayoutput-test config-snr en y u v\n" +
                "       displayoutput-test hdcp <enable|disable|query>\n" +
                "\n"
                );
    }

    @Override
    public void onRun() throws Exception {
        mClient = DisplayOutputManager.getInstance();

        String op = nextArgRequired();

        if (op.equals("print-modes")) {
            runPrintModes();
        } else if (op.equals("print-snr")) {
            runPrintSNRInfo();
        } else if (op.equals("config-snr")) {
            runConfigSNRInfo();
        } else if (op.equals("hdcp")) {
            runHdcpCmds();
        }
    }

    private void runPrintModes() throws Exception {
        int[] supported = mClient.getSupportModes(0,
                DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI);
        System.out.println("Support modes count: " + supported.length);
        for (int i = 0; i < supported.length; i++) {
            System.out.println("  - " + supported[i]);
        }
    }

    private void runPrintSNRInfo() throws Exception {
        boolean supported = mClient.supportedSNRSetting(0);
        int mode = mClient.getSNRFeatureMode(0);
        int[] strength = mClient.getSNRStrength(0);
        System.out.println("supportedSNRSetting: " + supported);
        System.out.println("SNR Feature mode: " + mode);
        System.out.println("SNR strength: " + strength[0] + " - " +
                strength[1] + " - " + strength[2]);
    }

    private void runConfigSNRInfo() throws Exception {
        int args[] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            String s = nextArgRequired();
            if (s == null) break;
            args[i] = Integer.parseInt(s);
        }
        int ret = mClient.setSNRConfig(0, args[0], args[1], args[2], args[3]);
        System.out.println("setSNRConfig return: " + ret);
        System.out.println("SNR config: " + args[0] + " : " +
                args[1] + " - " + args[2] + " - " + args[3]);
    }

    private void runHdcpCmds() throws Exception {
        String option = nextArgRequired();
        if (option.equals("enable")) {
            int ret = mClient.configHdcp(true);
            System.out.println("enable hdcp return: " + ret);
        } else if (option.equals("disable")) {
            int ret = mClient.configHdcp(false);
            System.out.println("disable hdcp return: " + ret);
        } else if (option.equals("query")) {
            int level = mClient.getConnectedHdcpLevel();
            int state = mClient.getAuthorizedStatus();
            System.out.println("hdcp protocol: " + level);
            System.out.println("hdcp authorized state: " + state);
        }
    }
}
