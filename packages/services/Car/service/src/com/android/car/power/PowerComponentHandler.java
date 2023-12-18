/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.power;

import static android.car.hardware.power.PowerComponent.BLUETOOTH;
import static android.car.hardware.power.PowerComponent.DISPLAY;
import static android.car.hardware.power.PowerComponent.VOICE_INTERACTION;
import static android.car.hardware.power.PowerComponent.WIFI;
import static android.car.hardware.power.PowerComponentUtil.FIRST_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.INVALID_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.LAST_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.powerComponentToString;
import static android.car.hardware.power.PowerComponentUtil.toPowerComponent;

import android.annotation.Nullable;
import android.bluetooth.BluetoothAdapter;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.PowerComponent;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.AtomicFile;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;
import android.util.SparseBooleanArray;

import com.android.car.CarLog;
import com.android.car.systeminterface.SystemInterface;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.app.IVoiceInteractionManagerService;
import com.android.server.utils.Slogf;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;

/**
 * Class that manages power components in the system. A power component mediator corresponding to a
 * power component is created and registered to this class. A power component mediator encapsulates
 * the function of powering on/off.
 */
@VisibleForTesting
public final class PowerComponentHandler {
    private static final String TAG = CarLog.tagFor(PowerComponentHandler.class);
    private static final String FORCED_OFF_COMPONENTS_FILENAME =
            "forced_off_components";

    private final Object mLock = new Object();
    private final Context mContext;
    private final SystemInterface mSystemInterface;
    private final AtomicFile mOffComponentsByUserFile;
    private final SparseArray<PowerComponentMediator> mPowerComponentMediators =
            new SparseArray<>();
    @GuardedBy("mLock")
    private final SparseBooleanArray mComponentStates =
            new SparseBooleanArray(LAST_POWER_COMPONENT - FIRST_POWER_COMPONENT + 1);
    @GuardedBy("mLock")
    private final SparseBooleanArray mComponentsOffByPolicy = new SparseBooleanArray();
    @GuardedBy("mLock")
    private final SparseBooleanArray mLastModifiedComponents = new SparseBooleanArray();
    private final IVoiceInteractionManagerService mVoiceInteractionServiceHolder;
    private final PackageManager mPackageManager;

    @GuardedBy("mLock")
    private String mCurrentPolicyId = "";

    PowerComponentHandler(Context context, SystemInterface systemInterface) {
        this(context, systemInterface, /* voiceInteractionService= */ null,
                new AtomicFile(new File(systemInterface.getSystemCarDir(),
                        FORCED_OFF_COMPONENTS_FILENAME)));
    }

    public PowerComponentHandler(Context context, SystemInterface systemInterface,
            IVoiceInteractionManagerService voiceInteractionService,
            AtomicFile componentStateFile) {
        mContext = context;
        mPackageManager = mContext.getPackageManager();
        mSystemInterface = systemInterface;
        mVoiceInteractionServiceHolder = voiceInteractionService;
        mOffComponentsByUserFile = componentStateFile;
    }

    void init() {
        PowerComponentMediatorFactory factory = new PowerComponentMediatorFactory();
        synchronized (mLock) {
            readUserOffComponentsLocked();
            for (int component = FIRST_POWER_COMPONENT; component <= LAST_POWER_COMPONENT;
                    component++) {
                mComponentStates.put(component, false);
                PowerComponentMediator mediator = factory.createPowerComponent(component);
                String componentName = powerComponentToString(component);
                if (mediator == null) {
                    Slogf.w(TAG, "Power component(%s) is not valid or doesn't need a mediator",
                            componentName);
                    continue;
                }
                if (!mediator.isComponentAvailable()) {
                    Slogf.w(TAG, "Power component(%s) is not available", componentName);
                    continue;
                }
                mPowerComponentMediators.put(component, mediator);
            }
        }
    }

