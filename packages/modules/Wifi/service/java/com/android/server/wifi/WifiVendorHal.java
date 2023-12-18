/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.server.wifi;

import static android.net.wifi.CoexUnsafeChannel.POWER_CAP_NONE;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.wifi.V1_0.IWifiApIface;
import android.hardware.wifi.V1_0.IWifiChip;
import android.hardware.wifi.V1_0.IWifiChipEventCallback;
import android.hardware.wifi.V1_0.IWifiIface;
import android.hardware.wifi.V1_0.IWifiStaIface;
import android.hardware.wifi.V1_0.IWifiStaIfaceEventCallback;
import android.hardware.wifi.V1_0.IfaceType;
import android.hardware.wifi.V1_0.StaBackgroundScanBucketEventReportSchemeMask;
import android.hardware.wifi.V1_0.StaBackgroundScanBucketParameters;
import android.hardware.wifi.V1_0.StaBackgroundScanParameters;
import android.hardware.wifi.V1_0.StaLinkLayerIfaceStats;
import android.hardware.wifi.V1_0.StaLinkLayerRadioStats;
import android.hardware.wifi.V1_0.StaLinkLayerStats;
import android.hardware.wifi.V1_0.StaRoamingConfig;
import android.hardware.wifi.V1_0.StaRoamingState;
import android.hardware.wifi.V1_0.StaScanData;
import android.hardware.wifi.V1_0.StaScanDataFlagMask;
import android.hardware.wifi.V1_0.StaScanResult;
import android.hardware.wifi.V1_0.WifiDebugHostWakeReasonStats;
import android.hardware.wifi.V1_0.WifiDebugPacketFateFrameType;
import android.hardware.wifi.V1_0.WifiDebugRingBufferFlags;
import android.hardware.wifi.V1_0.WifiDebugRingBufferStatus;
import android.hardware.wifi.V1_0.WifiDebugRxPacketFate;
import android.hardware.wifi.V1_0.WifiDebugRxPacketFateReport;
import android.hardware.wifi.V1_0.WifiDebugTxPacketFate;
import android.hardware.wifi.V1_0.WifiDebugTxPacketFateReport;
import android.hardware.wifi.V1_0.WifiInformationElement;
import android.hardware.wifi.V1_0.WifiStatus;
import android.hardware.wifi.V1_0.WifiStatusCode;
import android.hardware.wifi.V1_2.IWifiChipEventCallback.IfaceInfo;
import android.hardware.wifi.V1_5.IWifiChip.MultiStaUseCase;
import android.hardware.wifi.V1_5.IWifiChip.UsableChannelFilter;
import android.hardware.wifi.V1_5.StaPeerInfo;
import android.hardware.wifi.V1_5.StaRateStat;
import android.hardware.wifi.V1_5.WifiBand;
import android.hardware.wifi.V1_5.WifiIfaceMode;
import android.hardware.wifi.V1_5.WifiUsableChannel;
import android.net.MacAddress;
import android.net.apf.ApfCapabilities;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiAvailableChannel;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiSsid;
import android.os.Handler;
import android.os.RemoteException;
import android.os.WorkSource;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.util.SparseArray;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.HexDump;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.HalDeviceManager.InterfaceDestroyedListener;
import com.android.server.wifi.WifiLinkLayerStats.ChannelStats;
import com.android.server.wifi.WifiLinkLayerStats.PeerInfo;
import com.android.server.wifi.WifiLinkLayerStats.RadioStat;
import com.android.server.wifi.WifiLinkLayerStats.RateStat;
import com.android.server.wifi.WifiNative.RxFateReport;
import com.android.server.wifi.WifiNative.TxFateReport;
import com.android.server.wifi.util.BitMask;
import com.android.server.wifi.util.GeneralUtil.Mutable;
import com.android.server.wifi.util.NativeUtil;
import com.android.wifi.resources.R;

import com.google.errorprone.annotations.CompileTimeConstant;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Vendor HAL via HIDL
 */
public class WifiVendorHal {

    private static final WifiLog sNoLog = new FakeWifiLog();

    /**
     * Chatty logging should use mVerboseLog
     */
    @VisibleForTesting
    WifiLog mVerboseLog = sNoLog;

    /**
     * Errors should use mLog
     */
    @VisibleForTesting
    WifiLog mLog = new LogcatLog("WifiVendorHal");

    /**
     * Enables or disables verbose logging
     *
     * @param verbose - with the obvious interpretation
     */
    public void enableVerboseLogging(boolean verbose) {
        synchronized (sLock) {
            if (verbose) {
                mVerboseLog = mLog;
                enter("verbose=true").flush();
            } else {
                enter("verbose=false").flush();
                mVerboseLog = sNoLog;
            }
        }
    }

    /**
     * Checks for a successful status result.
     *
     * Failures are logged to mLog.
     *
     * @param status is the WifiStatus generated by a hal call
     * @return true for success, false for failure
     */
    private boolean ok(WifiStatus status) {
        if (status.code == WifiStatusCode.SUCCESS) return true;

        Thread cur = Thread.currentThread();
        StackTraceElement[] trace = cur.getStackTrace();

        mLog.err("% failed %")
                .c(niceMethodName(trace, 3))
                .c(status.toString())
                .flush();

        return false;
    }

    /**
     * Logs the argument along with the method name.
     *
     * Always returns its argument.
     */
    private boolean boolResult(boolean result) {
        if (mVerboseLog == sNoLog) return result;
        // Currently only seen if verbose logging is on

        Thread cur = Thread.currentThread();
        StackTraceElement[] trace = cur.getStackTrace();

        mVerboseLog.err("% returns %")
                .c(niceMethodName(trace, 3))
                .c(result)
                .flush();

        return result;
    }

    private <T> T objResult(T obj) {
        if (mVerboseLog == sNoLog) return obj;
        // Currently only seen if verbose logging is on

        Thread cur = Thread.currentThread();
        StackTraceElement[] trace = cur.getStackTrace();

        mVerboseLog.err("% returns %")
                .c(niceMethodName(trace, 3))
                .c(String.valueOf(obj))
                .flush();

        return obj;
    }

    /**
     * Logs the argument along with the method name.
     *
     * Always returns its argument.
     */
    private <T> T nullResult() {
        if (mVerboseLog == sNoLog) return null;
        // Currently only seen if verbose logging is on

        Thread cur = Thread.currentThread();
        StackTraceElement[] trace = cur.getStackTrace();

        mVerboseLog.err("% returns %")
                .c(niceMethodName(trace, 3))
                .c(null)
                .flush();

        return null;
    }

    /**
     * Logs the argument along with the method name.
     *
     * Always returns its argument.
     */
    private byte[] byteArrayResult(byte[] result) {
        if (mVerboseLog == sNoLog) return result;
        // Currently only seen if verbose logging is on

        Thread cur = Thread.currentThread();
        StackTraceElement[] trace = cur.getStackTrace();

        mVerboseLog.err("% returns %")
                .c(niceMethodName(trace, 3))
                .c(result == null ? "(null)" : HexDump.dumpHexString(result))
                .flush();

        return result;
    }

    /**
     * Logs at method entry
     *
     * @param format string with % placeholders
     * @return LogMessage formatter (remember to .flush())
     */
    private WifiLog.LogMessage enter(@CompileTimeConstant final String format) {
        if (mVerboseLog == sNoLog) return sNoLog.info(format);
        return mVerboseLog.trace(format, 1);
    }

    /**
     * Gets the method name and line number from a stack trace.
     *
     * Attempts to skip frames created by lambdas to get a human-sensible name.
     *
     * @param trace, fo example obtained by Thread.currentThread().getStackTrace()
     * @param start  frame number to log, typically 3
     * @return string containing the method name and line number
     */
    private static String niceMethodName(StackTraceElement[] trace, int start) {
        if (start >= trace.length) return "";
        StackTraceElement s = trace[start];
        String name = s.getMethodName();
        if (name.contains("lambda$")) {
            // Try to find a friendlier method name
            String myFile = s.getFileName();
            if (myFile != null) {
                for (int i = start + 1; i < trace.length; i++) {
                    if (myFile.equals(trace[i].getFileName())) {
                        name = trace[i].getMethodName();
                        break;
                    }
                }
            }
        }
        return (name + "(l." + s.getLineNumber() + ")");
    }

    // Vendor HAL HIDL interface objects.
    private IWifiChip mIWifiChip;
    private HashMap<String, IWifiStaIface> mIWifiStaIfaces = new HashMap<>();
    private HashMap<String, IWifiApIface> mIWifiApIfaces = new HashMap<>();
    private static Context sContext;
    private final HalDeviceManager mHalDeviceManager;
    private final WifiGlobals mWifiGlobals;
    private final HalDeviceManagerStatusListener mHalDeviceManagerStatusCallbacks;
    private final IWifiStaIfaceEventCallback mIWifiStaIfaceEventCallback;
    private final ChipEventCallback mIWifiChipEventCallback;
    private final ChipEventCallbackV12 mIWifiChipEventCallbackV12;
    private final ChipEventCallbackV14 mIWifiChipEventCallbackV14;

    // Plumbing for event handling.
    //
    // Being final fields, they can be accessed without synchronization under
    // some reasonable assumptions. See
    // https://docs.oracle.com/javase/specs/jls/se7/html/jls-17.html#jls-17.5
    private final Handler mHalEventHandler;

    public WifiVendorHal(Context context, HalDeviceManager halDeviceManager, Handler handler,
            WifiGlobals wifiGlobals) {
        sContext = context;
        mHalDeviceManager = halDeviceManager;
        mHalEventHandler = handler;
        mWifiGlobals = wifiGlobals;
        mHalDeviceManagerStatusCallbacks = new HalDeviceManagerStatusListener();
        mIWifiStaIfaceEventCallback = new StaIfaceEventCallback();
        mIWifiChipEventCallback = new ChipEventCallback();
        mIWifiChipEventCallbackV12 = new ChipEventCallbackV12();
        mIWifiChipEventCallbackV14 = new ChipEventCallbackV14();
    }

    public static final Object sLock = new Object();

    private void handleRemoteException(RemoteException e) {
        String methodName = niceMethodName(Thread.currentThread().getStackTrace(), 3);
        mVerboseLog.err("% RemoteException in HIDL call %").c(methodName).c(e.toString()).flush();
        // Recovery on HAL crash will be triggered by death listener.
    }

    private WifiNative.VendorHalDeathEventHandler mDeathEventHandler;

    /**
     * Initialize the Hal device manager and register for status callbacks.
     *
     * @param handler Handler to notify if the vendor HAL dies.
     * @return true on success, false otherwise.
     */
    public boolean initialize(WifiNative.VendorHalDeathEventHandler handler) {
        synchronized (sLock) {
            mHalDeviceManager.initialize();
            mHalDeviceManager.registerStatusListener(
                    mHalDeviceManagerStatusCallbacks, mHalEventHandler);
            mDeathEventHandler = handler;
            return true;
        }
    }

    private WifiNative.VendorHalRadioModeChangeEventHandler mRadioModeChangeEventHandler;

    /**
     * Register to listen for radio mode change events from the HAL.
     *
     * @param handler Handler to notify when the vendor HAL detects a radio mode change.
     */
    public void registerRadioModeChangeHandler(
            WifiNative.VendorHalRadioModeChangeEventHandler handler) {
        synchronized (sLock) {
            mRadioModeChangeEventHandler = handler;
        }
    }

    /**
     * Register to listen for subsystem restart events from the HAL.
     *
     * @param listener SubsystemRestartListener listener object.
     */
    public void registerSubsystemRestartListener(
            HalDeviceManager.SubsystemRestartListener listener) {
        mHalDeviceManager.registerSubsystemRestartListener(listener, mHalEventHandler);
    }

    /**
     * Returns whether the vendor HAL is supported on this device or not.
     */
    public boolean isVendorHalSupported() {
        synchronized (sLock) {
            return mHalDeviceManager.isSupported();
        }
    }

    /**
     * Returns whether the vendor HAL is ready or not.
     */
    public boolean isVendorHalReady() {
        synchronized (sLock) {
            return mHalDeviceManager.isReady();
        }
    }

    /**
     * Bring up the HIDL Vendor HAL and configure for AP (Access Point) mode
     *
     * @return true for success
     */
    public boolean startVendorHalAp() {
        synchronized (sLock) {
            if (!startVendorHal()) {
                return false;
            }
            if (TextUtils.isEmpty(createApIface(null, null,
                    SoftApConfiguration.BAND_2GHZ, false))) {
                stopVendorHal();
                return false;
            }
            return true;
        }
    }

    /**
     * Bring up the HIDL Vendor HAL and configure for STA (Station) mode
     *
     * @return true for success
     */
    public boolean startVendorHalSta() {
        synchronized (sLock) {
            if (!startVendorHal()) {
                return false;
            }
            if (TextUtils.isEmpty(createStaIface(null, null))) {
                stopVendorHal();
                return false;
            }
            return true;
        }
    }

    /**
     * Bring up the HIDL Vendor HAL.
     * @return true on success, false otherwise.
     */
    public boolean startVendorHal() {
        synchronized (sLock) {
            if (!mHalDeviceManager.start()) {
                mLog.err("Failed to start vendor HAL").flush();
                return false;
            }
            mLog.info("Vendor Hal started successfully").flush();
            return true;
        }
    }


    /** Helper method to lookup the corresponding STA iface object using iface name. */
    private IWifiStaIface getStaIface(@NonNull String ifaceName) {
        synchronized (sLock) {
            return mIWifiStaIfaces.get(ifaceName);
        }
    }

    private class StaInterfaceDestroyedListenerInternal implements InterfaceDestroyedListener {
        private final InterfaceDestroyedListener mExternalListener;

        StaInterfaceDestroyedListenerInternal(InterfaceDestroyedListener externalListener) {
            mExternalListener = externalListener;
        }

        @Override
        public void onDestroyed(@NonNull String ifaceName) {
            synchronized (sLock) {
                mIWifiStaIfaces.remove(ifaceName);
            }
            if (mExternalListener != null) {
                mExternalListener.onDestroyed(ifaceName);
            }
        }
    }

    /**
     * Create a STA iface using {@link HalDeviceManager}.
     *
     * @param destroyedListener Listener to be invoked when the interface is destroyed.
     * @param requestorWs Requestor worksource.
     * @return iface name on success, null otherwise.
     */
    public String createStaIface(@Nullable InterfaceDestroyedListener destroyedListener,
            @NonNull WorkSource requestorWs) {
        synchronized (sLock) {
            IWifiStaIface iface = mHalDeviceManager.createStaIface(
                    new StaInterfaceDestroyedListenerInternal(destroyedListener), null,
                    requestorWs);
            if (iface == null) {
                mLog.err("Failed to create STA iface").flush();
                return nullResult();
            }
            String ifaceName = mHalDeviceManager.getName((IWifiIface) iface);
            if (TextUtils.isEmpty(ifaceName)) {
                mLog.err("Failed to get iface name").flush();
                return nullResult();
            }
            if (!registerStaIfaceCallback(iface)) {
                mLog.err("Failed to register STA iface callback").flush();
                return nullResult();
            }
            if (!retrieveWifiChip((IWifiIface) iface)) {
                mLog.err("Failed to get wifi chip").flush();
                return nullResult();
            }
            enableLinkLayerStats(iface);
            mIWifiStaIfaces.put(ifaceName, iface);
            return ifaceName;
        }
    }