    CarPowerPolicy getAccumulatedPolicy() {
        synchronized (mLock) {
            int enabledComponentsCount = 0;
            int disabledComponentsCount = 0;
            for (int component = FIRST_POWER_COMPONENT; component <= LAST_POWER_COMPONENT;
                    component++) {
                if (mComponentStates.get(component, /* valueIfKeyNotFound= */ false)) {
                    enabledComponentsCount++;
                } else {
                    disabledComponentsCount++;
                }
            }
            int[] enabledComponents = new int[enabledComponentsCount];
            int[] disabledComponents = new int[disabledComponentsCount];
            int enabledIndex = 0;
            int disabledIndex = 0;
            for (int component = FIRST_POWER_COMPONENT; component <= LAST_POWER_COMPONENT;
                    component++) {
                if (mComponentStates.get(component, /* valueIfKeyNotFound= */ false)) {
                    enabledComponents[enabledIndex++] = component;
                } else {
                    disabledComponents[disabledIndex++] = component;
                }
            }
            return new CarPowerPolicy(mCurrentPolicyId, enabledComponents, disabledComponents);
        }
    }

    /**
     * Applies the given policy considering user setting.
     *
     * <p> If a component is the policy is not applied due to user setting, it is not notified to
     * listeners.
     */
    void applyPowerPolicy(CarPowerPolicy policy) {
        int[] enabledComponents = policy.getEnabledComponents();
        int[] disabledComponents = policy.getDisabledComponents();
        synchronized (mLock) {
            mLastModifiedComponents.clear();
            for (int i = 0; i < enabledComponents.length; i++) {
                int component = enabledComponents[i];
                if (setComponentEnabledLocked(component, /* enabled= */ true)) {
                    mLastModifiedComponents.put(component, /* value= */ true);
                }
            }
            for (int i = 0; i < disabledComponents.length; i++) {
                int component = disabledComponents[i];
                if (setComponentEnabledLocked(component, /* enabled= */ false)) {
                    mLastModifiedComponents.put(component, /* value= */ true);
                }
            }
            mCurrentPolicyId = policy.getPolicyId();
        }
    }

    boolean isComponentChanged(CarPowerPolicyFilter filter) {
        synchronized (mLock) {
            int[] components = filter.getComponents();
            for (int i = 0; i < components.length; i++) {
                if (mLastModifiedComponents.get(components[i], false)) {
                    return true;
                }
            }
            return false;
        }
    }

    void dump(IndentingPrintWriter writer) {
        synchronized (mLock) {
            writer.println("Power components state:");
            writer.increaseIndent();
            for (int component = FIRST_POWER_COMPONENT; component <= LAST_POWER_COMPONENT;
                    component++) {
                writer.printf("%s: %s\n", powerComponentToString(component),
                        mComponentStates.get(component, /* valueIfKeyNotFound= */ false)
                                ? "on" : "off");
            }
            writer.decreaseIndent();
            writer.println("Components powered off by power policy:");
            writer.increaseIndent();
            for (int i = 0; i < mComponentsOffByPolicy.size(); i++) {
                writer.println(powerComponentToString(mComponentsOffByPolicy.keyAt(i)));
            }
            writer.decreaseIndent();
            writer.print("Components changed by the last policy: ");
            writer.increaseIndent();
            for (int i = 0; i < mLastModifiedComponents.size(); i++) {
                if (i > 0) writer.print(", ");
                writer.print(powerComponentToString(mLastModifiedComponents.keyAt(i)));
            }
            writer.println();
            writer.decreaseIndent();
        }
    }

    /**
     * Modifies power component's state, considering user setting.
     *
     * @return {@code true} if power state is changed. Otherwise, {@code false}
     */
    private boolean setComponentEnabledLocked(int component, boolean enabled) {
        boolean oldState = mComponentStates.get(component, /* valueIfKeyNotFound= */ false);
        if (oldState == enabled) {
            return false;
        }

        mComponentStates.put(component, enabled);

        PowerComponentMediator mediator = mPowerComponentMediators.get(component);
        if (mediator == null) {
            Slogf.w(TAG, "%s doesn't have a mediator", powerComponentToString(component));
            return true;
        }

        boolean needPowerChange = false;
        if (mediator.isUserControllable()) {
            if (!enabled && mediator.isEnabled()) {
                mComponentsOffByPolicy.put(component, /* value= */ true);
                needPowerChange = true;
            }
            if (enabled && mComponentsOffByPolicy.get(component, /* valueIfKeyNotFound= */ false)) {
                mComponentsOffByPolicy.delete(component);
                needPowerChange = true;
            }
            if (needPowerChange) {
                writeUserOffComponentsLocked();
            }
        } else {
            needPowerChange = true;
        }

        if (needPowerChange) {
            mediator.setEnabled(enabled);
        }
        return true;
    }

    private void readUserOffComponentsLocked() {
        boolean invalid = false;
        mComponentsOffByPolicy.clear();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(mOffComponentsByUserFile.openRead(),
                        StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                int component = toPowerComponent(line.trim(), /* prefix= */ false);
                if (component == INVALID_POWER_COMPONENT) {
                    invalid = true;
                    break;
                }
                mComponentsOffByPolicy.put(component, /* value= */ true);
            }
        } catch (FileNotFoundException e) {
            // Behave as if there are no forced-off components.
            return;
        } catch (IOException e) {
            Slogf.w(TAG, "Failed to read %s: %s", FORCED_OFF_COMPONENTS_FILENAME, e);
            return;
        }
        if (invalid) {
            mOffComponentsByUserFile.delete();
        }
    }

    private void writeUserOffComponentsLocked() {
        FileOutputStream fos;
        try {
            fos = mOffComponentsByUserFile.startWrite();
        } catch (IOException e) {
            Slogf.e(TAG, e, "Cannot create %s", FORCED_OFF_COMPONENTS_FILENAME);
            return;
        }

        try (BufferedWriter writer = new BufferedWriter(
                new OutputStreamWriter(fos, StandardCharsets.UTF_8))) {
            for (int i = 0; i < mComponentsOffByPolicy.size(); i++) {
                if (!mComponentsOffByPolicy.valueAt(i)) {
                    continue;
                }
                writer.write(powerComponentToString(mComponentsOffByPolicy.keyAt(i)));
                writer.newLine();
            }
            writer.flush();
            mOffComponentsByUserFile.finishWrite(fos);
        } catch (IOException e) {
            mOffComponentsByUserFile.failWrite(fos);
            Slogf.e(TAG, e, "Writing %s failed", FORCED_OFF_COMPONENTS_FILENAME);
        }
    }

    abstract static class PowerComponentMediator {
        protected int mComponentId;

        PowerComponentMediator(int component) {
            mComponentId = component;
        }

        public boolean isComponentAvailable() {
            return false;
        }

        public boolean isUserControllable() {
            return false;
        }

        public boolean isEnabled() {
            return false;
        }

        public void setEnabled(boolean enabled) {}
    }

    // TODO(b/178824607): Check if power policy can turn on/off display as quickly as the existing
    // implementation.
    private final class DisplayPowerComponentMediator extends PowerComponentMediator {
        DisplayPowerComponentMediator() {
            super(DISPLAY);
        }

        @Override
        public boolean isComponentAvailable() {
            // It is assumed that display is supported in all vehicles.
            return true;
        }

        @Override
        public boolean isEnabled() {
            return mSystemInterface.isDisplayEnabled();
        }

        @Override
        public void setEnabled(boolean enabled) {
            mSystemInterface.setDisplayState(enabled);
            Slogf.d(TAG, "Display power component is %s", enabled ? "on" : "off");
        }
    }

    private final class WifiPowerComponentMediator extends PowerComponentMediator {
        private final WifiManager mWifiManager;

        WifiPowerComponentMediator() {
            super(WIFI);
            mWifiManager = mContext.getSystemService(WifiManager.class);
        }

        @Override
        public boolean isComponentAvailable() {
            return mPackageManager.hasSystemFeature(PackageManager.FEATURE_WIFI);
        }

        @Override
        public boolean isUserControllable() {
            return true;
        }

        @Override
        public boolean isEnabled() {
            return mWifiManager.isWifiEnabled();
        }

        @Override
        public void setEnabled(boolean enabled) {
            mWifiManager.setWifiEnabled(enabled);
            Slogf.d(TAG, "Wifi power component is %s", enabled ? "on" : "off");
        }
    }

    private final class VoiceInteractionPowerComponentMediator extends PowerComponentMediator {
        private final IVoiceInteractionManagerService mVoiceInteractionManagerService;

        private boolean mIsEnabled = true;

        VoiceInteractionPowerComponentMediator() {
            super(VOICE_INTERACTION);
            if (mVoiceInteractionServiceHolder == null) {
                mVoiceInteractionManagerService = IVoiceInteractionManagerService.Stub.asInterface(
                        ServiceManager.getService(Context.VOICE_INTERACTION_MANAGER_SERVICE));
            } else {
                mVoiceInteractionManagerService = mVoiceInteractionServiceHolder;
            }
        }

        @Override
        public boolean isComponentAvailable() {
            return mVoiceInteractionManagerService != null;
        }

        @Override
        public boolean isEnabled() {
            return mIsEnabled;
        }

        @Override
        public void setEnabled(boolean enabled) {
            try {
                mVoiceInteractionManagerService.setDisabled(!enabled);
                mIsEnabled = enabled;
                Slogf.d(TAG, "Voice Interaction power component is %s", enabled ? "on" : "off");
            } catch (RemoteException e) {
                Slogf.w(TAG, e, "IVoiceInteractionManagerService.setDisabled(%b) failed", !enabled);
            }
        }
    }

    private final class BluetoothPowerComponentMediator extends PowerComponentMediator {
        private final BluetoothAdapter mBluetoothAdapter;

        BluetoothPowerComponentMediator() {
            super(BLUETOOTH);
            mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        }

        @Override
        public boolean isComponentAvailable() {
            return mPackageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);
        }

        @Override
        public boolean isEnabled() {
            return mBluetoothAdapter.isEnabled();
        }

        @Override
        public void setEnabled(boolean enabled) {
            // No op
            Slogf.w(TAG, "Bluetooth power is controlled by "
                    + "com.android.car.BluetoothDeviceConnectionPolicy");
        }
    }

    private final class PowerComponentMediatorFactory {
        @Nullable
        PowerComponentMediator createPowerComponent(int component) {
            switch (component) {
                case PowerComponent.AUDIO:
                    // We don't control audio in framework level, because audio is enabled or
                    // disabled in audio HAL according to the current power policy.
                    return null;
                case PowerComponent.MEDIA:
                    return null;
                case PowerComponent.DISPLAY:
                    return new DisplayPowerComponentMediator();
                case PowerComponent.WIFI:
                    return new WifiPowerComponentMediator();
                case PowerComponent.CELLULAR:
                    return null;
                case PowerComponent.ETHERNET:
                    return null;
                case PowerComponent.PROJECTION:
                    return null;
                case PowerComponent.NFC:
                    return null;
                case PowerComponent.INPUT:
                    return null;
                case PowerComponent.VOICE_INTERACTION:
                    return new VoiceInteractionPowerComponentMediator();
                case PowerComponent.VISUAL_INTERACTION:
                    return null;
                case PowerComponent.TRUSTED_DEVICE_DETECTION:
                    return null;
                case PowerComponent.MICROPHONE:
                    // We don't control microphone in framework level, because microphone is enabled
                    // or disabled in audio HAL according to the current power policy.
                    return null;
                case PowerComponent.BLUETOOTH:
                    // com.android.car.BluetoothDeviceConnectionPolicy handles power state change.
                    // So, bluetooth mediator doesn't directly turn on/off BT, but it changes policy
                    // behavior, considering user intervetion.
                    return new BluetoothPowerComponentMediator();
                case PowerComponent.LOCATION:
                    // GNSS HAL handles power state change.
                    return null;
                case PowerComponent.CPU:
                    return null;
                default:
                    Slogf.w(TAG, "Unknown component(%d)", component);
                    return null;
            }
        }
    }

    static class PowerComponentException extends Exception {
        PowerComponentException(String message) {
            super(message);
        }
    }
}