    /**
     * Replace the requestor worksource info for a STA iface using {@link HalDeviceManager}.
     *
     * @param ifaceName Name of the interface being removed.
     * @param requestorWs Requestor worksource.
     * @return true on success, false otherwise.
     */
    public boolean replaceStaIfaceRequestorWs(@NonNull String ifaceName,
            @NonNull WorkSource requestorWs) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);

            if (!mHalDeviceManager.replaceRequestorWs(iface, requestorWs)) {
                mLog.err("Failed to replace requestor worksource for STA iface").flush();
                return boolResult(false);
            }
            return true;
        }
    }

    /**
     * Remove a STA iface using {@link HalDeviceManager}.
     *
     * @param ifaceName Name of the interface being removed.
     * @return true on success, false otherwise.
     */
    public boolean removeStaIface(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            if (!mHalDeviceManager.removeIface((IWifiIface) iface)) {
                mLog.err("Failed to remove STA iface").flush();
                return boolResult(false);
            }
            mIWifiStaIfaces.remove(ifaceName);
            return true;
        }
    }

    /** Helper method to lookup the corresponding AP iface object using iface name. */
    private IWifiApIface getApIface(@NonNull String ifaceName) {
        synchronized (sLock) {
            return mIWifiApIfaces.get(ifaceName);
        }
    }

    private class ApInterfaceDestroyedListenerInternal implements InterfaceDestroyedListener {
        private final InterfaceDestroyedListener mExternalListener;

        ApInterfaceDestroyedListenerInternal(InterfaceDestroyedListener externalListener) {
            mExternalListener = externalListener;
        }

        @Override
        public void onDestroyed(@NonNull String ifaceName) {
            synchronized (sLock) {
                mIWifiApIfaces.remove(ifaceName);
            }
            if (mExternalListener != null) {
                mExternalListener.onDestroyed(ifaceName);
            }
        }
    }

    private long getNecessaryCapabilitiesForSoftApMode(@SoftApConfiguration.BandType int band) {
        long caps = HalDeviceManager.CHIP_CAPABILITY_ANY;
        if ((band & SoftApConfiguration.BAND_60GHZ) != 0) {
            caps |= android.hardware.wifi.V1_5.IWifiChip.ChipCapabilityMask.WIGIG;
        }
        return caps;
    }

    /**
     * Create a AP iface using {@link HalDeviceManager}.
     *
     * @param destroyedListener Listener to be invoked when the interface is destroyed.
     * @param requestorWs Requestor worksource.
     * @param band The requesting band for this AP interface.
     * @param isBridged Whether or not AP interface is a bridge interface.
     * @return iface name on success, null otherwise.
     */
    public String createApIface(@Nullable InterfaceDestroyedListener destroyedListener,
            @NonNull WorkSource requestorWs,
            @SoftApConfiguration.BandType int band,
            boolean isBridged) {
        synchronized (sLock) {
            IWifiApIface iface = mHalDeviceManager.createApIface(
                    getNecessaryCapabilitiesForSoftApMode(band),
                    new ApInterfaceDestroyedListenerInternal(destroyedListener), null,
                    requestorWs, isBridged);
            if (iface == null) {
                mLog.err("Failed to create AP iface").flush();
                return nullResult();
            }
            String ifaceName = mHalDeviceManager.getName((IWifiIface) iface);
            if (TextUtils.isEmpty(ifaceName)) {
                mLog.err("Failed to get iface name").flush();
                return nullResult();
            }
            if (!retrieveWifiChip((IWifiIface) iface)) {
                mLog.err("Failed to get wifi chip").flush();
                return nullResult();
            }
            mIWifiApIfaces.put(ifaceName, iface);
            return ifaceName;
        }
    }

    /**
     * Replace the requestor worksource info for a AP iface using {@link HalDeviceManager}.
     *
     * @param ifaceName Name of the interface being removed.
     * @param requestorWs Requestor worksource.
     * @return true on success, false otherwise.
     */
    public boolean replaceApIfaceRequestorWs(@NonNull String ifaceName,
            @NonNull WorkSource requestorWs) {
        synchronized (sLock) {
            IWifiApIface iface = getApIface(ifaceName);
            if (iface == null) return boolResult(false);

            if (!mHalDeviceManager.replaceRequestorWs((IWifiIface) iface, requestorWs)) {
                mLog.err("Failed to replace requestor worksource for AP iface").flush();
                return boolResult(false);
            }
            return true;
        }
    }

    /**
     * Remove an AP iface using {@link HalDeviceManager}.
     *
     * @param ifaceName Name of the interface being removed.
     * @return true on success, false otherwise.
     */
    public boolean removeApIface(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiApIface iface = getApIface(ifaceName);
            if (iface == null) return boolResult(false);

            if (!mHalDeviceManager.removeIface((IWifiIface) iface)) {
                mLog.err("Failed to remove AP iface").flush();
                return boolResult(false);
            }
            mIWifiApIfaces.remove(ifaceName);
            return true;
        }
    }

    /**
     * Helper function to remove specific instance in bridged AP iface.
     *
     * @param ifaceName Name of the iface.
     * @param apIfaceInstance The identity of the ap instance.
     * @return true if the operation succeeded, false if there is an error in Hal.
     */
    public boolean removeIfaceInstanceFromBridgedApIface(@NonNull String ifaceName,
            @NonNull String apIfaceInstance) {
        try {
            android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
            if (iWifiChipV15 == null) return boolResult(false);
            return ok(iWifiChipV15.removeIfaceInstanceFromBridgedApIface(
                    ifaceName, apIfaceInstance));
        } catch (RemoteException e) {
            handleRemoteException(e);
            return false;
        }
    }

    @NonNull
    private ArrayList<android.hardware.wifi.V1_5.IWifiChip.CoexUnsafeChannel>
            frameworkCoexUnsafeChannelsToHidl(
                    @NonNull List<android.net.wifi.CoexUnsafeChannel> frameworkUnsafeChannels) {
        final ArrayList<android.hardware.wifi.V1_5.IWifiChip.CoexUnsafeChannel> hidlList =
                new ArrayList<>();
        if (!SdkLevel.isAtLeastS()) {
            return hidlList;
        }
        for (android.net.wifi.CoexUnsafeChannel frameworkUnsafeChannel : frameworkUnsafeChannels) {
            final android.hardware.wifi.V1_5.IWifiChip.CoexUnsafeChannel hidlUnsafeChannel =
                    new android.hardware.wifi.V1_5.IWifiChip.CoexUnsafeChannel();
            switch (frameworkUnsafeChannel.getBand()) {
                case (WifiScanner.WIFI_BAND_24_GHZ):
                    hidlUnsafeChannel.band = WifiBand.BAND_24GHZ;
                    break;
                case (WifiScanner.WIFI_BAND_5_GHZ):
                    hidlUnsafeChannel.band = WifiBand.BAND_5GHZ;
                    break;
                case (WifiScanner.WIFI_BAND_6_GHZ):
                    hidlUnsafeChannel.band = WifiBand.BAND_6GHZ;
                    break;
                case (WifiScanner.WIFI_BAND_60_GHZ):
                    hidlUnsafeChannel.band = WifiBand.BAND_60GHZ;
                    break;
                default:
                    mLog.err("Tried to set unsafe channel with unknown band: %")
                            .c(frameworkUnsafeChannel.getBand())
                            .flush();
                    continue;
            }
            hidlUnsafeChannel.channel = frameworkUnsafeChannel.getChannel();
            final int powerCapDbm = frameworkUnsafeChannel.getPowerCapDbm();
            if (powerCapDbm != POWER_CAP_NONE) {
                hidlUnsafeChannel.powerCapDbm = powerCapDbm;
            } else {
                hidlUnsafeChannel.powerCapDbm =
                        android.hardware.wifi.V1_5.IWifiChip.PowerCapConstant.NO_POWER_CAP;
            }
            hidlList.add(hidlUnsafeChannel);
        }
        return hidlList;
    }

    private int frameworkCoexRestrictionsToHidl(@WifiManager.CoexRestriction int restrictions) {
        int hidlRestrictions = 0;
        if (!SdkLevel.isAtLeastS()) {
            return hidlRestrictions;
        }
        if ((restrictions & WifiManager.COEX_RESTRICTION_WIFI_DIRECT) != 0) {
            hidlRestrictions |= android.hardware.wifi.V1_5.IWifiChip.CoexRestriction.WIFI_DIRECT;
        }
        if ((restrictions & WifiManager.COEX_RESTRICTION_SOFTAP) != 0) {
            hidlRestrictions |= android.hardware.wifi.V1_5.IWifiChip.CoexRestriction.SOFTAP;
        }
        if ((restrictions & WifiManager.COEX_RESTRICTION_WIFI_AWARE) != 0) {
            hidlRestrictions |= android.hardware.wifi.V1_5.IWifiChip.CoexRestriction.WIFI_AWARE;
        }
        return hidlRestrictions;
    }

    /**
     * Set the current coex unsafe channels to avoid and their restrictions.
     * @param unsafeChannels List of {@link android.net.wifi.CoexUnsafeChannel} to avoid.
     * @param restrictions int containing a bitwise-OR combination of
     *                     {@link WifiManager.CoexRestriction}.
     * @return true if the operation succeeded, false if there is an error in Hal.
     */
    public boolean setCoexUnsafeChannels(
            @NonNull List<android.net.wifi.CoexUnsafeChannel> unsafeChannels, int restrictions) {
        try {
            android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
            if (iWifiChipV15 == null) return boolResult(false);
            return ok(iWifiChipV15.setCoexUnsafeChannels(
                    frameworkCoexUnsafeChannelsToHidl(unsafeChannels),
                    frameworkCoexRestrictionsToHidl(restrictions)));
        } catch (RemoteException e) {
            handleRemoteException(e);
            return false;
        }
    }

    private boolean retrieveWifiChip(IWifiIface iface) {
        synchronized (sLock) {
            boolean registrationNeeded = mIWifiChip == null;
            mIWifiChip = mHalDeviceManager.getChip(iface);
            if (mIWifiChip == null) {
                mLog.err("Failed to get the chip created for the Iface").flush();
                return false;
            }
            if (!registrationNeeded) {
                return true;
            }
            if (!registerChipCallback()) {
                mLog.err("Failed to register chip callback").flush();
                mIWifiChip = null;
                return false;
            }
            return true;
        }
    }

    /**
     * Registers the sta iface callback.
     */
    private boolean registerStaIfaceCallback(IWifiStaIface iface) {
        synchronized (sLock) {
            if (iface == null) return boolResult(false);
            if (mIWifiStaIfaceEventCallback == null) return boolResult(false);
            try {
                WifiStatus status =
                        iface.registerEventCallback(mIWifiStaIfaceEventCallback);
                return ok(status);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Registers the sta iface callback.
     */
    private boolean registerChipCallback() {
        synchronized (sLock) {
            if (mIWifiChip == null) return boolResult(false);
            try {
                WifiStatus status;
                android.hardware.wifi.V1_4.IWifiChip iWifiChipV14 = getWifiChipForV1_4Mockable();
                android.hardware.wifi.V1_2.IWifiChip iWifiChipV12 = getWifiChipForV1_2Mockable();

                if (iWifiChipV14 != null) {
                    status = iWifiChipV14.registerEventCallback_1_4(mIWifiChipEventCallbackV14);
                } else if (iWifiChipV12 != null) {
                    status = iWifiChipV12.registerEventCallback_1_2(mIWifiChipEventCallbackV12);
                } else {
                    status = mIWifiChip.registerEventCallback(mIWifiChipEventCallback);
                }
                return ok(status);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Stops the HAL
     */
    public void stopVendorHal() {
        synchronized (sLock) {
            mHalDeviceManager.stop();
            clearState();
            mLog.info("Vendor Hal stopped").flush();
        }
    }

    /**
     * Clears the state associated with a started Iface
     *
     * Caller should hold the lock.
     */
    private void clearState() {
        mIWifiChip = null;
        mIWifiStaIfaces.clear();
        mIWifiApIfaces.clear();
        mDriverDescription = null;
        mFirmwareDescription = null;
    }

    /**
     * Tests whether the HAL is started and atleast one iface is up.
     */
    public boolean isHalStarted() {
        // For external use only. Methods in this class should test for null directly.
        synchronized (sLock) {
            return (!mIWifiStaIfaces.isEmpty() || !mIWifiApIfaces.isEmpty());
        }
    }

    /**
     * Gets the scan capabilities
     *
     * @param ifaceName Name of the interface.
     * @param capabilities object to be filled in
     * @return true for success, false for failure
     */
    public boolean getBgScanCapabilities(
            @NonNull String ifaceName, WifiNative.ScanCapabilities capabilities) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            try {
                Mutable<Boolean> ans = new Mutable<>(false);
                WifiNative.ScanCapabilities out = capabilities;
                iface.getBackgroundScanCapabilities((status, cap) -> {
                            if (!ok(status)) return;
                            mVerboseLog.info("scan capabilities %").c(cap.toString()).flush();
                            out.max_scan_cache_size = cap.maxCacheSize;
                            out.max_ap_cache_per_scan = cap.maxApCachePerScan;
                            out.max_scan_buckets = cap.maxBuckets;
                            out.max_rssi_sample_size = 0;
                            out.max_scan_reporting_threshold = cap.maxReportingThreshold;
                            ans.value = true;
                        }
                );
                return ans.value;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Holds the current background scan state, to implement pause and restart
     */
    @VisibleForTesting
    class CurrentBackgroundScan {
        public int cmdId;
        public StaBackgroundScanParameters param;
        public WifiNative.ScanEventHandler eventHandler = null;
        public boolean paused = false;
        public WifiScanner.ScanData[] latestScanResults = null;

        CurrentBackgroundScan(int id, WifiNative.ScanSettings settings) {
            cmdId = id;
            param = new StaBackgroundScanParameters();
            param.basePeriodInMs = settings.base_period_ms;
            param.maxApPerScan = settings.max_ap_per_scan;
            param.reportThresholdPercent = settings.report_threshold_percent;
            param.reportThresholdNumScans = settings.report_threshold_num_scans;
            if (settings.buckets != null) {
                for (WifiNative.BucketSettings bs : settings.buckets) {
                    param.buckets.add(makeStaBackgroundScanBucketParametersFromBucketSettings(bs));
                }
            }
        }
    }

    /**
     * Makes the Hal flavor of WifiNative.BucketSettings
     *
     * @param bs WifiNative.BucketSettings
     * @return Hal flavor of bs
     * @throws IllegalArgumentException if band value is not recognized
     */
    private StaBackgroundScanBucketParameters
            makeStaBackgroundScanBucketParametersFromBucketSettings(WifiNative.BucketSettings bs) {
        StaBackgroundScanBucketParameters pa = new StaBackgroundScanBucketParameters();
        pa.bucketIdx = bs.bucket;
        pa.band = makeWifiBandFromFrameworkBand(bs.band);
        if (bs.channels != null) {
            for (WifiNative.ChannelSettings cs : bs.channels) {
                pa.frequencies.add(cs.frequency);
            }
        }
        pa.periodInMs = bs.period_ms;
        pa.eventReportScheme = makeReportSchemeFromBucketSettingsReportEvents(bs.report_events);
        pa.exponentialMaxPeriodInMs = bs.max_period_ms;
        // Although HAL API allows configurable base value for the truncated
        // exponential back off scan. Native API and above support only
        // truncated binary exponential back off scan.
        // Hard code value of base to 2 here.
        pa.exponentialBase = 2;
        pa.exponentialStepCount = bs.step_count;
        return pa;
    }

    /**
     * Makes the Hal flavor of WifiScanner's band indication
     *
     * Note: This method is only used by background scan which does not
     *       support 6GHz, hence band combinations including 6GHz are considered invalid
     *
     * @param frameworkBand one of WifiScanner.WIFI_BAND_*
     * @return A WifiBand value
     * @throws IllegalArgumentException if frameworkBand is not recognized
     */
    private int makeWifiBandFromFrameworkBand(int frameworkBand) {
        switch (frameworkBand) {
            case WifiScanner.WIFI_BAND_UNSPECIFIED:
                return WifiBand.BAND_UNSPECIFIED;
            case WifiScanner.WIFI_BAND_24_GHZ:
                return WifiBand.BAND_24GHZ;
            case WifiScanner.WIFI_BAND_5_GHZ:
                return WifiBand.BAND_5GHZ;
            case WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY:
                return WifiBand.BAND_5GHZ_DFS;
            case WifiScanner.WIFI_BAND_5_GHZ_WITH_DFS:
                return WifiBand.BAND_5GHZ_WITH_DFS;
            case WifiScanner.WIFI_BAND_BOTH:
                return WifiBand.BAND_24GHZ_5GHZ;
            case WifiScanner.WIFI_BAND_BOTH_WITH_DFS:
                return WifiBand.BAND_24GHZ_5GHZ_WITH_DFS;
            case WifiScanner.WIFI_BAND_6_GHZ:
                return WifiBand.BAND_6GHZ;
            case WifiScanner.WIFI_BAND_24_5_6_GHZ:
                return WifiBand.BAND_24GHZ_5GHZ_6GHZ;
            case WifiScanner.WIFI_BAND_24_5_WITH_DFS_6_GHZ:
                return WifiBand.BAND_24GHZ_5GHZ_WITH_DFS_6GHZ;
            case WifiScanner.WIFI_BAND_60_GHZ:
                return WifiBand.BAND_60GHZ;
            case WifiScanner.WIFI_BAND_24_5_6_60_GHZ:
                return WifiBand.BAND_24GHZ_5GHZ_6GHZ_60GHZ;
            case WifiScanner.WIFI_BAND_24_5_WITH_DFS_6_60_GHZ:
                return WifiBand.BAND_24GHZ_5GHZ_WITH_DFS_6GHZ_60GHZ;
            case WifiScanner.WIFI_BAND_24_GHZ_WITH_5GHZ_DFS:
            default:
                throw new IllegalArgumentException("bad band " + frameworkBand);
        }
    }

    /**
     * Makes the Hal flavor of WifiScanner's report event mask
     *
     * @param reportUnderscoreEvents is logical OR of WifiScanner.REPORT_EVENT_* values
     * @return Corresponding StaBackgroundScanBucketEventReportSchemeMask value
     * @throws IllegalArgumentException if a mask bit is not recognized
     */
    private int makeReportSchemeFromBucketSettingsReportEvents(int reportUnderscoreEvents) {
        int ans = 0;
        BitMask in = new BitMask(reportUnderscoreEvents);
        if (in.testAndClear(WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN)) {
            ans |= StaBackgroundScanBucketEventReportSchemeMask.EACH_SCAN;
        }
        if (in.testAndClear(WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT)) {
            ans |= StaBackgroundScanBucketEventReportSchemeMask.FULL_RESULTS;
        }
        if (in.testAndClear(WifiScanner.REPORT_EVENT_NO_BATCH)) {
            ans |= StaBackgroundScanBucketEventReportSchemeMask.NO_BATCH;
        }
        if (in.value != 0) throw new IllegalArgumentException("bad " + reportUnderscoreEvents);
        return ans;
    }

    private int mLastScanCmdId; // For assigning cmdIds to scans

    @VisibleForTesting
    CurrentBackgroundScan mScan = null;

    /**
     * Starts a background scan
     *
     * Any ongoing scan will be stopped first
     *
     * @param ifaceName    Name of the interface.
     * @param settings     to control the scan
     * @param eventHandler to call with the results
     * @return true for success
     */
    public boolean startBgScan(@NonNull String ifaceName,
                               WifiNative.ScanSettings settings,
                               WifiNative.ScanEventHandler eventHandler) {
        WifiStatus status;
        if (eventHandler == null) return boolResult(false);
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            try {
                if (mScan != null && !mScan.paused) {
                    ok(iface.stopBackgroundScan(mScan.cmdId));
                    mScan = null;
                }
                mLastScanCmdId = (mLastScanCmdId % 9) + 1; // cycle through non-zero single digits
                CurrentBackgroundScan scan = new CurrentBackgroundScan(mLastScanCmdId, settings);
                status = iface.startBackgroundScan(scan.cmdId, scan.param);
                if (!ok(status)) return false;
                scan.eventHandler = eventHandler;
                mScan = scan;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }


    /**
     * Stops any ongoing backgound scan
     *
     * @param ifaceName Name of the interface.
     */
    public void stopBgScan(@NonNull String ifaceName) {
        WifiStatus status;
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return;
            try {
                if (mScan != null) {
                    ok(iface.stopBackgroundScan(mScan.cmdId));
                    mScan = null;
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
            }
        }
    }

    /**
     * Pauses an ongoing backgound scan
     *
     * @param ifaceName Name of the interface.
     */
    public void pauseBgScan(@NonNull String ifaceName) {
        WifiStatus status;
        synchronized (sLock) {
            try {
                IWifiStaIface iface = getStaIface(ifaceName);
                if (iface == null) return;
                if (mScan != null && !mScan.paused) {
                    status = iface.stopBackgroundScan(mScan.cmdId);
                    if (!ok(status)) return;
                    mScan.paused = true;
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
            }
        }
    }

    /**
     * Restarts a paused background scan
     *
     * @param ifaceName Name of the interface.
     */
    public void restartBgScan(@NonNull String ifaceName) {
        WifiStatus status;
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return;
            try {
                if (mScan != null && mScan.paused) {
                    status = iface.startBackgroundScan(mScan.cmdId, mScan.param);
                    if (!ok(status)) return;
                    mScan.paused = false;
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
            }
        }
    }

    /**
     * Gets the latest scan results received from the HIDL interface callback.
     * TODO(b/35754840): This hop to fetch scan results after callback is unnecessary. Refactor
     * WifiScanner to use the scan results from the callback.
     *
     * @param ifaceName Name of the interface.
     */
    public WifiScanner.ScanData[] getBgScanResults(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return null;
            if (mScan == null) return null;
            return mScan.latestScanResults;
        }
    }

    /**
     * Get the link layer statistics
     *
     * Note - we always enable link layer stats on a STA interface.
     *
     * @param ifaceName Name of the interface.
     * @return the statistics, or null if unable to do so
     */
    public WifiLinkLayerStats getWifiLinkLayerStats(@NonNull String ifaceName) {
        if (getWifiStaIfaceForV1_5Mockable(ifaceName) != null) {
            return getWifiLinkLayerStats_1_5_Internal(ifaceName);
        } else if (getWifiStaIfaceForV1_3Mockable(ifaceName) != null) {
            return getWifiLinkLayerStats_1_3_Internal(ifaceName);
        } else {
            return getWifiLinkLayerStats_internal(ifaceName);
        }
    }

    private WifiLinkLayerStats getWifiLinkLayerStats_internal(@NonNull String ifaceName) {
        class AnswerBox {
            public StaLinkLayerStats value = null;
        }
        AnswerBox answer = new AnswerBox();
        synchronized (sLock) {
            try {
                IWifiStaIface iface = getStaIface(ifaceName);
                if (iface == null) return null;
                iface.getLinkLayerStats((status, stats) -> {
                    if (!ok(status)) return;
                    answer.value = stats;
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        WifiLinkLayerStats stats = frameworkFromHalLinkLayerStats(answer.value);
        return stats;
    }

    private WifiLinkLayerStats getWifiLinkLayerStats_1_3_Internal(@NonNull String ifaceName) {
        class AnswerBox {
            public android.hardware.wifi.V1_3.StaLinkLayerStats value = null;
        }
        AnswerBox answer = new AnswerBox();
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_3.IWifiStaIface iface =
                        getWifiStaIfaceForV1_3Mockable(ifaceName);
                if (iface == null) return null;
                iface.getLinkLayerStats_1_3((status, stats) -> {
                    if (!ok(status)) return;
                    answer.value = stats;
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        WifiLinkLayerStats stats = frameworkFromHalLinkLayerStats_1_3(answer.value);
        return stats;
    }

    private WifiLinkLayerStats getWifiLinkLayerStats_1_5_Internal(@NonNull String ifaceName) {
        class AnswerBox {
            public android.hardware.wifi.V1_5.StaLinkLayerStats value = null;
        }
        AnswerBox answer = new AnswerBox();
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiStaIface iface =
                        getWifiStaIfaceForV1_5Mockable(ifaceName);
                if (iface == null) return null;
                iface.getLinkLayerStats_1_5((status, stats) -> {
                    if (!ok(status)) return;
                    answer.value = stats;
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        WifiLinkLayerStats stats = frameworkFromHalLinkLayerStats_1_5(answer.value);
        return stats;
    }


    /**
     * Makes the framework version of link layer stats from the hal version.
     */
    @VisibleForTesting
    static WifiLinkLayerStats frameworkFromHalLinkLayerStats(StaLinkLayerStats stats) {
        if (stats == null) return null;
        WifiLinkLayerStats out = new WifiLinkLayerStats();
        setIfaceStats(out, stats.iface);
        setRadioStats(out, stats.radios);
        setTimeStamp(out, stats.timeStampInMs);
        out.version = WifiLinkLayerStats.V1_0;
        return out;
    }

    /**
     * Makes the framework version of link layer stats from the hal version.
     */
    @VisibleForTesting
    static WifiLinkLayerStats frameworkFromHalLinkLayerStats_1_3(
            android.hardware.wifi.V1_3.StaLinkLayerStats stats) {
        if (stats == null) return null;
        WifiLinkLayerStats out = new WifiLinkLayerStats();
        setIfaceStats(out, stats.iface);
        setRadioStats_1_3(out, stats.radios);
        setTimeStamp(out, stats.timeStampInMs);
        out.version = WifiLinkLayerStats.V1_3;
        return out;
    }

    /**
     * Makes the framework version of link layer stats from the hal version.
     */
    @VisibleForTesting
    static WifiLinkLayerStats frameworkFromHalLinkLayerStats_1_5(
            android.hardware.wifi.V1_5.StaLinkLayerStats stats) {
        if (stats == null) return null;
        WifiLinkLayerStats out = new WifiLinkLayerStats();
        setIfaceStats_1_5(out, stats.iface);
        setRadioStats_1_5(out, stats.radios);
        setTimeStamp(out, stats.timeStampInMs);
        out.version = WifiLinkLayerStats.V1_5;
        return out;
    }

    private static void setIfaceStats(WifiLinkLayerStats stats, StaLinkLayerIfaceStats iface) {
        if (iface == null) return;
        stats.beacon_rx = iface.beaconRx;
        stats.rssi_mgmt = iface.avgRssiMgmt;
        // Statistics are broken out by Wireless Multimedia Extensions categories
        // WME Best Effort Access Category
        stats.rxmpdu_be = iface.wmeBePktStats.rxMpdu;
        stats.txmpdu_be = iface.wmeBePktStats.txMpdu;
        stats.lostmpdu_be = iface.wmeBePktStats.lostMpdu;
        stats.retries_be = iface.wmeBePktStats.retries;
        // WME Background Access Category
        stats.rxmpdu_bk = iface.wmeBkPktStats.rxMpdu;
        stats.txmpdu_bk = iface.wmeBkPktStats.txMpdu;
        stats.lostmpdu_bk = iface.wmeBkPktStats.lostMpdu;
        stats.retries_bk = iface.wmeBkPktStats.retries;
        // WME Video Access Category
        stats.rxmpdu_vi = iface.wmeViPktStats.rxMpdu;
        stats.txmpdu_vi = iface.wmeViPktStats.txMpdu;
        stats.lostmpdu_vi = iface.wmeViPktStats.lostMpdu;
        stats.retries_vi = iface.wmeViPktStats.retries;
        // WME Voice Access Category
        stats.rxmpdu_vo = iface.wmeVoPktStats.rxMpdu;
        stats.txmpdu_vo = iface.wmeVoPktStats.txMpdu;
        stats.lostmpdu_vo = iface.wmeVoPktStats.lostMpdu;
        stats.retries_vo = iface.wmeVoPktStats.retries;
    }

    private static void setIfaceStats_1_5(WifiLinkLayerStats stats,
            android.hardware.wifi.V1_5.StaLinkLayerIfaceStats iface) {
        if (iface == null) return;
        setIfaceStats(stats, iface.V1_0);
        stats.timeSliceDutyCycleInPercent = iface.timeSliceDutyCycleInPercent;
        // WME Best Effort Access Category
        stats.contentionTimeMinBeInUsec = iface.wmeBeContentionTimeStats.contentionTimeMinInUsec;
        stats.contentionTimeMaxBeInUsec = iface.wmeBeContentionTimeStats.contentionTimeMaxInUsec;
        stats.contentionTimeAvgBeInUsec = iface.wmeBeContentionTimeStats.contentionTimeAvgInUsec;
        stats.contentionNumSamplesBe = iface.wmeBeContentionTimeStats.contentionNumSamples;
        // WME Background Access Category
        stats.contentionTimeMinBkInUsec = iface.wmeBkContentionTimeStats.contentionTimeMinInUsec;
        stats.contentionTimeMaxBkInUsec = iface.wmeBkContentionTimeStats.contentionTimeMaxInUsec;
        stats.contentionTimeAvgBkInUsec = iface.wmeBkContentionTimeStats.contentionTimeAvgInUsec;
        stats.contentionNumSamplesBk = iface.wmeBkContentionTimeStats.contentionNumSamples;
        // WME Video Access Category
        stats.contentionTimeMinViInUsec = iface.wmeViContentionTimeStats.contentionTimeMinInUsec;
        stats.contentionTimeMaxViInUsec = iface.wmeViContentionTimeStats.contentionTimeMaxInUsec;
        stats.contentionTimeAvgViInUsec = iface.wmeViContentionTimeStats.contentionTimeAvgInUsec;
        stats.contentionNumSamplesVi = iface.wmeViContentionTimeStats.contentionNumSamples;
        // WME Voice Access Category
        stats.contentionTimeMinVoInUsec = iface.wmeVoContentionTimeStats.contentionTimeMinInUsec;
        stats.contentionTimeMaxVoInUsec = iface.wmeVoContentionTimeStats.contentionTimeMaxInUsec;
        stats.contentionTimeAvgVoInUsec = iface.wmeVoContentionTimeStats.contentionTimeAvgInUsec;
        stats.contentionNumSamplesVo = iface.wmeVoContentionTimeStats.contentionNumSamples;
        // Peer information statistics
        stats.peerInfo = new PeerInfo[iface.peers.size()];
        for (int i = 0; i < stats.peerInfo.length; i++) {
            PeerInfo peer = new PeerInfo();
            StaPeerInfo staPeerInfo = iface.peers.get(i);
            peer.staCount = staPeerInfo.staCount;
            peer.chanUtil = staPeerInfo.chanUtil;
            RateStat[] rateStats = new RateStat[staPeerInfo.rateStats.size()];
            for (int j = 0; j < staPeerInfo.rateStats.size(); j++) {
                rateStats[j] = new RateStat();
                StaRateStat staRateStat = staPeerInfo.rateStats.get(j);
                rateStats[j].preamble = staRateStat.rateInfo.preamble;
                rateStats[j].nss = staRateStat.rateInfo.nss;
                rateStats[j].bw = staRateStat.rateInfo.bw;
                rateStats[j].rateMcsIdx = staRateStat.rateInfo.rateMcsIdx;
                rateStats[j].bitRateInKbps = staRateStat.rateInfo.bitRateInKbps;
                rateStats[j].txMpdu = staRateStat.txMpdu;
                rateStats[j].rxMpdu = staRateStat.rxMpdu;
                rateStats[j].mpduLost = staRateStat.mpduLost;
                rateStats[j].retries = staRateStat.retries;
            }
            peer.rateStats = rateStats;
            stats.peerInfo[i] = peer;
        }
    }

    private static void setRadioStats(WifiLinkLayerStats stats,
            List<StaLinkLayerRadioStats> radios) {
        if (radios == null) return;
        // Do not coalesce this info for multi radio devices with older HALs.
        if (radios.size() > 0) {
            StaLinkLayerRadioStats radioStats = radios.get(0);
            stats.on_time = radioStats.onTimeInMs;
            stats.tx_time = radioStats.txTimeInMs;
            stats.tx_time_per_level = new int[radioStats.txTimeInMsPerLevel.size()];
            for (int i = 0; i < stats.tx_time_per_level.length; i++) {
                stats.tx_time_per_level[i] = radioStats.txTimeInMsPerLevel.get(i);
            }
            stats.rx_time = radioStats.rxTimeInMs;
            stats.on_time_scan = radioStats.onTimeInMsForScan;
            stats.numRadios = 1;
        }
    }

    /**
     * Set individual radio stats from the hal radio stats
     */
    private static void setFrameworkPerRadioStatsFromHidl(int radioId, RadioStat radio,
            android.hardware.wifi.V1_3.StaLinkLayerRadioStats hidlRadioStats) {
        radio.radio_id = radioId;
        radio.on_time = hidlRadioStats.V1_0.onTimeInMs;
        radio.tx_time = hidlRadioStats.V1_0.txTimeInMs;
        radio.rx_time = hidlRadioStats.V1_0.rxTimeInMs;
        radio.on_time_scan = hidlRadioStats.V1_0.onTimeInMsForScan;
        radio.on_time_nan_scan = hidlRadioStats.onTimeInMsForNanScan;
        radio.on_time_background_scan = hidlRadioStats.onTimeInMsForBgScan;
        radio.on_time_roam_scan = hidlRadioStats.onTimeInMsForRoamScan;
        radio.on_time_pno_scan = hidlRadioStats.onTimeInMsForPnoScan;
        radio.on_time_hs20_scan = hidlRadioStats.onTimeInMsForHs20Scan;
        /* Copy list of channel stats */
        for (android.hardware.wifi.V1_3.WifiChannelStats channelStats
                : hidlRadioStats.channelStats) {
            ChannelStats channelStatsEntry = new ChannelStats();
            channelStatsEntry.frequency = channelStats.channel.centerFreq;
            channelStatsEntry.radioOnTimeMs = channelStats.onTimeInMs;
            channelStatsEntry.ccaBusyTimeMs = channelStats.ccaBusyTimeInMs;
            radio.channelStatsMap.put(channelStats.channel.centerFreq, channelStatsEntry);
        }
    }

    /**
     * If config_wifiLinkLayerAllRadiosStatsAggregationEnabled is set to true, aggregate
     * the radio stats from all the radios else process the stats from Radio 0 only.
     */
    private static void aggregateFrameworkRadioStatsFromHidl(int radioIndex,
            WifiLinkLayerStats stats,
            android.hardware.wifi.V1_3.StaLinkLayerRadioStats hidlRadioStats) {
        if (!sContext.getResources()
                .getBoolean(R.bool.config_wifiLinkLayerAllRadiosStatsAggregationEnabled)
                && radioIndex > 0) {
            return;
        }
        // Aggregate the radio stats from all the radios
        stats.on_time += hidlRadioStats.V1_0.onTimeInMs;
        stats.tx_time += hidlRadioStats.V1_0.txTimeInMs;
        // Aggregate tx_time_per_level based on the assumption that the length of
        // txTimeInMsPerLevel is the same across all radios. So txTimeInMsPerLevel on other
        // radios at array indices greater than the length of first radio will be dropped.
        if (stats.tx_time_per_level == null) {
            stats.tx_time_per_level = new int[hidlRadioStats.V1_0.txTimeInMsPerLevel.size()];
        }
        for (int i = 0; i < hidlRadioStats.V1_0.txTimeInMsPerLevel.size()
                && i < stats.tx_time_per_level.length; i++) {
            stats.tx_time_per_level[i] += hidlRadioStats.V1_0.txTimeInMsPerLevel.get(i);
        }
        stats.rx_time += hidlRadioStats.V1_0.rxTimeInMs;
        stats.on_time_scan += hidlRadioStats.V1_0.onTimeInMsForScan;
        stats.on_time_nan_scan += hidlRadioStats.onTimeInMsForNanScan;
        stats.on_time_background_scan += hidlRadioStats.onTimeInMsForBgScan;
        stats.on_time_roam_scan += hidlRadioStats.onTimeInMsForRoamScan;
        stats.on_time_pno_scan += hidlRadioStats.onTimeInMsForPnoScan;
        stats.on_time_hs20_scan += hidlRadioStats.onTimeInMsForHs20Scan;
        /* Copy list of channel stats */
        for (android.hardware.wifi.V1_3.WifiChannelStats channelStats
                : hidlRadioStats.channelStats) {
            ChannelStats channelStatsEntry =
                    stats.channelStatsMap.get(channelStats.channel.centerFreq);
            if (channelStatsEntry == null) {
                channelStatsEntry = new ChannelStats();
                channelStatsEntry.frequency = channelStats.channel.centerFreq;
                stats.channelStatsMap.put(channelStats.channel.centerFreq, channelStatsEntry);
            }
            channelStatsEntry.radioOnTimeMs += channelStats.onTimeInMs;
            channelStatsEntry.ccaBusyTimeMs += channelStats.ccaBusyTimeInMs;
        }
        stats.numRadios++;
    }

    private static void setRadioStats_1_3(WifiLinkLayerStats stats,
            List<android.hardware.wifi.V1_3.StaLinkLayerRadioStats> radios) {
        if (radios == null) return;
        int radioIndex = 0;
        for (android.hardware.wifi.V1_3.StaLinkLayerRadioStats radioStats : radios) {
            aggregateFrameworkRadioStatsFromHidl(radioIndex, stats, radioStats);
            radioIndex++;
        }
    }

    private static void setRadioStats_1_5(WifiLinkLayerStats stats,
            List<android.hardware.wifi.V1_5.StaLinkLayerRadioStats> radios) {
        if (radios == null) return;
        int radioIndex = 0;
        stats.radioStats = new RadioStat[radios.size()];
        for (android.hardware.wifi.V1_5.StaLinkLayerRadioStats radioStats : radios) {
            RadioStat radio = new RadioStat();
            setFrameworkPerRadioStatsFromHidl(radioStats.radioId, radio, radioStats.V1_3);
            stats.radioStats[radioIndex] = radio;
            aggregateFrameworkRadioStatsFromHidl(radioIndex, stats, radioStats.V1_3);
            radioIndex++;
        }
    }

    private static void setTimeStamp(WifiLinkLayerStats stats, long timeStampInMs) {
        stats.timeStampInMs = timeStampInMs;
    }

    @VisibleForTesting
    boolean mLinkLayerStatsDebug = false;  // Passed to Hal

    /**
     * Enables the linkLayerStats in the Hal.
     *
     * This is called unconditionally whenever we create a STA interface.
     *
     * @param iface Iface object.
     */
    private void enableLinkLayerStats(IWifiStaIface iface) {
        synchronized (sLock) {
            try {
                WifiStatus status;
                status = iface.enableLinkLayerStatsCollection(mLinkLayerStatsDebug);
                if (!ok(status)) {
                    mLog.err("unable to enable link layer stats collection").flush();
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
            }
        }
    }

    /**
     * Translation table used by getSupportedFeatureSet for translating IWifiChip caps for V1.1
     */
    private static final long[][] sChipFeatureCapabilityTranslation = {
            {WifiManager.WIFI_FEATURE_TX_POWER_LIMIT,
                    android.hardware.wifi.V1_1.IWifiChip.ChipCapabilityMask.SET_TX_POWER_LIMIT
            },
            {WifiManager.WIFI_FEATURE_D2D_RTT,
                    android.hardware.wifi.V1_1.IWifiChip.ChipCapabilityMask.D2D_RTT
            },
            {WifiManager.WIFI_FEATURE_D2AP_RTT,
                    android.hardware.wifi.V1_1.IWifiChip.ChipCapabilityMask.D2AP_RTT
            }
    };

    /**
     * Translation table used by getSupportedFeatureSet for translating IWifiChip caps for
     * additional capabilities introduced in V1.5
     */
    private static final long[][] sChipFeatureCapabilityTranslation15 = {
            {WifiManager.WIFI_FEATURE_INFRA_60G,
                    android.hardware.wifi.V1_5.IWifiChip.ChipCapabilityMask.WIGIG
            }
    };

    /**
     * Translation table used by getSupportedFeatureSet for translating IWifiChip caps for
     * additional capabilities introduced in V1.3
     */
    private static final long[][] sChipFeatureCapabilityTranslation13 = {
            {WifiManager.WIFI_FEATURE_LOW_LATENCY,
                    android.hardware.wifi.V1_3.IWifiChip.ChipCapabilityMask.SET_LATENCY_MODE
            },
            {WifiManager.WIFI_FEATURE_P2P_RAND_MAC,
                    android.hardware.wifi.V1_3.IWifiChip.ChipCapabilityMask.P2P_RAND_MAC
            }

    };

    /**
     * Feature bit mask translation for Chip V1.1
     *
     * @param capabilities bitmask defined IWifiChip.ChipCapabilityMask
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    @VisibleForTesting
    int wifiFeatureMaskFromChipCapabilities(int capabilities) {
        int features = 0;
        for (int i = 0; i < sChipFeatureCapabilityTranslation.length; i++) {
            if ((capabilities & sChipFeatureCapabilityTranslation[i][1]) != 0) {
                features |= sChipFeatureCapabilityTranslation[i][0];
            }
        }
        return features;
    }

    /**
     * Feature bit mask translation for Chip V1.5
     *
     * @param capabilities bitmask defined IWifiChip.ChipCapabilityMask
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    @VisibleForTesting
    long wifiFeatureMaskFromChipCapabilities_1_5(int capabilities) {
        // First collect features from previous versions
        long features = wifiFeatureMaskFromChipCapabilities_1_3(capabilities);

        // Next collect features for V1_5 version
        for (int i = 0; i < sChipFeatureCapabilityTranslation15.length; i++) {
            if ((capabilities & sChipFeatureCapabilityTranslation15[i][1]) != 0) {
                features |= sChipFeatureCapabilityTranslation15[i][0];
            }
        }
        return features;
    }

    /**
     * Feature bit mask translation for Chip V1.3
     *
     * @param capabilities bitmask defined IWifiChip.ChipCapabilityMask
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    @VisibleForTesting
    long wifiFeatureMaskFromChipCapabilities_1_3(int capabilities) {
        // First collect features from previous versions
        long features = wifiFeatureMaskFromChipCapabilities(capabilities);

        // Next collect features for V1_3 version
        for (int i = 0; i < sChipFeatureCapabilityTranslation13.length; i++) {
            if ((capabilities & sChipFeatureCapabilityTranslation13[i][1]) != 0) {
                features |= sChipFeatureCapabilityTranslation13[i][0];
            }
        }
        return features;
    }

    /**
     * Translation table used by getSupportedFeatureSet for translating IWifiStaIface caps
     */
    private static final long[][] sStaFeatureCapabilityTranslation = {
            {WifiManager.WIFI_FEATURE_PASSPOINT,
                    IWifiStaIface.StaIfaceCapabilityMask.HOTSPOT
            },
            {WifiManager.WIFI_FEATURE_SCANNER,
                    IWifiStaIface.StaIfaceCapabilityMask.BACKGROUND_SCAN,
            },
            {WifiManager.WIFI_FEATURE_PNO,
                    IWifiStaIface.StaIfaceCapabilityMask.PNO
            },
            {WifiManager.WIFI_FEATURE_TDLS,
                    IWifiStaIface.StaIfaceCapabilityMask.TDLS
            },
            {WifiManager.WIFI_FEATURE_TDLS_OFFCHANNEL,
                    IWifiStaIface.StaIfaceCapabilityMask.TDLS_OFFCHANNEL
            },
            {WifiManager.WIFI_FEATURE_LINK_LAYER_STATS,
                    IWifiStaIface.StaIfaceCapabilityMask.LINK_LAYER_STATS
            },
            {WifiManager.WIFI_FEATURE_RSSI_MONITOR,
                    IWifiStaIface.StaIfaceCapabilityMask.RSSI_MONITOR
            },
            {WifiManager.WIFI_FEATURE_MKEEP_ALIVE,
                    IWifiStaIface.StaIfaceCapabilityMask.KEEP_ALIVE
            },
            {WifiManager.WIFI_FEATURE_CONFIG_NDO,
                    IWifiStaIface.StaIfaceCapabilityMask.ND_OFFLOAD
            },
            {WifiManager.WIFI_FEATURE_CONTROL_ROAMING,
                    IWifiStaIface.StaIfaceCapabilityMask.CONTROL_ROAMING
            },
            {WifiManager.WIFI_FEATURE_IE_WHITELIST,
                    IWifiStaIface.StaIfaceCapabilityMask.PROBE_IE_WHITELIST
            },
            {WifiManager.WIFI_FEATURE_SCAN_RAND,
                    IWifiStaIface.StaIfaceCapabilityMask.SCAN_RAND
            }
    };

    /**
     * Feature bit mask translation for STAs
     *
     * @param capabilities bitmask defined IWifiStaIface.StaIfaceCapabilityMask
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    @VisibleForTesting
    long wifiFeatureMaskFromStaCapabilities(int capabilities) {
        long features = 0;
        for (int i = 0; i < sStaFeatureCapabilityTranslation.length; i++) {
            if ((capabilities & sStaFeatureCapabilityTranslation[i][1]) != 0) {
                features |= sStaFeatureCapabilityTranslation[i][0];
            }
        }
        return features;
    }

    /**
     * Translation table used by getSupportedFeatureSetFromPackageManager
     * for translating System caps
     */
    private static final Pair[] sSystemFeatureCapabilityTranslation = new Pair[] {
            Pair.create(WifiManager.WIFI_FEATURE_INFRA, PackageManager.FEATURE_WIFI),
            Pair.create(WifiManager.WIFI_FEATURE_P2P, PackageManager.FEATURE_WIFI_DIRECT),
            Pair.create(WifiManager.WIFI_FEATURE_AWARE, PackageManager.FEATURE_WIFI_AWARE),
    };

    /**
     * If VendorHal is not supported, reading PackageManager
     * system features to return basic capabilities.
     *
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    private long getSupportedFeatureSetFromPackageManager() {
        long featureSet = 0;
        final PackageManager pm = sContext.getPackageManager();
        for (Pair pair: sSystemFeatureCapabilityTranslation) {
            if (pm.hasSystemFeature((String) pair.second)) {
                featureSet |= (long) pair.first;
            }
        }
        enter("System feature set: %").c(featureSet).flush();
        return featureSet;
    }

    /**
     * Get the supported features
     *
     * The result may differ depending on the mode (STA or AP)
     *
     * @param ifaceName Name of the interface.
     * @return bitmask defined by WifiManager.WIFI_FEATURE_*
     */
    public long getSupportedFeatureSet(@NonNull String ifaceName) {
        long featureSet = 0;
        if (!mHalDeviceManager.isStarted() || !mHalDeviceManager.isSupported()) {
            return getSupportedFeatureSetFromPackageManager();
        }
        try {
            final Mutable<Long> feat = new Mutable<>(0L);
            synchronized (sLock) {
                android.hardware.wifi.V1_3.IWifiChip iWifiChipV13 = getWifiChipForV1_3Mockable();
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
                if (iWifiChipV15 != null) {
                    iWifiChipV15.getCapabilities_1_5((status, capabilities) -> {
                        if (!ok(status)) return;
                        feat.value = wifiFeatureMaskFromChipCapabilities_1_5(capabilities);
                    });
                } else if (iWifiChipV13 != null) {
                    iWifiChipV13.getCapabilities_1_3((status, capabilities) -> {
                        if (!ok(status)) return;
                        feat.value = wifiFeatureMaskFromChipCapabilities_1_3(capabilities);
                    });
                } else if (mIWifiChip != null) {
                    mIWifiChip.getCapabilities((status, capabilities) -> {
                        if (!ok(status)) return;
                        feat.value = (long) wifiFeatureMaskFromChipCapabilities(capabilities);
                    });
                }

                IWifiStaIface iface = getStaIface(ifaceName);
                if (iface != null) {
                    iface.getCapabilities((status, capabilities) -> {
                        if (!ok(status)) return;
                        feat.value |= wifiFeatureMaskFromStaCapabilities(capabilities);
                    });
                }
            }
            featureSet = feat.value;
        } catch (RemoteException e) {
            handleRemoteException(e);
            return 0;
        }

        if (mWifiGlobals.isWpa3SaeH2eSupported()) {
            featureSet |= WifiManager.WIFI_FEATURE_SAE_H2E;
        }

        Set<Integer> supportedIfaceTypes = mHalDeviceManager.getSupportedIfaceTypes();
        if (supportedIfaceTypes.contains(IfaceType.STA)) {
            featureSet |= WifiManager.WIFI_FEATURE_INFRA;
        }
        if (supportedIfaceTypes.contains(IfaceType.AP)) {
            featureSet |= WifiManager.WIFI_FEATURE_MOBILE_HOTSPOT;
        }
        if (supportedIfaceTypes.contains(IfaceType.P2P)) {
            featureSet |= WifiManager.WIFI_FEATURE_P2P;
        }
        if (supportedIfaceTypes.contains(IfaceType.NAN)) {
            featureSet |= WifiManager.WIFI_FEATURE_AWARE;
        }

        return featureSet;
    }

    /**
     * Set Mac address on the given interface
     *
     * @param ifaceName Name of the interface
     * @param mac MAC address to change into
     * @return true for success
     */
    public boolean setStaMacAddress(@NonNull String ifaceName, @NonNull MacAddress mac) {
        byte[] macByteArray = mac.toByteArray();
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_2.IWifiStaIface sta12 =
                        getWifiStaIfaceForV1_2Mockable(ifaceName);
                if (sta12 == null) return boolResult(false);
                return ok(sta12.setMacAddress(macByteArray));
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Reset MAC address to factory MAC address on the given interface
     *
     * @param ifaceName Name of the interface
     * @return true for success
     */
    public boolean resetApMacToFactoryMacAddress(@NonNull String ifaceName) {
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiApIface ap15 =
                        getWifiApIfaceForV1_5Mockable(ifaceName);
                if (ap15 == null) {
                    MacAddress mac = getApFactoryMacAddress(ifaceName);
                    return mac != null && setApMacAddress(ifaceName, mac);
                }
                return ok(ap15.resetToFactoryMacAddress());
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Set Mac address on the given interface
     *
     * @param ifaceName Name of the interface
     * @param mac MAC address to change into
     * @return true for success
     */
    public boolean setApMacAddress(@NonNull String ifaceName, @NonNull MacAddress mac) {
        byte[] macByteArray = mac.toByteArray();
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_4.IWifiApIface ap14 =
                        getWifiApIfaceForV1_4Mockable(ifaceName);
                if (ap14 == null) return boolResult(false);
                return ok(ap14.setMacAddress(macByteArray));
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Returns true if Hal version supports setMacAddress, otherwise false.
     *
     * @param ifaceName Name of the interface
     */
    public boolean isStaSetMacAddressSupported(@NonNull String ifaceName) {
        synchronized (sLock) {
            android.hardware.wifi.V1_2.IWifiStaIface sta12 =
                    getWifiStaIfaceForV1_2Mockable(ifaceName);
            return sta12 != null;
        }
    }

    /**
     * Returns true if Hal version supports setMacAddress, otherwise false.
     *
     * @param ifaceName Name of the interface
     */
    public boolean isApSetMacAddressSupported(@NonNull String ifaceName) {
        synchronized (sLock) {
            android.hardware.wifi.V1_4.IWifiApIface ap14 =
                    getWifiApIfaceForV1_4Mockable(ifaceName);
            return ap14 != null;
        }
    }

    /**
     * Get factory MAC address of the given interface
     *
     * @param ifaceName Name of the interface
     * @return factory MAC address of the interface or null.
     */
    public MacAddress getStaFactoryMacAddress(@NonNull String ifaceName) {
        class AnswerBox {
            public MacAddress mac = null;
        }
        synchronized (sLock) {
            try {
                AnswerBox box = new AnswerBox();

                android.hardware.wifi.V1_3.IWifiStaIface sta13 =
                        getWifiStaIfaceForV1_3Mockable(ifaceName);
                if (sta13 == null) return null;
                sta13.getFactoryMacAddress((status, macBytes) -> {
                    if (!ok(status)) return;
                    box.mac = MacAddress.fromBytes(macBytes);
                });
                return box.mac;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
    }

    /**
     * Get factory MAC address of the given interface
     *
     * @param ifaceName Name of the interface
     * @return factory MAC address of the interface or null.
     */
    public MacAddress getApFactoryMacAddress(@NonNull String ifaceName) {
        class AnswerBox {
            public MacAddress mac = null;
        }
        synchronized (sLock) {
            try {
                AnswerBox box = new AnswerBox();
                android.hardware.wifi.V1_4.IWifiApIface ap14 =
                        getWifiApIfaceForV1_4Mockable(ifaceName);
                if (ap14 == null) return null;
                ap14.getFactoryMacAddress((status, macBytes) -> {
                    if (!ok(status)) return;
                    box.mac = MacAddress.fromBytes(macBytes);
                });
                return box.mac;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
    }

    /**
     * Get the APF (Android Packet Filter) capabilities of the device
     *
     * @param ifaceName Name of the interface.
     * @return APF capabilities object.
     */
    public ApfCapabilities getApfCapabilities(@NonNull String ifaceName) {
        class AnswerBox {
            public ApfCapabilities value = sNoApfCapabilities;
        }
        synchronized (sLock) {
            try {
                IWifiStaIface iface = getStaIface(ifaceName);
                if (iface == null) return sNoApfCapabilities;
                AnswerBox box = new AnswerBox();
                iface.getApfPacketFilterCapabilities((status, capabilities) -> {
                    if (!ok(status)) return;
                    box.value = new ApfCapabilities(
                        /* apfVersionSupported */   capabilities.version,
                        /* maximumApfProgramSize */ capabilities.maxLength,
                        /* apfPacketFormat */       android.system.OsConstants.ARPHRD_ETHER);
                });
                return box.value;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return sNoApfCapabilities;
            }
        }
    }

    private static final ApfCapabilities sNoApfCapabilities = new ApfCapabilities(0, 0, 0);

    /**
     * Installs an APF program on this iface, replacing any existing program.
     *
     * @param ifaceName Name of the interface.
     * @param filter is the android packet filter program
     * @return true for success
     */
    public boolean installPacketFilter(@NonNull String ifaceName, byte[] filter) {
        int cmdId = 0; // We only aspire to support one program at a time
        if (filter == null) return boolResult(false);
        // Copy the program before taking the lock.
        ArrayList<Byte> program = NativeUtil.byteArrayToArrayList(filter);
        enter("filter length %").c(filter.length).flush();
        synchronized (sLock) {
            try {
                IWifiStaIface iface = getStaIface(ifaceName);
                if (iface == null) return boolResult(false);
                WifiStatus status = iface.installApfPacketFilter(cmdId, program);
                if (!ok(status)) return false;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Reads the APF program and data buffer on this iface.
     *
     * @param ifaceName Name of the interface
     * @return the buffer returned by the driver, or null in case of an error
     */
    public byte[] readPacketFilter(@NonNull String ifaceName) {
        class AnswerBox {
            public byte[] data = null;
        }
        AnswerBox answer = new AnswerBox();
        enter("").flush();
        // TODO: Must also take the wakelock here to prevent going to sleep with APF disabled.
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_2.IWifiStaIface ifaceV12 =
                        getWifiStaIfaceForV1_2Mockable(ifaceName);
                if (ifaceV12 == null) return byteArrayResult(null);
                ifaceV12.readApfPacketFilterData((status, dataByteArray) -> {
                    if (!ok(status)) return;
                    answer.data = NativeUtil.byteArrayFromArrayList(dataByteArray);
                });
                return byteArrayResult(answer.data);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return byteArrayResult(null);
            }
        }
    }

    /**
     * Set country code for this Wifi chip
     *
     * @param countryCode - two-letter country code (as ISO 3166)
     * @return true for success
     */
    public boolean setChipCountryCode(String countryCode) {
        if (countryCode == null) return boolResult(false);
        if (countryCode.length() != 2) return boolResult(false);
        byte[] code;
        try {
            code = NativeUtil.stringToByteArray(countryCode);
        } catch (IllegalArgumentException e) {
            return boolResult(false);
        }
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
                if (iWifiChipV15 == null) return boolResult(false);
                WifiStatus status = iWifiChipV15.setCountryCode(code);
                if (!ok(status)) return false;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Get the names of the bridged AP instances.
     *
     * @param ifaceName Name of the bridged interface.
     * @return A list which contains the names of the bridged AP instances.
     */
    @Nullable
    public List<String> getBridgedApInstances(@NonNull String ifaceName) {
        synchronized (sLock) {
            try {
                Mutable<List<String>> instancesResp  = new Mutable<>();
                android.hardware.wifi.V1_5.IWifiApIface ap15 =
                        getWifiApIfaceForV1_5Mockable(ifaceName);
                if (ap15 == null) return null;
                ap15.getBridgedInstances((status, instances) -> {
                    if (!ok(status)) return;
                    instancesResp.value = new ArrayList<>(instances);
                });
                return instancesResp.value;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
    }

    /**
     * Set country code for this AP iface.
     *
     * @param ifaceName Name of the interface.
     * @param countryCode - two-letter country code (as ISO 3166)
     * @return true for success
     */
    public boolean setApCountryCode(@NonNull String ifaceName, String countryCode) {
        if (countryCode == null) return boolResult(false);
        if (countryCode.length() != 2) return boolResult(false);
        byte[] code;
        try {
            code = NativeUtil.stringToByteArray(countryCode);
        } catch (IllegalArgumentException e) {
            return boolResult(false);
        }
        synchronized (sLock) {
            try {
                IWifiApIface iface = getApIface(ifaceName);
                if (iface == null) return boolResult(false);
                WifiStatus status = iface.setCountryCode(code);
                if (!ok(status)) return false;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    private WifiNative.WifiLoggerEventHandler mLogEventHandler = null;

    /**
     * Registers the logger callback and enables alerts.
     * Ring buffer data collection is only triggered when |startLoggingRingBuffer| is invoked.
     */
    public boolean setLoggingEventHandler(WifiNative.WifiLoggerEventHandler handler) {
        if (handler == null) return boolResult(false);
        synchronized (sLock) {
            if (mIWifiChip == null) return boolResult(false);
            if (mLogEventHandler != null) return boolResult(false);
            try {
                WifiStatus status = mIWifiChip.enableDebugErrorAlerts(true);
                if (!ok(status)) return false;
                mLogEventHandler = handler;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Stops all logging and resets the logger callback.
     * This stops both the alerts and ring buffer data collection.
     * Existing log handler is cleared.
     */
    public boolean resetLogHandler() {
        synchronized (sLock) {
            mLogEventHandler = null;
            if (mIWifiChip == null) return boolResult(false);
            try {
                WifiStatus status = mIWifiChip.enableDebugErrorAlerts(false);
                if (!ok(status)) return false;
                status = mIWifiChip.stopLoggingToDebugRingBuffer();
                if (!ok(status)) return false;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Control debug data collection
     *
     * @param verboseLevel       0 to 3, inclusive. 0 stops logging.
     * @param flags              Ignored.
     * @param maxIntervalInSec   Maximum interval between reports; ignore if 0.
     * @param minDataSizeInBytes Minimum data size in buffer for report; ignore if 0.
     * @param ringName           Name of the ring for which data collection is to start.
     * @return true for success
     */
    public boolean startLoggingRingBuffer(int verboseLevel, int flags, int maxIntervalInSec,
                                          int minDataSizeInBytes, String ringName) {
        enter("verboseLevel=%, flags=%, maxIntervalInSec=%, minDataSizeInBytes=%, ringName=%")
                .c(verboseLevel).c(flags).c(maxIntervalInSec).c(minDataSizeInBytes).c(ringName)
                .flush();
        synchronized (sLock) {
            if (mIWifiChip == null) return boolResult(false);
            try {
                // note - flags are not used
                WifiStatus status = mIWifiChip.startLoggingToDebugRingBuffer(
                        ringName,
                        verboseLevel,
                        maxIntervalInSec,
                        minDataSizeInBytes
                );
                return ok(status);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Pointlessly fail
     *
     * @return -1
     */
    public int getSupportedLoggerFeatureSet() {
        return -1;
    }

    private String mDriverDescription; // Cached value filled by requestChipDebugInfo()

    /**
     * Vendor-provided wifi driver version string
     */
    public String getDriverVersion() {
        synchronized (sLock) {
            if (mDriverDescription == null) requestChipDebugInfo();
            return mDriverDescription;
        }
    }

    private String mFirmwareDescription; // Cached value filled by requestChipDebugInfo()

    /**
     * Vendor-provided wifi firmware version string
     */
    public String getFirmwareVersion() {
        synchronized (sLock) {
            if (mFirmwareDescription == null) requestChipDebugInfo();
            return mFirmwareDescription;
        }
    }

    /**
     * Refreshes our idea of the driver and firmware versions
     */
    private void requestChipDebugInfo() {
        mDriverDescription = null;
        mFirmwareDescription = null;
        try {
            if (mIWifiChip == null) return;
            mIWifiChip.requestChipDebugInfo((status, chipDebugInfo) -> {
                if (!ok(status)) return;
                mDriverDescription = chipDebugInfo.driverDescription;
                mFirmwareDescription = chipDebugInfo.firmwareDescription;
            });
        } catch (RemoteException e) {
            handleRemoteException(e);
            return;
        }
        mLog.info("Driver: % Firmware: %")
                .c(mDriverDescription)
                .c(mFirmwareDescription)
                .flush();
    }

    /**
     * Creates RingBufferStatus from the Hal version
     */
    private static WifiNative.RingBufferStatus ringBufferStatus(WifiDebugRingBufferStatus h) {
        WifiNative.RingBufferStatus ans = new WifiNative.RingBufferStatus();
        ans.name = h.ringName;
        ans.flag = frameworkRingBufferFlagsFromHal(h.flags);
        ans.ringBufferId = h.ringId;
        ans.ringBufferByteSize = h.sizeInBytes;
        ans.verboseLevel = h.verboseLevel;
        // Remaining fields are unavailable
        //  writtenBytes;
        //  readBytes;
        //  writtenRecords;
        return ans;
    }

    /**
     * Translates a hal wifiDebugRingBufferFlag to the WifiNative version
     */
    private static int frameworkRingBufferFlagsFromHal(int wifiDebugRingBufferFlag) {
        BitMask checkoff = new BitMask(wifiDebugRingBufferFlag);
        int flags = 0;
        if (checkoff.testAndClear(WifiDebugRingBufferFlags.HAS_BINARY_ENTRIES)) {
            flags |= WifiNative.RingBufferStatus.HAS_BINARY_ENTRIES;
        }
        if (checkoff.testAndClear(WifiDebugRingBufferFlags.HAS_ASCII_ENTRIES)) {
            flags |= WifiNative.RingBufferStatus.HAS_ASCII_ENTRIES;
        }
        if (checkoff.testAndClear(WifiDebugRingBufferFlags.HAS_PER_PACKET_ENTRIES)) {
            flags |= WifiNative.RingBufferStatus.HAS_PER_PACKET_ENTRIES;
        }
        if (checkoff.value != 0) {
            throw new IllegalArgumentException("Unknown WifiDebugRingBufferFlag " + checkoff.value);
        }
        return flags;
    }

    /**
     * Creates array of RingBufferStatus from the Hal version
     */
    private static WifiNative.RingBufferStatus[] makeRingBufferStatusArray(
            ArrayList<WifiDebugRingBufferStatus> ringBuffers) {
        WifiNative.RingBufferStatus[] ans = new WifiNative.RingBufferStatus[ringBuffers.size()];
        int i = 0;
        for (WifiDebugRingBufferStatus b : ringBuffers) {
            ans[i++] = ringBufferStatus(b);
        }
        return ans;
    }

    /**
     * API to get the status of all ring buffers supported by driver
     */
    public WifiNative.RingBufferStatus[] getRingBufferStatus() {
        class AnswerBox {
            public WifiNative.RingBufferStatus[] value = null;
        }
        AnswerBox ans = new AnswerBox();
        synchronized (sLock) {
            if (mIWifiChip == null) return null;
            try {
                mIWifiChip.getDebugRingBuffersStatus((status, ringBuffers) -> {
                    if (!ok(status)) return;
                    ans.value = makeRingBufferStatusArray(ringBuffers);
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        return ans.value;
    }

    /**
     * Indicates to driver that all the data has to be uploaded urgently
     */
    public boolean getRingBufferData(String ringName) {
        enter("ringName %").c(ringName).flush();
        synchronized (sLock) {
            if (mIWifiChip == null) return boolResult(false);
            try {
                WifiStatus status = mIWifiChip.forceDumpToDebugRingBuffer(ringName);
                return ok(status);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * request hal to flush ring buffers to files
     */
    public boolean flushRingBufferData() {
        synchronized (sLock) {
            if (mIWifiChip == null) return boolResult(false);
            android.hardware.wifi.V1_3.IWifiChip iWifiChipV13 = getWifiChipForV1_3Mockable();
            if (iWifiChipV13 != null) {
                try {
                    WifiStatus status = iWifiChipV13.flushRingBufferToFile();
                    return ok(status);
                } catch (RemoteException e) {
                    handleRemoteException(e);
                    return false;
                }
            }
            return false;
        }
    }

    /**
     * Request vendor debug info from the firmware
     */
    public byte[] getFwMemoryDump() {
        class AnswerBox {
            public byte[] value;
        }
        AnswerBox ans = new AnswerBox();
        synchronized (sLock) {
            if (mIWifiChip == null) return (null);
            try {
                mIWifiChip.requestFirmwareDebugDump((status, blob) -> {
                    if (!ok(status)) return;
                    ans.value = NativeUtil.byteArrayFromArrayList(blob);
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        return ans.value;
    }

    /**
     * Request vendor debug info from the driver
     */
    public byte[] getDriverStateDump() {
        class AnswerBox {
            public byte[] value;
        }
        AnswerBox ans = new AnswerBox();
        synchronized (sLock) {
            if (mIWifiChip == null) return (null);
            try {
                mIWifiChip.requestDriverDebugDump((status, blob) -> {
                    if (!ok(status)) return;
                    ans.value = NativeUtil.byteArrayFromArrayList(blob);
                });
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
        return ans.value;
    }

    /**
     * Start packet fate monitoring
     * <p>
     * Once started, monitoring remains active until HAL is unloaded.
     *
     * @param ifaceName Name of the interface.
     * @return true for success
     */
    public boolean startPktFateMonitoring(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            try {
                WifiStatus status = iface.startDebugPacketFateMonitoring();
                return ok(status);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    private byte halToFrameworkPktFateFrameType(int type) {
        switch (type) {
            case WifiDebugPacketFateFrameType.UNKNOWN:
                return WifiLoggerHal.FRAME_TYPE_UNKNOWN;
            case WifiDebugPacketFateFrameType.ETHERNET_II:
                return WifiLoggerHal.FRAME_TYPE_ETHERNET_II;
            case WifiDebugPacketFateFrameType.MGMT_80211:
                return WifiLoggerHal.FRAME_TYPE_80211_MGMT;
            default:
                throw new IllegalArgumentException("bad " + type);
        }
    }

    private byte halToFrameworkRxPktFate(int type) {
        switch (type) {
            case WifiDebugRxPacketFate.SUCCESS:
                return WifiLoggerHal.RX_PKT_FATE_SUCCESS;
            case WifiDebugRxPacketFate.FW_QUEUED:
                return WifiLoggerHal.RX_PKT_FATE_FW_QUEUED;
            case WifiDebugRxPacketFate.FW_DROP_FILTER:
                return WifiLoggerHal.RX_PKT_FATE_FW_DROP_FILTER;
            case WifiDebugRxPacketFate.FW_DROP_INVALID:
                return WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID;
            case WifiDebugRxPacketFate.FW_DROP_NOBUFS:
                return WifiLoggerHal.RX_PKT_FATE_FW_DROP_NOBUFS;
            case WifiDebugRxPacketFate.FW_DROP_OTHER:
                return WifiLoggerHal.RX_PKT_FATE_FW_DROP_OTHER;
            case WifiDebugRxPacketFate.DRV_QUEUED:
                return WifiLoggerHal.RX_PKT_FATE_DRV_QUEUED;
            case WifiDebugRxPacketFate.DRV_DROP_FILTER:
                return WifiLoggerHal.RX_PKT_FATE_DRV_DROP_FILTER;
            case WifiDebugRxPacketFate.DRV_DROP_INVALID:
                return WifiLoggerHal.RX_PKT_FATE_DRV_DROP_INVALID;
            case WifiDebugRxPacketFate.DRV_DROP_NOBUFS:
                return WifiLoggerHal.RX_PKT_FATE_DRV_DROP_NOBUFS;
            case WifiDebugRxPacketFate.DRV_DROP_OTHER:
                return WifiLoggerHal.RX_PKT_FATE_DRV_DROP_OTHER;
            default:
                throw new IllegalArgumentException("bad " + type);
        }
    }

    private byte halToFrameworkTxPktFate(int type) {
        switch (type) {
            case WifiDebugTxPacketFate.ACKED:
                return WifiLoggerHal.TX_PKT_FATE_ACKED;
            case WifiDebugTxPacketFate.SENT:
                return WifiLoggerHal.TX_PKT_FATE_SENT;
            case WifiDebugTxPacketFate.FW_QUEUED:
                return WifiLoggerHal.TX_PKT_FATE_FW_QUEUED;
            case WifiDebugTxPacketFate.FW_DROP_INVALID:
                return WifiLoggerHal.TX_PKT_FATE_FW_DROP_INVALID;
            case WifiDebugTxPacketFate.FW_DROP_NOBUFS:
                return WifiLoggerHal.TX_PKT_FATE_FW_DROP_NOBUFS;
            case WifiDebugTxPacketFate.FW_DROP_OTHER:
                return WifiLoggerHal.TX_PKT_FATE_FW_DROP_OTHER;
            case WifiDebugTxPacketFate.DRV_QUEUED:
                return WifiLoggerHal.TX_PKT_FATE_DRV_QUEUED;
            case WifiDebugTxPacketFate.DRV_DROP_INVALID:
                return WifiLoggerHal.TX_PKT_FATE_DRV_DROP_INVALID;
            case WifiDebugTxPacketFate.DRV_DROP_NOBUFS:
                return WifiLoggerHal.TX_PKT_FATE_DRV_DROP_NOBUFS;
            case WifiDebugTxPacketFate.DRV_DROP_OTHER:
                return WifiLoggerHal.TX_PKT_FATE_DRV_DROP_OTHER;
            default:
                throw new IllegalArgumentException("bad " + type);
        }
    }

    /**
     * Retrieve fates of outbound packets
     * <p>
     * Reports the outbound frames for the most recent association (space allowing).
     *
     * @param ifaceName Name of the interface.
     * @return list of TxFateReports up to size {@link WifiLoggerHal#MAX_FATE_LOG_LEN}, or empty
     * list on failure.
     */
    public List<TxFateReport> getTxPktFates(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return objResult(new ArrayList<>());
            try {
                List<TxFateReport> reportBufs = new ArrayList<>();
                iface.getDebugTxPacketFates((status, fates) -> {
                    if (!ok(status)) return;
                    for (WifiDebugTxPacketFateReport fate : fates) {
                        if (reportBufs.size() >= WifiLoggerHal.MAX_FATE_LOG_LEN) break;
                        byte code = halToFrameworkTxPktFate(fate.fate);
                        long us = fate.frameInfo.driverTimestampUsec;
                        byte type = halToFrameworkPktFateFrameType(fate.frameInfo.frameType);
                        byte[] frame = NativeUtil.byteArrayFromArrayList(
                                fate.frameInfo.frameContent);
                        reportBufs.add(new TxFateReport(code, us, type, frame));
                    }
                });
                return reportBufs;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return new ArrayList<>();
            }
        }
    }

    /**
     * Retrieve fates of inbound packets
     * <p>
     * Reports the inbound frames for the most recent association (space allowing).
     *
     * @param ifaceName Name of the interface.
     * @return list of RxFateReports up to size {@link WifiLoggerHal#MAX_FATE_LOG_LEN}, or empty
     * list on failure.
     */
    public List<RxFateReport> getRxPktFates(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return objResult(new ArrayList<>());
            try {
                List<RxFateReport> reportBufs = new ArrayList<>();
                iface.getDebugRxPacketFates((status, fates) -> {
                    if (!ok(status)) return;
                    for (WifiDebugRxPacketFateReport fate : fates) {
                        if (reportBufs.size() >= WifiLoggerHal.MAX_FATE_LOG_LEN) break;
                        byte code = halToFrameworkRxPktFate(fate.fate);
                        long us = fate.frameInfo.driverTimestampUsec;
                        byte type = halToFrameworkPktFateFrameType(fate.frameInfo.frameType);
                        byte[] frame = NativeUtil.byteArrayFromArrayList(
                                fate.frameInfo.frameContent);
                        reportBufs.add(new RxFateReport(code, us, type, frame));
                    }
                });
                return reportBufs;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return new ArrayList<>();
            }
        }
    }

    /**
     * Start sending the specified keep alive packets periodically.
     *
     * @param ifaceName Name of the interface.
     * @param slot
     * @param srcMac
     * @param dstMac
     * @param keepAlivePacket
     * @param protocol
     * @param periodInMs
     * @return 0 for success, -1 for error
     */
    public int startSendingOffloadedPacket(
            @NonNull String ifaceName, int slot, byte[] srcMac, byte[] dstMac,
            byte[] packet, int protocol, int periodInMs) {
        enter("slot=% periodInMs=%").c(slot).c(periodInMs).flush();

        ArrayList<Byte> data = NativeUtil.byteArrayToArrayList(packet);

        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return -1;
            try {
                WifiStatus status = iface.startSendingKeepAlivePackets(
                        slot,
                        data,
                        (short) protocol,
                        srcMac,
                        dstMac,
                        periodInMs);
                if (!ok(status)) return -1;
                return 0;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return -1;
            }
        }
    }

    /**
     * Stop sending the specified keep alive packets.
     *
     * @param ifaceName Name of the interface.
     * @param slot id - same as startSendingOffloadedPacket call.
     * @return 0 for success, -1 for error
     */
    public int stopSendingOffloadedPacket(@NonNull String ifaceName, int slot) {
        enter("slot=%").c(slot).flush();

        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return -1;
            try {
                WifiStatus status = iface.stopSendingKeepAlivePackets(slot);
                if (!ok(status)) return -1;
                return 0;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return -1;
            }
        }
    }

    /**
     * A fixed cmdId for our RssiMonitoring (we only do one at a time)
     */
    @VisibleForTesting
    static final int sRssiMonCmdId = 7551;

    /**
     * Our client's handler
     */
    private WifiNative.WifiRssiEventHandler mWifiRssiEventHandler;

    /**
     * Start RSSI monitoring on the currently connected access point.
     *
     * @param ifaceName        Name of the interface.
     * @param maxRssi          Maximum RSSI threshold.
     * @param minRssi          Minimum RSSI threshold.
     * @param rssiEventHandler Called when RSSI goes above maxRssi or below minRssi
     * @return 0 for success, -1 for failure
     */
    public int startRssiMonitoring(@NonNull String ifaceName, byte maxRssi, byte minRssi,
                                   WifiNative.WifiRssiEventHandler rssiEventHandler) {
        enter("maxRssi=% minRssi=%").c(maxRssi).c(minRssi).flush();
        if (maxRssi <= minRssi) return -1;
        if (rssiEventHandler == null) return -1;
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return -1;
            try {
                iface.stopRssiMonitoring(sRssiMonCmdId);
                WifiStatus status;
                status = iface.startRssiMonitoring(sRssiMonCmdId, maxRssi, minRssi);
                if (!ok(status)) return -1;
                mWifiRssiEventHandler = rssiEventHandler;
                return 0;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return -1;
            }
        }
    }

    /**
     * Stop RSSI monitoring
     *
     * @param ifaceName Name of the interface.
     * @return 0 for success, -1 for failure
     */
    public int stopRssiMonitoring(@NonNull String ifaceName) {
        synchronized (sLock) {
            mWifiRssiEventHandler = null;
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return -1;
            try {
                WifiStatus status = iface.stopRssiMonitoring(sRssiMonCmdId);
                if (!ok(status)) return -1;
                return 0;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return -1;
            }
        }
    }

    //TODO - belongs in NativeUtil
    private static int[] intsFromArrayList(ArrayList<Integer> a) {
        if (a == null) return null;
        int[] b = new int[a.size()];
        int i = 0;
        for (Integer e : a) b[i++] = e;
        return b;
    }

    /**
     * Translates from Hal version of wake reason stats to the framework version of same
     *
     * @param h - Hal version of wake reason stats
     * @return framework version of same
     */
    private static WlanWakeReasonAndCounts halToFrameworkWakeReasons(
            WifiDebugHostWakeReasonStats h) {
        if (h == null) return null;
        WlanWakeReasonAndCounts ans = new WlanWakeReasonAndCounts();
        ans.totalCmdEventWake = h.totalCmdEventWakeCnt;
        ans.totalDriverFwLocalWake = h.totalDriverFwLocalWakeCnt;
        ans.totalRxDataWake = h.totalRxPacketWakeCnt;
        ans.rxUnicast = h.rxPktWakeDetails.rxUnicastCnt;
        ans.rxMulticast = h.rxPktWakeDetails.rxMulticastCnt;
        ans.rxBroadcast = h.rxPktWakeDetails.rxBroadcastCnt;
        ans.icmp = h.rxIcmpPkWakeDetails.icmpPkt;
        ans.icmp6 = h.rxIcmpPkWakeDetails.icmp6Pkt;
        ans.icmp6Ra = h.rxIcmpPkWakeDetails.icmp6Ra;
        ans.icmp6Na = h.rxIcmpPkWakeDetails.icmp6Na;
        ans.icmp6Ns = h.rxIcmpPkWakeDetails.icmp6Ns;
        ans.ipv4RxMulticast = h.rxMulticastPkWakeDetails.ipv4RxMulticastAddrCnt;
        ans.ipv6Multicast = h.rxMulticastPkWakeDetails.ipv6RxMulticastAddrCnt;
        ans.otherRxMulticast = h.rxMulticastPkWakeDetails.otherRxMulticastAddrCnt;
        ans.cmdEventWakeCntArray = intsFromArrayList(h.cmdEventWakeCntPerType);
        ans.driverFWLocalWakeCntArray = intsFromArrayList(h.driverFwLocalWakeCntPerType);
        return ans;
    }

    /**
     * Fetch the host wakeup reasons stats from wlan driver.
     *
     * @return the |WlanWakeReasonAndCounts| from the wlan driver, or null on failure.
     */
    public WlanWakeReasonAndCounts getWlanWakeReasonCount() {
        class AnswerBox {
            public WifiDebugHostWakeReasonStats value = null;
        }
        AnswerBox ans = new AnswerBox();
        synchronized (sLock) {
            if (mIWifiChip == null) return null;
            try {
                mIWifiChip.getDebugHostWakeReasonStats((status, stats) -> {
                    if (ok(status)) {
                        ans.value = stats;
                    }
                });
                return halToFrameworkWakeReasons(ans.value);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
    }

    /**
     * Enable/Disable Neighbour discovery offload functionality in the firmware.
     *
     * @param ifaceName Name of the interface.
     * @param enabled true to enable, false to disable.
     * @return true for success, false for failure
     */
    public boolean configureNeighborDiscoveryOffload(@NonNull String ifaceName, boolean enabled) {
        enter("enabled=%").c(enabled).flush();
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            try {
                WifiStatus status = iface.enableNdOffload(enabled);
                if (!ok(status)) return false;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
        return true;
    }

    // Firmware roaming control.

    /**
     * Query the firmware roaming capabilities.
     *
     * @param ifaceName Name of the interface.
     * @return capabilities object on success, null otherwise.
     */
    @Nullable
    public WifiNative.RoamingCapabilities getRoamingCapabilities(@NonNull String ifaceName) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return nullResult();
            try {
                Mutable<Boolean> ok = new Mutable<>(false);
                WifiNative.RoamingCapabilities out = new WifiNative.RoamingCapabilities();
                iface.getRoamingCapabilities((status, cap) -> {
                    if (!ok(status)) return;
                    out.maxBlocklistSize = cap.maxBlacklistSize;
                    out.maxAllowlistSize = cap.maxWhitelistSize;
                    ok.value = true;
                });
                if (ok.value) {
                    return out;
                } else {
                    return null;
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            }
        }
    }

    /**
     * Enable/disable firmware roaming.
     *
     * @param ifaceName Name of the interface.
     * @param state the intended roaming state
     * @return SET_FIRMWARE_ROAMING_SUCCESS, SET_FIRMWARE_ROAMING_FAILURE,
     *         or SET_FIRMWARE_ROAMING_BUSY
     */
    public @WifiNative.RoamingEnableStatus int enableFirmwareRoaming(@NonNull String ifaceName,
            @WifiNative.RoamingEnableState int state) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return WifiNative.SET_FIRMWARE_ROAMING_FAILURE;
            try {
                byte val;
                switch (state) {
                    case WifiNative.DISABLE_FIRMWARE_ROAMING:
                        val = StaRoamingState.DISABLED;
                        break;
                    case WifiNative.ENABLE_FIRMWARE_ROAMING:
                        val = StaRoamingState.ENABLED;
                        break;
                    default:
                        mLog.err("enableFirmwareRoaming invalid argument %").c(state).flush();
                        return WifiNative.SET_FIRMWARE_ROAMING_FAILURE;
                }
                WifiStatus status = iface.setRoamingState(val);
                if (ok(status)) {
                    return WifiNative.SET_FIRMWARE_ROAMING_SUCCESS;
                } else if (status.code == WifiStatusCode.ERROR_BUSY) {
                    return WifiNative.SET_FIRMWARE_ROAMING_BUSY;
                } else {
                    return WifiNative.SET_FIRMWARE_ROAMING_FAILURE;
                }
            } catch (RemoteException e) {
                handleRemoteException(e);
                return WifiNative.SET_FIRMWARE_ROAMING_FAILURE;
            }
        }
    }

    /**
     * Set firmware roaming configurations.
     *
     * @param ifaceName Name of the interface.
     * @param config new roaming configuration object
     * @return true for success; false for failure
     */
    public boolean configureRoaming(@NonNull String ifaceName, WifiNative.RoamingConfig config) {
        synchronized (sLock) {
            IWifiStaIface iface = getStaIface(ifaceName);
            if (iface == null) return boolResult(false);
            try {
                StaRoamingConfig roamingConfig = new StaRoamingConfig();

                // parse the blacklist BSSIDs if any
                if (config.blocklistBssids != null) {
                    for (String bssid : config.blocklistBssids) {
                        byte[] mac = NativeUtil.macAddressToByteArray(bssid);
                        roamingConfig.bssidBlacklist.add(mac);
                    }
                }

                // parse the whitelist SSIDs if any
                if (config.allowlistSsids != null) {
                    for (String ssidStr : config.allowlistSsids) {
                        byte[] ssid = NativeUtil.byteArrayFromArrayList(
                                NativeUtil.decodeSsid(ssidStr));
                        // HIDL code is throwing InvalidArgumentException when ssidWhitelist has
                        // SSIDs with less than 32 byte length this is due to HAL definition of
                        // SSID declared it as 32-byte fixed length array. Thus pad additional
                        // bytes with 0's to pass SSIDs as byte arrays of 32 length
                        byte[] ssid_32 = new byte[32];
                        for (int i = 0; i < ssid.length; i++) {
                            ssid_32[i] = ssid[i];
                        }
                        roamingConfig.ssidWhitelist.add(ssid_32);
                    }
                }

                WifiStatus status = iface.configureRoaming(roamingConfig);
                if (!ok(status)) return false;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            } catch (IllegalArgumentException e) {
                mLog.err("Illegal argument for roaming configuration").c(e.toString()).flush();
                return false;
            }
            return true;
        }
    }

    /**
     * Method to mock out the V1_1 IWifiChip retrieval in unit tests.
     *
     * @return 1.1 IWifiChip object if the device is running the 1.1 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_1.IWifiChip getWifiChipForV1_1Mockable() {
        if (mIWifiChip == null) return null;
        return android.hardware.wifi.V1_1.IWifiChip.castFrom(mIWifiChip);
    }

    /**
     * Method to mock out the V1_2 IWifiChip retrieval in unit tests.
     *
     * @return 1.2 IWifiChip object if the device is running the 1.2 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_2.IWifiChip getWifiChipForV1_2Mockable() {
        if (mIWifiChip == null) return null;
        return android.hardware.wifi.V1_2.IWifiChip.castFrom(mIWifiChip);
    }

    /**
     * Method to mock out the V1_3 IWifiChip retrieval in unit tests.
     *
     * @return 1.3 IWifiChip object if the device is running the 1.3 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_3.IWifiChip getWifiChipForV1_3Mockable() {
        if (mIWifiChip == null) return null;
        return android.hardware.wifi.V1_3.IWifiChip.castFrom(mIWifiChip);
    }

    /**
     * Method to mock out the V1_4 IWifiChip retrieval in unit tests.
     *
     * @return 1.4 IWifiChip object if the device is running the 1.4 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_4.IWifiChip getWifiChipForV1_4Mockable() {
        if (mIWifiChip == null) return null;
        return android.hardware.wifi.V1_4.IWifiChip.castFrom(mIWifiChip);
    }

    /**
     * Method to mock out the V1_5 IWifiChip retrieval in unit tests.
     *
     * @return 1.5 IWifiChip object if the device is running the 1.5 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_5.IWifiChip getWifiChipForV1_5Mockable() {
        if (mIWifiChip == null) return null;
        return android.hardware.wifi.V1_5.IWifiChip.castFrom(mIWifiChip);
    }

    /**
     * Method to mock out the V1_2 IWifiStaIface retrieval in unit tests.
     *
     * @param ifaceName Name of the interface
     * @return 1.2 IWifiStaIface object if the device is running the 1.2 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_2.IWifiStaIface getWifiStaIfaceForV1_2Mockable(
            @NonNull String ifaceName) {
        IWifiStaIface iface = getStaIface(ifaceName);
        if (iface == null) return null;
        return android.hardware.wifi.V1_2.IWifiStaIface.castFrom(iface);
    }

    /**
     * Method to mock out the V1_3 IWifiStaIface retrieval in unit tests.
     *
     * @param ifaceName Name of the interface
     * @return 1.3 IWifiStaIface object if the device is running the 1.3 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_3.IWifiStaIface getWifiStaIfaceForV1_3Mockable(
            @NonNull String ifaceName) {
        IWifiStaIface iface = getStaIface(ifaceName);
        if (iface == null) return null;
        return android.hardware.wifi.V1_3.IWifiStaIface.castFrom(iface);
    }

    /**
     * Method to mock out the V1_5 IWifiStaIface retrieval in unit tests.
     *
     * @param ifaceName Name of the interface
     * @return 1.5 IWifiStaIface object if the device is running the 1.5 wifi hal service, null
     * otherwise.
     */
    protected android.hardware.wifi.V1_5.IWifiStaIface getWifiStaIfaceForV1_5Mockable(
            @NonNull String ifaceName) {
        IWifiStaIface iface = getStaIface(ifaceName);
        if (iface == null) return null;
        return android.hardware.wifi.V1_5.IWifiStaIface.castFrom(iface);
    }

    protected android.hardware.wifi.V1_4.IWifiApIface getWifiApIfaceForV1_4Mockable(
            String ifaceName) {
        IWifiApIface iface = getApIface(ifaceName);
        if (iface == null) return null;
        return android.hardware.wifi.V1_4.IWifiApIface.castFrom(iface);
    }

    protected android.hardware.wifi.V1_5.IWifiApIface getWifiApIfaceForV1_5Mockable(
            String ifaceName) {
        IWifiApIface iface = getApIface(ifaceName);
        if (iface == null) return null;
        return android.hardware.wifi.V1_5.IWifiApIface.castFrom(iface);
    }

    /**
     * sarPowerBackoffRequired_1_1()
     * This method checks if we need to backoff wifi Tx power due to SAR requirements.
     * It handles the case when the device is running the V1_1 version of WifiChip HAL
     * In that HAL version, it is required to perform wifi Tx power backoff only if
     * a voice call is ongoing.
     */
    private boolean sarPowerBackoffRequired_1_1(SarInfo sarInfo) {
        /* As long as no voice call is active (in case voice call is supported),
         * no backoff is needed */
        if (sarInfo.sarVoiceCallSupported) {
            return (sarInfo.isVoiceCall || sarInfo.isEarPieceActive);
        } else {
            return false;
        }
    }

    /**
     * frameworkToHalTxPowerScenario_1_1()
     * This method maps the information inside the SarInfo instance into a SAR scenario
     * when device is running the V1_1 version of WifiChip HAL.
     * In this HAL version, only one scenario is defined which is for VOICE_CALL (if voice call is
     * supported).
     * Otherwise, an exception is thrown.
     */
    private int frameworkToHalTxPowerScenario_1_1(SarInfo sarInfo) {
        if (sarInfo.sarVoiceCallSupported && (sarInfo.isVoiceCall || sarInfo.isEarPieceActive)) {
            return android.hardware.wifi.V1_1.IWifiChip.TxPowerScenario.VOICE_CALL;
        } else {
            throw new IllegalArgumentException("bad scenario: voice call not active/supported");
        }
    }

    /**
     * sarPowerBackoffRequired_1_2()
     * This method checks if we need to backoff wifi Tx power due to SAR requirements.
     * It handles the case when the device is running the V1_2 version of WifiChip HAL
     */
    private boolean sarPowerBackoffRequired_1_2(SarInfo sarInfo) {
        if (sarInfo.sarSapSupported && sarInfo.isWifiSapEnabled) {
            return true;
        }
        if (sarInfo.sarVoiceCallSupported && (sarInfo.isVoiceCall || sarInfo.isEarPieceActive)) {
            return true;
        }
        return false;
    }

    /**
     * frameworkToHalTxPowerScenario_1_2()
     * This method maps the information inside the SarInfo instance into a SAR scenario
     * when device is running the V1_2 version of WifiChip HAL.
     * If SAR SoftAP input is supported,
     * we make these assumptions:
     *   - All voice calls are treated as if device is near the head.
     *   - SoftAP scenario is treated as if device is near the body.
     * In case SoftAP is not supported, then we should revert to the V1_1 HAL
     * behavior, and the only valid scenario would be when a voice call is ongoing.
     */
    private int frameworkToHalTxPowerScenario_1_2(SarInfo sarInfo) {
        if (sarInfo.sarSapSupported && sarInfo.sarVoiceCallSupported) {
            if (sarInfo.isVoiceCall || sarInfo.isEarPieceActive) {
                return android.hardware.wifi.V1_2.IWifiChip
                        .TxPowerScenario.ON_HEAD_CELL_ON;
            } else if (sarInfo.isWifiSapEnabled) {
                return android.hardware.wifi.V1_2.IWifiChip
                        .TxPowerScenario.ON_BODY_CELL_ON;
            } else {
                throw new IllegalArgumentException("bad scenario: no voice call/softAP active");
            }
        } else if (sarInfo.sarVoiceCallSupported) {
            /* SAR SoftAP input not supported, act like V1_1 */
            if (sarInfo.isVoiceCall || sarInfo.isEarPieceActive) {
                return android.hardware.wifi.V1_1.IWifiChip.TxPowerScenario.VOICE_CALL;
            } else {
                throw new IllegalArgumentException("bad scenario: voice call not active");
            }
        } else {
            throw new IllegalArgumentException("Invalid case: voice call not supported");
        }
    }

    /**
     * Select one of the pre-configured TX power level scenarios or reset it back to normal.
     * Primarily used for meeting SAR requirements during voice calls.
     *
     * Note: If it was found out that the scenario to be reported is the same as last reported one,
     *       then exit with success.
     *       This is to handle the case when some HAL versions deal with different inputs equally,
     *       in that case, we should not call the hal unless there is a change in scenario.
     * Note: It is assumed that this method is only called if SAR is enabled. The logic of whether
     *       to call it or not resides in SarManager class.
     *
     * @param sarInfo The collection of inputs to select the SAR scenario.
     * @return true for success; false for failure or if the HAL version does not support this API.
     */
    public boolean selectTxPowerScenario(SarInfo sarInfo) {
        synchronized (sLock) {
            // First attempt to get a V_1_2 instance of the Wifi HAL.
            android.hardware.wifi.V1_2.IWifiChip iWifiChipV12 = getWifiChipForV1_2Mockable();
            if (iWifiChipV12 != null) {
                return selectTxPowerScenario_1_2(iWifiChipV12, sarInfo);
            }

            // Now attempt to get a V_1_1 instance of the Wifi HAL.
            android.hardware.wifi.V1_1.IWifiChip iWifiChipV11 = getWifiChipForV1_1Mockable();
            if (iWifiChipV11 != null) {
                return selectTxPowerScenario_1_1(iWifiChipV11, sarInfo);
            }

            // HAL version does not support SAR
            return false;
        }
    }

    private boolean selectTxPowerScenario_1_1(
            android.hardware.wifi.V1_1.IWifiChip iWifiChip, SarInfo sarInfo) {
        WifiStatus status;
        try {
            if (sarPowerBackoffRequired_1_1(sarInfo)) {
                // Power backoff is needed, so calculate the required scenario,
                // and attempt to set it.
                int halScenario = frameworkToHalTxPowerScenario_1_1(sarInfo);
                if (sarInfo.setSarScenarioNeeded(halScenario)) {
                    status = iWifiChip.selectTxPowerScenario(halScenario);
                    if (ok(status)) {
                        mLog.d("Setting SAR scenario to " + halScenario);
                        return true;
                    } else {
                        mLog.e("Failed to set SAR scenario to " + halScenario);
                        return false;
                    }
                }

                // Reaching here means setting SAR scenario would be redundant,
                // do nothing and return with success.
                return true;
            }

            // We don't need to perform power backoff, so attempt to reset SAR scenario.
            if (sarInfo.resetSarScenarioNeeded()) {
                status = iWifiChip.resetTxPowerScenario();
                if (ok(status)) {
                    mLog.d("Resetting SAR scenario");
                    return true;
                } else {
                    mLog.e("Failed to reset SAR scenario");
                    return false;
                }
            }

            // Resetting SAR scenario would be redundant,
            // do nothing and return with success.
            return true;
        } catch (RemoteException e) {
            handleRemoteException(e);
            return false;
        } catch (IllegalArgumentException e) {
            mLog.err("Illegal argument for selectTxPowerScenario_1_1()").c(e.toString()).flush();
            return false;
        }
    }

    private boolean selectTxPowerScenario_1_2(
            android.hardware.wifi.V1_2.IWifiChip iWifiChip, SarInfo sarInfo) {
        WifiStatus status;
        try {
            if (sarPowerBackoffRequired_1_2(sarInfo)) {
                // Power backoff is needed, so calculate the required scenario,
                // and attempt to set it.
                int halScenario = frameworkToHalTxPowerScenario_1_2(sarInfo);
                if (sarInfo.setSarScenarioNeeded(halScenario)) {
                    status = iWifiChip.selectTxPowerScenario_1_2(halScenario);
                    if (ok(status)) {
                        mLog.d("Setting SAR scenario to " + halScenario);
                        return true;
                    } else {
                        mLog.e("Failed to set SAR scenario to " + halScenario);
                        return false;
                    }
                }

                // Reaching here means setting SAR scenario would be redundant,
                // do nothing and return with success.
                return true;
            }

            // We don't need to perform power backoff, so attempt to reset SAR scenario.
            if (sarInfo.resetSarScenarioNeeded()) {
                status = iWifiChip.resetTxPowerScenario();
                if (ok(status)) {
                    mLog.d("Resetting SAR scenario");
                    return true;
                } else {
                    mLog.e("Failed to reset SAR scenario");
                    return false;
                }
            }

            // Resetting SAR scenario would be redundant,
            // do nothing and return with success.
            return true;
        } catch (RemoteException e) {
            handleRemoteException(e);
            return false;
        } catch (IllegalArgumentException e) {
            mLog.err("Illegal argument for selectTxPowerScenario_1_2()").c(e.toString()).flush();
            return false;
        }
    }

    /**
     * Enable/Disable low-latency mode
     *
     * @param enabled true to enable low-latency mode, false to disable it
     */
    public boolean setLowLatencyMode(boolean enabled) {
        synchronized (sLock) {
            android.hardware.wifi.V1_3.IWifiChip iWifiChipV13 = getWifiChipForV1_3Mockable();
            if (iWifiChipV13 != null) {
                try {
                    int mode;
                    if (enabled) {
                        mode = android.hardware.wifi.V1_3.IWifiChip.LatencyMode.LOW;
                    } else {
                        mode = android.hardware.wifi.V1_3.IWifiChip.LatencyMode.NORMAL;
                    }

                    WifiStatus status = iWifiChipV13.setLatencyMode(mode);
                    if (ok(status)) {
                        mVerboseLog.d("Setting low-latency mode to " + enabled);
                        return true;
                    } else {
                        mLog.e("Failed to set low-latency mode to " + enabled);
                        return false;
                    }
                } catch (RemoteException e) {
                    handleRemoteException(e);
                    return false;
                }
            }

            // HAL version does not support this api
            return false;
        }
    }

    /**
     * Returns whether STA + AP concurrency is supported or not.
     */
    public boolean isStaApConcurrencySupported() {
        synchronized (sLock) {
            return mHalDeviceManager.canSupportIfaceCombo(new SparseArray<Integer>() {{
                    put(IfaceType.STA, 1);
                    put(IfaceType.AP, 1);
                }});
        }
    }

    /**
     * Returns whether STA + STA concurrency is supported or not.
     */
    public boolean isStaStaConcurrencySupported() {
        synchronized (sLock) {
            return mHalDeviceManager.canSupportIfaceCombo(new SparseArray<Integer>() {{
                    put(IfaceType.STA, 2);
                }});
        }
    }

    /**
     * Returns whether a new AP iface can be created or not.
     */
    public boolean isItPossibleToCreateApIface(@NonNull WorkSource requestorWs) {
        synchronized (sLock) {
            return mHalDeviceManager.isItPossibleToCreateIface(IfaceType.AP, requestorWs);
        }
    }

    /**
     * Returns whether a new STA iface can be created or not.
     */
    public boolean isItPossibleToCreateStaIface(@NonNull WorkSource requestorWs) {
        synchronized (sLock) {
            return mHalDeviceManager.isItPossibleToCreateIface(IfaceType.STA, requestorWs);
        }

    }
    /**
     * Set primary connection when multiple STA ifaces are active.
     *
     * @param ifaceName Name of the interface.
     * @return true for success
     */
    public boolean setMultiStaPrimaryConnection(@NonNull String ifaceName) {
        if (TextUtils.isEmpty(ifaceName)) return boolResult(false);
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
                if (iWifiChipV15 == null) return boolResult(false);
                WifiStatus status = iWifiChipV15.setMultiStaPrimaryConnection(ifaceName);
                if (!ok(status)) return false;
                return true;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    private byte frameworkMultiStaUseCaseToHidl(@WifiNative.MultiStaUseCase int useCase)
            throws IllegalArgumentException {
        switch (useCase) {
            case WifiNative.DUAL_STA_TRANSIENT_PREFER_PRIMARY:
                return MultiStaUseCase.DUAL_STA_TRANSIENT_PREFER_PRIMARY;
            case WifiNative.DUAL_STA_NON_TRANSIENT_UNBIASED:
                return MultiStaUseCase.DUAL_STA_NON_TRANSIENT_UNBIASED;
            default:
                throw new IllegalArgumentException("Invalid use case " + useCase);
        }
    }

    /**
     * Set use-case when multiple STA ifaces are active.
     *
     * @param useCase one of the use cases.
     * @return true for success
     */
    public boolean setMultiStaUseCase(@WifiNative.MultiStaUseCase int useCase) {
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
                if (iWifiChipV15 == null) return boolResult(false);
                WifiStatus status = iWifiChipV15.setMultiStaUseCase(
                        frameworkMultiStaUseCaseToHidl(useCase));
                if (!ok(status)) return false;
                return true;
            } catch (IllegalArgumentException e) {
                mLog.e("Invalid use case " + e);
                return boolResult(false);
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    /**
     * Notify scan mode state to driver to save power in scan-only mode.
     *
     * @param ifaceName Name of the interface.
     * @param enable whether is in scan-only mode
     * @return true for success
     */
    public boolean setScanMode(@NonNull String ifaceName, boolean enable) {
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiStaIface iface =
                        getWifiStaIfaceForV1_5Mockable(ifaceName);
                if (iface != null) {
                    WifiStatus status = iface.setScanMode(enable);
                    if (!ok(status)) return false;
                    return true;
                }
                return false;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return false;
            }
        }
    }

    // This creates a blob of IE elements from the array received.
    // TODO: This ugly conversion can be removed if we put IE elements in ScanResult.
    private static byte[] hidlIeArrayToFrameworkIeBlob(ArrayList<WifiInformationElement> ies) {
        if (ies == null || ies.isEmpty()) return new byte[0];
        ArrayList<Byte> ieBlob = new ArrayList<>();
        for (WifiInformationElement ie : ies) {
            ieBlob.add(ie.id);
            ieBlob.addAll(ie.data);
        }
        return NativeUtil.byteArrayFromArrayList(ieBlob);
    }

    // This is only filling up the fields of Scan Result used by Gscan clients.
    private static ScanResult hidlToFrameworkScanResult(StaScanResult scanResult) {
        if (scanResult == null) return null;
        ScanResult frameworkScanResult = new ScanResult();
        frameworkScanResult.SSID = NativeUtil.encodeSsid(scanResult.ssid);
        frameworkScanResult.wifiSsid =
                WifiSsid.createFromByteArray(NativeUtil.byteArrayFromArrayList(scanResult.ssid));
        frameworkScanResult.BSSID = NativeUtil.macAddressFromByteArray(scanResult.bssid);
        frameworkScanResult.level = scanResult.rssi;
        frameworkScanResult.frequency = scanResult.frequency;
        frameworkScanResult.timestamp = scanResult.timeStampInUs;
        return frameworkScanResult;
    }

    private static ScanResult[] hidlToFrameworkScanResults(ArrayList<StaScanResult> scanResults) {
        if (scanResults == null || scanResults.isEmpty()) return new ScanResult[0];
        ScanResult[] frameworkScanResults = new ScanResult[scanResults.size()];
        int i = 0;
        for (StaScanResult scanResult : scanResults) {
            frameworkScanResults[i++] = hidlToFrameworkScanResult(scanResult);
        }
        return frameworkScanResults;
    }

    /**
     * This just returns whether the scan was interrupted or not.
     */
    private static int hidlToFrameworkScanDataFlags(int flag) {
        if (flag == StaScanDataFlagMask.INTERRUPTED) {
            return 1;
        } else {
            return 0;
        }
    }

    private static WifiScanner.ScanData[] hidlToFrameworkScanDatas(
            int cmdId, ArrayList<StaScanData> scanDatas) {
        if (scanDatas == null || scanDatas.isEmpty()) return new WifiScanner.ScanData[0];
        WifiScanner.ScanData[] frameworkScanDatas = new WifiScanner.ScanData[scanDatas.size()];
        int i = 0;
        for (StaScanData scanData : scanDatas) {
            int flags = hidlToFrameworkScanDataFlags(scanData.flags);
            ScanResult[] frameworkScanResults = hidlToFrameworkScanResults(scanData.results);
            frameworkScanDatas[i++] =
                    new WifiScanner.ScanData(cmdId, flags, scanData.bucketsScanned,
                            WifiScanner.WIFI_BAND_UNSPECIFIED, frameworkScanResults);
        }
        return frameworkScanDatas;
    }

    /**
     * Callback for events on the STA interface.
     */
    private class StaIfaceEventCallback extends IWifiStaIfaceEventCallback.Stub {
        @Override
        public void onBackgroundScanFailure(int cmdId) {
            mVerboseLog.d("onBackgroundScanFailure " + cmdId);
            WifiNative.ScanEventHandler eventHandler;
            synchronized (sLock) {
                if (mScan == null || cmdId != mScan.cmdId) return;
                eventHandler = mScan.eventHandler;
            }
            eventHandler.onScanStatus(WifiNative.WIFI_SCAN_FAILED);
        }

        @Override
        public void onBackgroundFullScanResult(
                int cmdId, int bucketsScanned, StaScanResult result) {
            mVerboseLog.d("onBackgroundFullScanResult " + cmdId);
            WifiNative.ScanEventHandler eventHandler;
            synchronized (sLock) {
                if (mScan == null || cmdId != mScan.cmdId) return;
                eventHandler = mScan.eventHandler;
            }
            eventHandler.onFullScanResult(hidlToFrameworkScanResult(result), bucketsScanned);
        }

        @Override
        public void onBackgroundScanResults(int cmdId, ArrayList<StaScanData> scanDatas) {
            mVerboseLog.d("onBackgroundScanResults " + cmdId);
            WifiNative.ScanEventHandler eventHandler;
            // WifiScanner currently uses the results callback to fetch the scan results.
            // So, simulate that by sending out the notification and then caching the results
            // locally. This will then be returned to WifiScanner via getScanResults.
            synchronized (sLock) {
                if (mScan == null || cmdId != mScan.cmdId) return;
                eventHandler = mScan.eventHandler;
                mScan.latestScanResults = hidlToFrameworkScanDatas(cmdId, scanDatas);
            }
            eventHandler.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);
        }

        @Override
        public void onRssiThresholdBreached(int cmdId, byte[/* 6 */] currBssid, int currRssi) {
            mVerboseLog.d("onRssiThresholdBreached " + cmdId + "currRssi " + currRssi);
            WifiNative.WifiRssiEventHandler eventHandler;
            synchronized (sLock) {
                if (mWifiRssiEventHandler == null || cmdId != sRssiMonCmdId) return;
                eventHandler = mWifiRssiEventHandler;
            }
            eventHandler.onRssiThresholdBreached((byte) currRssi);
        }
    }

    /**
     * Callback for events on the chip.
     */
    private class ChipEventCallback extends IWifiChipEventCallback.Stub {
        @Override
        public void onChipReconfigured(int modeId) {
            mVerboseLog.d("onChipReconfigured " + modeId);
        }

        @Override
        public void onChipReconfigureFailure(WifiStatus status) {
            mVerboseLog.d("onChipReconfigureFailure " + status);
        }

        public void onIfaceAdded(int type, String name) {
            mVerboseLog.d("onIfaceAdded " + type + ", name: " + name);
        }

        @Override
        public void onIfaceRemoved(int type, String name) {
            mVerboseLog.d("onIfaceRemoved " + type + ", name: " + name);
        }

        @Override
        public void onDebugRingBufferDataAvailable(
                WifiDebugRingBufferStatus status, java.util.ArrayList<Byte> data) {
            //TODO(b/35875078) Reinstate logging when execessive callbacks are fixed
            // mVerboseLog.d("onDebugRingBufferDataAvailable " + status);
            mHalEventHandler.post(() -> {
                WifiNative.WifiLoggerEventHandler eventHandler;
                synchronized (sLock) {
                    if (mLogEventHandler == null || status == null || data == null) return;
                    eventHandler = mLogEventHandler;
                }
                // Because |sLock| has been released, there is a chance that we'll execute
                // a spurious callback (after someone has called resetLogHandler()).
                //
                // However, the alternative risks deadlock. Consider:
                // [T1.1] WifiDiagnostics.captureBugReport()
                // [T1.2] -- acquire WifiDiagnostics object's intrinsic lock
                // [T1.3]    -> WifiVendorHal.getRingBufferData()
                // [T1.4]       -- acquire WifiVendorHal.sLock
                // [T2.1] <lambda>()
                // [T2.2] -- acquire WifiVendorHal.sLock
                // [T2.3]    -> WifiDiagnostics.onRingBufferData()
                // [T2.4]       -- acquire WifiDiagnostics object's intrinsic lock
                //
                // The problem here is that the two threads acquire the locks in opposite order.
                // If, for example, T2.2 executes between T1.2 and 1.4, then T1 and T2
                // will be deadlocked.
                int sizeBefore = data.size();
                boolean conversionFailure = false;
                try {
                    eventHandler.onRingBufferData(
                            ringBufferStatus(status), NativeUtil.byteArrayFromArrayList(data));
                    int sizeAfter = data.size();
                    if (sizeAfter != sizeBefore) {
                        conversionFailure = true;
                    }
                } catch (ArrayIndexOutOfBoundsException e) {
                    conversionFailure = true;
                }
                if (conversionFailure) {
                    Log.wtf("WifiVendorHal", "Conversion failure detected in "
                            + "onDebugRingBufferDataAvailable. "
                            + "The input ArrayList |data| is potentially corrupted. "
                            + "Starting size=" + sizeBefore + ", "
                            + "final size=" + data.size());
                }
            });
        }

        @Override
        public void onDebugErrorAlert(int errorCode, java.util.ArrayList<Byte> debugData) {
            mLog.w("onDebugErrorAlert " + errorCode);
            mHalEventHandler.post(() -> {
                WifiNative.WifiLoggerEventHandler eventHandler;
                synchronized (sLock) {
                    if (mLogEventHandler == null || debugData == null) return;
                    eventHandler = mLogEventHandler;
                }
                // See comment in onDebugRingBufferDataAvailable(), for an explanation
                // of why this callback is invoked without |sLock| held.
                eventHandler.onWifiAlert(
                        errorCode, NativeUtil.byteArrayFromArrayList(debugData));
            });
        }
    }

    private boolean areSameIfaceNames(List<IfaceInfo> ifaceList1, List<IfaceInfo> ifaceList2) {
        List<String> ifaceNamesList1 = ifaceList1
                .stream()
                .map(i -> i.name)
                .collect(Collectors.toList());
        List<String> ifaceNamesList2 = ifaceList2
                .stream()
                .map(i -> i.name)
                .collect(Collectors.toList());
        return ifaceNamesList1.containsAll(ifaceNamesList2);
    }

    /**
     * Callback for events on the 1.2 chip.
     */
    private class ChipEventCallbackV12 extends
            android.hardware.wifi.V1_2.IWifiChipEventCallback.Stub {
        @Override
        public void onChipReconfigured(int modeId) {
            mIWifiChipEventCallback.onChipReconfigured(modeId);
        }

        @Override
        public void onChipReconfigureFailure(WifiStatus status) {
            mIWifiChipEventCallback.onChipReconfigureFailure(status);
        }

        public void onIfaceAdded(int type, String name) {
            mIWifiChipEventCallback.onIfaceAdded(type, name);
        }

        @Override
        public void onIfaceRemoved(int type, String name) {
            mIWifiChipEventCallback.onIfaceRemoved(type, name);
        }

        @Override
        public void onDebugRingBufferDataAvailable(
                WifiDebugRingBufferStatus status, java.util.ArrayList<Byte> data) {
            mIWifiChipEventCallback.onDebugRingBufferDataAvailable(status, data);
        }

        @Override
        public void onDebugErrorAlert(int errorCode, java.util.ArrayList<Byte> debugData) {
            mIWifiChipEventCallback.onDebugErrorAlert(errorCode, debugData);
        }

        @Override
        public void onRadioModeChange(ArrayList<RadioModeInfo> radioModeInfoList) {
            mVerboseLog.d("onRadioModeChange " + radioModeInfoList);
            WifiNative.VendorHalRadioModeChangeEventHandler handler;
            synchronized (sLock) {
                if (mRadioModeChangeEventHandler == null || radioModeInfoList == null) return;
                handler = mRadioModeChangeEventHandler;
            }
            // Should only contain 1 or 2 radio infos.
            if (radioModeInfoList.size() == 0 || radioModeInfoList.size() > 2) {
                mLog.e("Unexpected number of radio info in list " + radioModeInfoList.size());
                return;
            }
            RadioModeInfo radioModeInfo0 = radioModeInfoList.get(0);
            RadioModeInfo radioModeInfo1 =
                    radioModeInfoList.size() == 2 ? radioModeInfoList.get(1) : null;
            // Number of ifaces on each radio should be equal.
            if (radioModeInfo1 != null
                    && radioModeInfo0.ifaceInfos.size() != radioModeInfo1.ifaceInfos.size()) {
                mLog.e("Unexpected number of iface info in list "
                        + radioModeInfo0.ifaceInfos.size() + ", "
                        + radioModeInfo1.ifaceInfos.size());
                return;
            }
            int numIfacesOnEachRadio = radioModeInfo0.ifaceInfos.size();
            // Only 1 or 2 ifaces should be present on each radio.
            if (numIfacesOnEachRadio == 0 || numIfacesOnEachRadio > 2) {
                mLog.e("Unexpected number of iface info in list " + numIfacesOnEachRadio);
                return;
            }
            Runnable runnable = null;
            // 2 ifaces simultaneous on 2 radios.
            if (radioModeInfoList.size() == 2 && numIfacesOnEachRadio == 1) {
                // Iface on radio0 should be different from the iface on radio1 for DBS & SBS.
                if (areSameIfaceNames(radioModeInfo0.ifaceInfos, radioModeInfo1.ifaceInfos)) {
                    mLog.e("Unexpected for both radio infos to have same iface");
                    return;
                }
                if (radioModeInfo0.bandInfo != radioModeInfo1.bandInfo) {
                    runnable = () -> {
                        handler.onDbs();
                    };
                } else {
                    runnable = () -> {
                        handler.onSbs(radioModeInfo0.bandInfo);
                    };
                }
            // 2 ifaces time sharing on 1 radio.
            } else if (radioModeInfoList.size() == 1 && numIfacesOnEachRadio == 2) {
                IfaceInfo ifaceInfo0 = radioModeInfo0.ifaceInfos.get(0);
                IfaceInfo ifaceInfo1 = radioModeInfo0.ifaceInfos.get(1);
                if (ifaceInfo0.channel != ifaceInfo1.channel) {
                    runnable = () -> {
                        handler.onMcc(radioModeInfo0.bandInfo);
                    };
                } else {
                    runnable = () -> {
                        handler.onScc(radioModeInfo0.bandInfo);
                    };
                }
            } else {
                // Not concurrency scenario, uninteresting...
            }
            if (runnable != null) mHalEventHandler.post(runnable);
        }
    }

    /**
     * Callback for events on the 1.4 chip.
     */
    private class ChipEventCallbackV14 extends
            android.hardware.wifi.V1_4.IWifiChipEventCallback.Stub {
        @Override
        public void onChipReconfigured(int modeId) {
            mIWifiChipEventCallback.onChipReconfigured(modeId);
        }

        @Override
        public void onChipReconfigureFailure(WifiStatus status) {
            mIWifiChipEventCallback.onChipReconfigureFailure(status);
        }

        public void onIfaceAdded(int type, String name) {
            mIWifiChipEventCallback.onIfaceAdded(type, name);
        }

        @Override
        public void onIfaceRemoved(int type, String name) {
            mIWifiChipEventCallback.onIfaceRemoved(type, name);
        }

        @Override
        public void onDebugRingBufferDataAvailable(
                WifiDebugRingBufferStatus status, java.util.ArrayList<Byte> data) {
            mIWifiChipEventCallback.onDebugRingBufferDataAvailable(status, data);
        }

        @Override
        public void onDebugErrorAlert(int errorCode, java.util.ArrayList<Byte> debugData) {
            mIWifiChipEventCallback.onDebugErrorAlert(errorCode, debugData);
        }

        @Override
        public void onRadioModeChange(
                ArrayList<android.hardware.wifi.V1_2.IWifiChipEventCallback.RadioModeInfo>
                radioModeInfoList) {
            mIWifiChipEventCallbackV12.onRadioModeChange(radioModeInfoList);
        }

        @Override
        public void onRadioModeChange_1_4(ArrayList<RadioModeInfo> radioModeInfoList) {
            mVerboseLog.d("onRadioModeChange_1_4 " + radioModeInfoList);
            WifiNative.VendorHalRadioModeChangeEventHandler handler;
            synchronized (sLock) {
                if (mRadioModeChangeEventHandler == null || radioModeInfoList == null) return;
                handler = mRadioModeChangeEventHandler;
            }
            // Should only contain 1 or 2 radio infos.
            if (radioModeInfoList.size() == 0 || radioModeInfoList.size() > 2) {
                mLog.e("Unexpected number of radio info in list " + radioModeInfoList.size());
                return;
            }
            RadioModeInfo radioModeInfo0 = radioModeInfoList.get(0);
            RadioModeInfo radioModeInfo1 =
                    radioModeInfoList.size() == 2 ? radioModeInfoList.get(1) : null;
            // Number of ifaces on each radio should be equal.
            if (radioModeInfo1 != null
                    && radioModeInfo0.ifaceInfos.size() != radioModeInfo1.ifaceInfos.size()) {
                mLog.e("Unexpected number of iface info in list "
                        + radioModeInfo0.ifaceInfos.size() + ", "
                        + radioModeInfo1.ifaceInfos.size());
                return;
            }
            int numIfacesOnEachRadio = radioModeInfo0.ifaceInfos.size();
            // Only 1 or 2 ifaces should be present on each radio.
            if (numIfacesOnEachRadio == 0 || numIfacesOnEachRadio > 2) {
                mLog.e("Unexpected number of iface info in list " + numIfacesOnEachRadio);
                return;
            }
            Runnable runnable = null;
            // 2 ifaces simultaneous on 2 radios.
            if (radioModeInfoList.size() == 2 && numIfacesOnEachRadio == 1) {
                // Iface on radio0 should be different from the iface on radio1 for DBS & SBS.
                if (areSameIfaceNames(radioModeInfo0.ifaceInfos, radioModeInfo1.ifaceInfos)) {
                    mLog.e("Unexpected for both radio infos to have same iface");
                    return;
                }
                if (radioModeInfo0.bandInfo != radioModeInfo1.bandInfo) {
                    runnable = () -> {
                        handler.onDbs();
                    };
                } else {
                    runnable = () -> {
                        handler.onSbs(radioModeInfo0.bandInfo);
                    };
                }
            // 2 ifaces time sharing on 1 radio.
            } else if (radioModeInfoList.size() == 1 && numIfacesOnEachRadio == 2) {
                IfaceInfo ifaceInfo0 = radioModeInfo0.ifaceInfos.get(0);
                IfaceInfo ifaceInfo1 = radioModeInfo0.ifaceInfos.get(1);
                if (ifaceInfo0.channel != ifaceInfo1.channel) {
                    runnable = () -> {
                        handler.onMcc(radioModeInfo0.bandInfo);
                    };
                } else {
                    runnable = () -> {
                        handler.onScc(radioModeInfo0.bandInfo);
                    };
                }
            } else {
                // Not concurrency scenario, uninteresting...
            }
            if (runnable != null) mHalEventHandler.post(runnable);
        }
    }

    /**
     * Hal Device Manager callbacks.
     */
    public class HalDeviceManagerStatusListener implements HalDeviceManager.ManagerStatusListener {
        @Override
        public void onStatusChanged() {
            boolean isReady = mHalDeviceManager.isReady();
            boolean isStarted = mHalDeviceManager.isStarted();

            mVerboseLog.i("Device Manager onStatusChanged. isReady(): " + isReady
                    + ", isStarted(): " + isStarted);
            if (!isReady) {
                // Probably something unpleasant, e.g. the server died
                WifiNative.VendorHalDeathEventHandler handler;
                synchronized (sLock) {
                    clearState();
                    handler = mDeathEventHandler;
                }
                if (handler != null) {
                    handler.onDeath();
                }
            }
        }
    }

    /**
     * Trigger subsystem restart in vendor side
     */
    public boolean startSubsystemRestart() {
        synchronized (sLock) {
            android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
            if (iWifiChipV15 != null) {
                try {
                    return ok(iWifiChipV15.triggerSubsystemRestart());
                } catch (RemoteException e) {
                    handleRemoteException(e);
                    return false;
                }
            }
            // HAL version does not support this api
            return false;
        }
    }

    /**
     * Convert framework's operational mode to HAL's operational mode.
     */
    private int frameworkToHalIfaceMode(@WifiAvailableChannel.OpMode int mode) {
        int halMode = 0;
        if ((mode & WifiAvailableChannel.OP_MODE_STA) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_STA;
        }
        if ((mode & WifiAvailableChannel.OP_MODE_SAP) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_SOFTAP;
        }
        if ((mode & WifiAvailableChannel.OP_MODE_WIFI_DIRECT_CLI) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_P2P_CLIENT;
        }
        if ((mode & WifiAvailableChannel.OP_MODE_WIFI_DIRECT_GO) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_P2P_GO;
        }
        if ((mode & WifiAvailableChannel.OP_MODE_WIFI_AWARE) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_NAN;
        }
        if ((mode & WifiAvailableChannel.OP_MODE_TDLS) != 0) {
            halMode |= WifiIfaceMode.IFACE_MODE_TDLS;
        }
        return halMode;
    }

    /**
     * Convert from HAL's operational mode to framework's operational mode.
     */
    private @WifiAvailableChannel.OpMode int frameworkFromHalIfaceMode(int halMode) {
        int mode = 0;
        if ((halMode & WifiIfaceMode.IFACE_MODE_STA) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_STA;
        }
        if ((halMode & WifiIfaceMode.IFACE_MODE_SOFTAP) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_SAP;
        }
        if ((halMode & WifiIfaceMode.IFACE_MODE_P2P_CLIENT) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_WIFI_DIRECT_CLI;
        }
        if ((halMode & WifiIfaceMode.IFACE_MODE_P2P_GO) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_WIFI_DIRECT_GO;
        }
        if ((halMode & WifiIfaceMode.IFACE_MODE_NAN) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_WIFI_AWARE;
        }
        if ((halMode & WifiIfaceMode.IFACE_MODE_TDLS) != 0) {
            mode |= WifiAvailableChannel.OP_MODE_TDLS;
        }
        return mode;
    }

    /**
     * Convert framework's WifiAvailableChannel.FILTER_* to HAL's UsableChannelFilter.
     */
    private int frameworkToHalUsableFilter(@WifiAvailableChannel.Filter int filter) {
        int halFilter = 0;  // O implies no additional filter other than regulatory (default)

        if ((filter & WifiAvailableChannel.FILTER_CONCURRENCY) != 0) {
            halFilter |= UsableChannelFilter.CONCURRENCY;
        }
        if ((filter & WifiAvailableChannel.FILTER_CELLULAR_COEXISTENCE) != 0) {
            halFilter |= UsableChannelFilter.CELLULAR_COEXISTENCE;
        }
        return halFilter;
    }

    /**
     * Retrieve the list of usable Wifi channels.
     */
    public List<WifiAvailableChannel> getUsableChannels(
            @WifiScanner.WifiBand int band,
            @WifiAvailableChannel.OpMode int mode,
            @WifiAvailableChannel.Filter int filter) {
        synchronized (sLock) {
            try {
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 = getWifiChipForV1_5Mockable();
                if (iWifiChipV15 == null) {
                    return null;
                }
                Mutable<List<WifiAvailableChannel>> answer = new Mutable<>();
                iWifiChipV15.getUsableChannels(
                        makeWifiBandFromFrameworkBand(band),
                        frameworkToHalIfaceMode(mode),
                        frameworkToHalUsableFilter(filter), (status, channels) -> {
                            if (!ok(status)) return;
                            answer.value = new ArrayList<>();
                            for (WifiUsableChannel ch : channels) {
                                answer.value.add(new WifiAvailableChannel(ch.channel,
                                        frameworkFromHalIfaceMode(ch.ifaceModeMask)));
                            }
                        });
                return answer.value;
            } catch (RemoteException e) {
                handleRemoteException(e);
                return null;
            } catch (IllegalArgumentException e) {
                mLog.e("Illegal argument for getUsableChannels() " + e);
                return null;
            }
        }
    }
}
