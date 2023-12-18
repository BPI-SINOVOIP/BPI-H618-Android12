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

import static android.car.hardware.power.PowerComponentUtil.FIRST_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.INVALID_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.LAST_POWER_COMPONENT;
import static android.car.hardware.power.PowerComponentUtil.powerComponentToString;
import static android.car.hardware.power.PowerComponentUtil.toPowerComponent;

import static org.xmlpull.v1.XmlPullParser.END_DOCUMENT;
import static org.xmlpull.v1.XmlPullParser.END_TAG;
import static org.xmlpull.v1.XmlPullParser.START_TAG;
import static org.xmlpull.v1.XmlPullParser.TEXT;

import android.annotation.Nullable;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.PowerComponent;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReport;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.Xml;

import com.android.car.CarLog;
import com.android.car.CarServiceUtils;
import com.android.internal.annotations.VisibleForTesting;
import com.android.server.utils.Slogf;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Helper class to read and manage vendor power policies.
 *
 * <p>{@code CarPowerManagementService} manages power policies through {@code PolicyReader}. This
 * class is not thread-safe, and must be used in the main thread or with additional serialization.
 */
public final class PolicyReader {
    public static final String POWER_STATE_WAIT_FOR_VHAL = "WaitForVHAL";
    public static final String POWER_STATE_ON = "On";

    static final String SYSTEM_POWER_POLICY_PREFIX = "system_power_policy_";
    // Preemptive system power policy used for disabling user interaction in Silent Mode or Garage
    // Mode.
    static final String POWER_POLICY_ID_NO_USER_INTERACTION = SYSTEM_POWER_POLICY_PREFIX
            + "no_user_interaction";
    // Preemptive system power policy used for preparing Suspend-to-RAM.
    static final String POWER_POLICY_ID_SUSPEND_TO_RAM = SYSTEM_POWER_POLICY_PREFIX
            + "suspend_to_ram";
    // Non-preemptive system power policy used for turning all components on.
    static final String POWER_POLICY_ID_ALL_ON = SYSTEM_POWER_POLICY_PREFIX + "all_on";
    // Non-preemptive system power policy used to represent minimally on state.
    static final String POWER_POLICY_ID_INITIAL_ON = SYSTEM_POWER_POLICY_PREFIX + "initial_on";

    static final int INVALID_POWER_STATE = -1;

    private static final String TAG = CarLog.tagFor(PolicyReader.class);
    private static final String VENDOR_POLICY_PATH = "/vendor/etc/automotive/power_policy.xml";

    private static final String NAMESPACE = null;
    private static final Set<String> VALID_VERSIONS = new ArraySet<>(Arrays.asList("1.0"));
    private static final String TAG_POWER_POLICY = "powerPolicy";
    private static final String TAG_POLICY_GROUPS = "policyGroups";
    private static final String TAG_POLICY_GROUP = "policyGroup";
    private static final String TAG_DEFAULT_POLICY = "defaultPolicy";
    private static final String TAG_NO_DEFAULT_POLICY = "noDefaultPolicy";
    private static final String TAG_POLICIES = "policies";
    private static final String TAG_POLICY = "policy";
    private static final String TAG_OTHER_COMPONENTS = "otherComponents";
    private static final String TAG_COMPONENT = "component";
    private static final String TAG_SYSTEM_POLICY_OVERRIDES = "systemPolicyOverrides";
    private static final String ATTR_VERSION = "version";
    private static final String ATTR_ID = "id";
    private static final String ATTR_STATE = "state";
    private static final String ATTR_BEHAVIOR = "behavior";
    private static final String POWER_ONOFF_ON = "on";
    private static final String POWER_ONOFF_OFF = "off";
    private static final String POWER_ONOFF_UNTOUCHED = "untouched";

    private static final int[] ALL_COMPONENTS;
    private static final int[] NO_COMPONENTS = new int[0];
    private static final int[] INITIAL_ON_COMPONENTS = {
            PowerComponent.AUDIO, PowerComponent.DISPLAY, PowerComponent.CPU
    };
    private static final int[] NO_USER_INTERACTION_ENABLED_COMPONENTS = {
            PowerComponent.WIFI, PowerComponent.CELLULAR,
            PowerComponent.ETHERNET, PowerComponent.TRUSTED_DEVICE_DETECTION, PowerComponent.CPU
    };
    private static final int[] NO_USER_INTERACTION_DISABLED_COMPONENTS = {
            PowerComponent.AUDIO, PowerComponent.MEDIA, PowerComponent.DISPLAY,
            PowerComponent.BLUETOOTH, PowerComponent.PROJECTION, PowerComponent.NFC,
            PowerComponent.INPUT, PowerComponent.VOICE_INTERACTION,
            PowerComponent.VISUAL_INTERACTION, PowerComponent.LOCATION, PowerComponent.MICROPHONE
    };
    private static final Set<Integer> SYSTEM_POLICY_CONFIGURABLE_COMPONENTS =
            new ArraySet<>(Arrays.asList(PowerComponent.BLUETOOTH, PowerComponent.NFC,
            PowerComponent.TRUSTED_DEVICE_DETECTION));
    private static final int[] SUSPEND_TO_RAM_DISABLED_COMPONENTS = {
            PowerComponent.AUDIO, PowerComponent.BLUETOOTH, PowerComponent.WIFI,
            PowerComponent.LOCATION
    };
    private static final CarPowerPolicy POWER_POLICY_ALL_ON;
    private static final CarPowerPolicy POWER_POLICY_INITIAL_ON;
    private static final CarPowerPolicy POWER_POLICY_SUSPEND_TO_RAM;

    static {
        int allCount = LAST_POWER_COMPONENT - FIRST_POWER_COMPONENT + 1;
        ALL_COMPONENTS = new int[allCount];
        int[] initialOnDisabledComponents = new int[allCount - INITIAL_ON_COMPONENTS.length];
        int pos = 0;
        for (int c = FIRST_POWER_COMPONENT; c <= LAST_POWER_COMPONENT; c++) {
            ALL_COMPONENTS[c - FIRST_POWER_COMPONENT] = c;
            if (!containsComponent(INITIAL_ON_COMPONENTS, c)) {
                initialOnDisabledComponents[pos++] = c;
            }
        }

        POWER_POLICY_ALL_ON = new CarPowerPolicy(POWER_POLICY_ID_ALL_ON, ALL_COMPONENTS.clone(),
                NO_COMPONENTS.clone());
        POWER_POLICY_INITIAL_ON = new CarPowerPolicy(POWER_POLICY_ID_INITIAL_ON,
                INITIAL_ON_COMPONENTS.clone(), initialOnDisabledComponents);
        POWER_POLICY_SUSPEND_TO_RAM = new CarPowerPolicy(POWER_POLICY_ID_SUSPEND_TO_RAM,
                NO_COMPONENTS.clone(), SUSPEND_TO_RAM_DISABLED_COMPONENTS.clone());
    }

    private ArrayMap<String, CarPowerPolicy> mRegisteredPowerPolicies;
    private ArrayMap<String, SparseArray<String>> mPolicyGroups;
    private ArrayMap<String, CarPowerPolicy> mPreemptivePowerPolicies;

    /**
     * Gets {@code CarPowerPolicy} corresponding to the given policy ID.
     */
    @Nullable
    CarPowerPolicy getPowerPolicy(String policyId) {
        return mRegisteredPowerPolicies.get(policyId);
    }

    /**
     * Gets {@code CarPowerPolicy} corresponding to the given power state in the given power
     * policy group.
     */
    @Nullable
    CarPowerPolicy getDefaultPowerPolicyForState(String groupId, int state) {
        SparseArray<String> group = mPolicyGroups.get(groupId);
        if (group == null) {
            return null;
        }
        String policyId = group.get(state);
        if (policyId == null) {
            return null;
        }
        return mRegisteredPowerPolicies.get(policyId);
    }

    /**
     * Gets the preemptive power policy corresponding to the given policy ID.
     *
     * <p> When a preemptive power policy is the current power policy, applying a regular power
     * policy is deferred until the preemptive power policy is released.
     */
    @Nullable
    CarPowerPolicy getPreemptivePowerPolicy(String policyId) {
        return mPreemptivePowerPolicies.get(policyId);
    }

    boolean isPowerPolicyGroupAvailable(String groupId) {
        return mPolicyGroups.containsKey(groupId);
    }

    boolean isPreemptivePowerPolicy(String policyId) {
        return mPreemptivePowerPolicies.containsKey(policyId);
    }

    void init() {
        initPolicies();
        readPowerPolicyConfiguration();
    }

    /**
     * Creates and registers a new power policy.
     *
     * @return {@code PolicyOperationStatus.OK}, if successful. Otherwise, the other values.
     */
    @PolicyOperationStatus.ErrorCode
    int definePowerPolicy(String policyId, String[] enabledComponents,
            String[] disabledComponents) {
        if (policyId == null) {
            int error = PolicyOperationStatus.ERROR_INVALID_POWER_POLICY_ID;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, "policyId cannot be null"));
            return error;
        }
        if (isSystemPowerPolicy(policyId)) {
            int error = PolicyOperationStatus.ERROR_INVALID_POWER_POLICY_ID;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error,
                    "policyId should not start with " + SYSTEM_POWER_POLICY_PREFIX));
            return error;
        }
        if (mRegisteredPowerPolicies.containsKey(policyId)) {
            int error = PolicyOperationStatus.ERROR_DOUBLE_REGISTERED_POWER_POLICY_ID;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, policyId));
            return error;
        }
        SparseBooleanArray components = new SparseBooleanArray();
        int status = parseComponents(enabledComponents, true, components);
        if (status != PolicyOperationStatus.OK) {
            return status;
        }
        status = parseComponents(disabledComponents, false, components);
        if (status != PolicyOperationStatus.OK) {
            return status;
        }
        CarPowerPolicy policy = new CarPowerPolicy(policyId, toIntArray(components, true),
                toIntArray(components, false));
        mRegisteredPowerPolicies.put(policyId, policy);
        return PolicyOperationStatus.OK;
    }

    /**
     * Defines and registers a new power policy group.
     *
     * @return {@code PolicyOperationStatus.OK}, if successful. Otherwise, the other values.
     */
    @PolicyOperationStatus.ErrorCode
    int definePowerPolicyGroup(String policyGroupId, SparseArray<String> defaultPolicyPerState) {
        if (policyGroupId == null) {
            return PolicyOperationStatus.ERROR_INVALID_POWER_POLICY_GROUP_ID;
        }
        if (mPolicyGroups.containsKey(policyGroupId)) {
            int error = PolicyOperationStatus.ERROR_DOUBLE_REGISTERED_POWER_POLICY_GROUP_ID;
            Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, policyGroupId));
            return error;
        }
        for (int i = 0; i < defaultPolicyPerState.size(); i++) {
            int state = defaultPolicyPerState.keyAt(i);
            String policyId = defaultPolicyPerState.valueAt(i);
            if (!mRegisteredPowerPolicies.containsKey(policyId)) {
                int error = PolicyOperationStatus.ERROR_NOT_REGISTERED_POWER_POLICY_ID;
                Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, policyId + " for "
                        + powerStateToString(state)));
                return error;
            }
        }
        mPolicyGroups.put(policyGroupId, defaultPolicyPerState);
        return PolicyOperationStatus.OK;
    }

    void dump(IndentingPrintWriter writer) {
        writer.printf("Registered power policies:%s\n",
                mRegisteredPowerPolicies.size() == 0 ? " none" : "");
        writer.increaseIndent();
        for (Map.Entry<String, CarPowerPolicy> entry : mRegisteredPowerPolicies.entrySet()) {
            writer.println(toString(entry.getValue()));
        }
        writer.decreaseIndent();
        writer.printf("Power policy groups:%s\n", mPolicyGroups.isEmpty() ? " none" : "");
        writer.increaseIndent();
        for (Map.Entry<String, SparseArray<String>> entry : mPolicyGroups.entrySet()) {
            writer.printf("%s\n", entry.getKey());
            writer.increaseIndent();
            SparseArray<String> group = entry.getValue();
            for (int i = 0; i < group.size(); i++) {
                writer.printf("- %s --> %s\n", powerStateToString(group.keyAt(i)),
                        group.valueAt(i));
            }
            writer.decreaseIndent();
        }
        writer.decreaseIndent();
        writer.println("Preemptive power policy:");
        writer.increaseIndent();
        for (int i = 0; i < mPreemptivePowerPolicies.size(); i++) {
            writer.println(toString(mPreemptivePowerPolicies.valueAt(i)));
        }
        writer.decreaseIndent();
    }

    @VisibleForTesting
    void initPolicies() {
        mRegisteredPowerPolicies = new ArrayMap<>();
        mRegisteredPowerPolicies.put(POWER_POLICY_ID_ALL_ON, POWER_POLICY_ALL_ON);
        mRegisteredPowerPolicies.put(POWER_POLICY_ID_INITIAL_ON, POWER_POLICY_INITIAL_ON);

        mPolicyGroups = new ArrayMap<>();

        mPreemptivePowerPolicies = new ArrayMap<>();
        mPreemptivePowerPolicies.put(POWER_POLICY_ID_NO_USER_INTERACTION,
                new CarPowerPolicy(POWER_POLICY_ID_NO_USER_INTERACTION,
                        NO_USER_INTERACTION_ENABLED_COMPONENTS.clone(),
                        NO_USER_INTERACTION_DISABLED_COMPONENTS.clone()));
        mPreemptivePowerPolicies.put(POWER_POLICY_ID_SUSPEND_TO_RAM, POWER_POLICY_SUSPEND_TO_RAM);
    }

    private void readPowerPolicyConfiguration() {
        try (InputStream inputStream = new FileInputStream(VENDOR_POLICY_PATH)) {
            readPowerPolicyFromXml(inputStream);
        } catch (IOException | XmlPullParserException | PolicyXmlException e) {
            Slogf.w(TAG, "Proceed without registered policies: failed to parse %s: %s",
                    VENDOR_POLICY_PATH, e);
        }
    }

    @VisibleForTesting
    void readPowerPolicyFromXml(InputStream stream) throws PolicyXmlException,
            XmlPullParserException, IOException {
        XmlPullParser parser = Xml.newPullParser();
        parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, NAMESPACE != null);
        parser.setInput(stream, null);

        // Ensure <powerPolicy> is the root
        parser.nextTag();
        parser.require(START_TAG, NAMESPACE, TAG_POWER_POLICY);
        // Check version
        String version = parser.getAttributeValue(NAMESPACE, ATTR_VERSION);
        if (!VALID_VERSIONS.contains(version)) {
            throw new PolicyXmlException("invalid XML version: " + version);
        }

        ArrayMap<String, CarPowerPolicy> registeredPolicies = new ArrayMap<>();
        ArrayMap<String, SparseArray<String>> policyGroups = new ArrayMap<>();
        CarPowerPolicy systemPolicyOverride = null;

        int type;
        while ((type = parser.next()) != END_DOCUMENT && type != END_TAG) {
            if (type != START_TAG) continue;
            switch (parser.getName()) {
                case TAG_POLICIES:
                    registeredPolicies = parsePolicies(parser, true);
                    break;
                case TAG_POLICY_GROUPS:
                    policyGroups = parsePolicyGroups(parser);
                    break;
                case TAG_SYSTEM_POLICY_OVERRIDES:
                    systemPolicyOverride = parseSystemPolicyOverrides(parser);
                    break;
                default:
                    throw new PolicyXmlException("unknown tag: " + parser.getName() + " under "
                            + TAG_POWER_POLICY);
            }
        }
        validatePolicyGroups(policyGroups, registeredPolicies);

        mRegisteredPowerPolicies = registeredPolicies;
        mPolicyGroups = policyGroups;
        reconstructSystemPowerPolicy(systemPolicyOverride);
    }

    private ArrayMap<String, CarPowerPolicy> parsePolicies(XmlPullParser parser,
            boolean includeOtherComponents) throws PolicyXmlException, XmlPullParserException,
            IOException {
        ArrayMap<String, CarPowerPolicy> policies = new ArrayMap<>();
        int type;
        while ((type = parser.next()) != END_DOCUMENT && type != END_TAG) {
            if (type != START_TAG) continue;
            if (TAG_POLICY.equals(parser.getName())) {
                String policyId = parser.getAttributeValue(NAMESPACE, ATTR_ID);
                if (policyId == null || policyId.equals("")) {
                    throw new PolicyXmlException("no |" + ATTR_ID + "| attribute of |" + TAG_POLICY
                            + "| tag");
                }
                if (includeOtherComponents && isSystemPowerPolicy(policyId)) {
                    throw new PolicyXmlException("Policy ID should not start with "
                            + SYSTEM_POWER_POLICY_PREFIX);
                }
                policies.put(policyId, parsePolicy(parser, policyId, includeOtherComponents));
            } else {
                throw new PolicyXmlException("unknown tag: " + parser.getName() + " under "
                        + TAG_POLICIES);
            }
        }
        return policies;
    }

    private ArrayMap<String, SparseArray<String>> parsePolicyGroups(XmlPullParser parser) throws
            PolicyXmlException, XmlPullParserException, IOException {
        ArrayMap<String, SparseArray<String>> policyGroups = new ArrayMap<>();
        int type;
        while ((type = parser.next()) != END_DOCUMENT && type != END_TAG) {
            if (type != START_TAG) continue;
            if (TAG_POLICY_GROUP.equals(parser.getName())) {
                String groupId = parser.getAttributeValue(NAMESPACE, ATTR_ID);
                if (groupId == null || groupId.equals("")) {
                    throw new PolicyXmlException("no |" + ATTR_ID + "| attribute of |"
                            + TAG_POLICY_GROUP + "| tag");
                }
                policyGroups.put(groupId, parsePolicyGroup(parser));
            } else {
                throw new PolicyXmlException("unknown tag: " + parser.getName() + " under "
                        + TAG_POLICY_GROUPS);
            }
        }
        return policyGroups;
    }

    @Nullable
    private CarPowerPolicy parseSystemPolicyOverrides(XmlPullParser parser) throws
            PolicyXmlException, XmlPullParserException, IOException {
        ArrayMap<String, CarPowerPolicy> systemOverrides = parsePolicies(parser, false);
        int numOverrides = systemOverrides.size();
        if (numOverrides == 0) {
            return null;
        }
        if (numOverrides > 1) {
            throw new PolicyXmlException("only one system power policy is supported: "
                    + numOverrides + " system policies exist");
        }
        CarPowerPolicy policyOverride =
                systemOverrides.get(POWER_POLICY_ID_NO_USER_INTERACTION);
        if (policyOverride == null) {
            throw new PolicyXmlException("system power policy id should be "
                    + POWER_POLICY_ID_NO_USER_INTERACTION);
        }
        Set<Integer> visited = new ArraySet<>();
        checkSystemPowerPolicyComponents(policyOverride.getEnabledComponents(), visited);
        checkSystemPowerPolicyComponents(policyOverride.getDisabledComponents(), visited);
        return policyOverride;
    }

    private CarPowerPolicy parsePolicy(XmlPullParser parser, String policyId,
            boolean includeOtherComponents) throws PolicyXmlException, XmlPullParserException,
            IOException {
        SparseBooleanArray components = new SparseBooleanArray();
        String behavior = POWER_ONOFF_UNTOUCHED;
        boolean otherComponentsProcessed = false;
        int type;
        while ((type = parser.next()) != END_DOCUMENT && type != END_TAG) {
            if (type != START_TAG) continue;
            if (TAG_COMPONENT.equals(parser.getName())) {
                String id = parser.getAttributeValue(NAMESPACE, ATTR_ID);
                int powerComponent = toPowerComponent(id, true);
                if (powerComponent == INVALID_POWER_COMPONENT) {
                    throw new PolicyXmlException("invalid value(" + id + ") in |" + ATTR_ID
                            + "| attribute of |" + TAG_COMPONENT + "| tag");
                }
                if (components.indexOfKey(powerComponent) >= 0) {
                    throw new PolicyXmlException(id + " is specified more than once in |"
                            + TAG_COMPONENT + "| tag");
                }
                String state = getText(parser);
                switch (state) {
                    case POWER_ONOFF_ON:
                        components.put(powerComponent, true);
                        break;
                    case POWER_ONOFF_OFF:
                        components.put(powerComponent, false);
                        break;
                    default:
                        throw new PolicyXmlException("target state(" + state + ") for " + id
                                + " is not valid");
                }
                skip(parser);
            } else if (TAG_OTHER_COMPONENTS.equals(parser.getName())) {
                if (!includeOtherComponents) {
                    throw new PolicyXmlException("|" + TAG_OTHER_COMPONENTS
                            + "| tag is not expected");
                }
                if (otherComponentsProcessed) {
                    throw new PolicyXmlException("more than one |" + TAG_OTHER_COMPONENTS
                            + "| tag");
                }
                otherComponentsProcessed = true;
                behavior = parser.getAttributeValue(NAMESPACE, ATTR_BEHAVIOR);
                if (behavior == null) {
                    throw new PolicyXmlException("no |" + ATTR_BEHAVIOR + "| attribute of |"
                            + TAG_OTHER_COMPONENTS + "| tag");
                }
                switch (behavior) {
                    case POWER_ONOFF_ON:
                    case POWER_ONOFF_OFF:
                    case POWER_ONOFF_UNTOUCHED:
                        break;
                    default:
                        throw new PolicyXmlException("invalid value(" + behavior + ") in |"
                                + ATTR_BEHAVIOR + "| attribute of |" + TAG_OTHER_COMPONENTS
                                + "| tag");
                }
                skip(parser);
            } else {
                throw new PolicyXmlException("unknown tag: " + parser.getName() + " under "
                        + TAG_POLICY);
            }
        }
        boolean enabled = false;
        boolean untouched = false;
        if (POWER_ONOFF_ON.equals(behavior)) {
            enabled = true;
        } else if (POWER_ONOFF_OFF.equals(behavior)) {
            enabled = false;
        } else {
            untouched = true;
        }
        if (!untouched) {
            for (int component = FIRST_POWER_COMPONENT;
                    component <= LAST_POWER_COMPONENT; component++) {
                if (components.indexOfKey(component) >= 0) continue;
                components.put(component, enabled);
            }
        }
        return new CarPowerPolicy(policyId, toIntArray(components, true),
                toIntArray(components, false));
    }

    private SparseArray<String> parsePolicyGroup(XmlPullParser parser) throws PolicyXmlException,
            XmlPullParserException, IOException {
        SparseArray<String> policyGroup = new SparseArray<>();
        int type;
        Set<Integer> visited = new ArraySet<>();
        while ((type = parser.next()) != END_DOCUMENT && type != END_TAG) {
            if (type != START_TAG) continue;
            if (TAG_DEFAULT_POLICY.equals(parser.getName())) {
                String id = parser.getAttributeValue(NAMESPACE, ATTR_ID);
                if (id == null || id.isEmpty()) {
                    throw new PolicyXmlException("no |" + ATTR_ID + "| attribute of |"
                            + TAG_DEFAULT_POLICY + "| tag");
                }
                String state = parser.getAttributeValue(NAMESPACE, ATTR_STATE);
                int powerState = toPowerState(state);
                if (powerState == INVALID_POWER_STATE) {
                    throw new PolicyXmlException("invalid value(" + state + ") in |" + ATTR_STATE
                            + "| attribute of |" + TAG_DEFAULT_POLICY + "| tag");
                }
                if (visited.contains(powerState)) {
                    throw new PolicyXmlException("power state(" + state
                            + ") is specified more than once");
                }
                policyGroup.put(powerState, id);
                visited.add(powerState);
                skip(parser);
            } else if (TAG_NO_DEFAULT_POLICY.equals(parser.getName())) {
                String state = parser.getAttributeValue(NAMESPACE, ATTR_STATE);
                int powerState = toPowerState(state);
                if (powerState == INVALID_POWER_STATE) {
                    throw new PolicyXmlException("invalid value(" + state + ") in |" + ATTR_STATE
                            + "| attribute of |" + TAG_DEFAULT_POLICY + "| tag");
                }
                if (visited.contains(powerState)) {
                    throw new PolicyXmlException("power state(" + state
                            + ") is specified more than once");
                }
                visited.add(powerState);
                skip(parser);
            } else {
                throw new PolicyXmlException("unknown tag: " + parser.getName() + " under "
                        + TAG_POLICY_GROUP);
            }
        }
        return policyGroup;
    }

    private void validatePolicyGroups(ArrayMap<String, SparseArray<String>> policyGroups,
            ArrayMap<String, CarPowerPolicy> registeredPolicies) throws PolicyXmlException {
        for (Map.Entry<String, SparseArray<String>> entry : policyGroups.entrySet()) {
            SparseArray<String> group = entry.getValue();
            for (int i = 0; i < group.size(); i++) {
                String policyId = group.valueAt(i);
                if (!registeredPolicies.containsKey(group.valueAt(i))) {
                    throw new PolicyXmlException("group(id: " + entry.getKey()
                            + ") contains invalid policy(id: " + policyId + ")");
                }
            }
        }
    }

    private void reconstructSystemPowerPolicy(@Nullable CarPowerPolicy policyOverride) {
        if (policyOverride == null) return;

        List<Integer> enabledComponents = Arrays.stream(NO_USER_INTERACTION_ENABLED_COMPONENTS)
                .boxed().collect(Collectors.toList());
        List<Integer> disabledComponents = Arrays.stream(NO_USER_INTERACTION_DISABLED_COMPONENTS)
                .boxed().collect(Collectors.toList());
        int[] overrideEnabledComponents = policyOverride.getEnabledComponents();
        int[] overrideDisabledComponents = policyOverride.getDisabledComponents();
        for (int i = 0; i < overrideEnabledComponents.length; i++) {
            removeComponent(disabledComponents, overrideEnabledComponents[i]);
            addComponent(enabledComponents, overrideEnabledComponents[i]);
        }
        for (int i = 0; i < overrideDisabledComponents.length; i++) {
            removeComponent(enabledComponents, overrideDisabledComponents[i]);
            addComponent(disabledComponents, overrideDisabledComponents[i]);
        }
        mPreemptivePowerPolicies.put(POWER_POLICY_ID_NO_USER_INTERACTION,
                new CarPowerPolicy(POWER_POLICY_ID_NO_USER_INTERACTION,
                        CarServiceUtils.toIntArray(enabledComponents),
                        CarServiceUtils.toIntArray(disabledComponents)));
    }

    private void removeComponent(List<Integer> components, int component) {
        int index = components.lastIndexOf(component);
        if (index != -1) {
            components.remove(index);
        }
    }

    private void addComponent(List<Integer> components, int component) {
        int index = components.lastIndexOf(component);
        if (index == -1) {
            components.add(component);
        }
    }

    private String getText(XmlPullParser parser) throws PolicyXmlException, XmlPullParserException,
            IOException {
        if (parser.getEventType() != START_TAG) {
            throw new PolicyXmlException("tag pair doesn't match");
        }
        parser.next();
        if (parser.getEventType() != TEXT) {
            throw new PolicyXmlException("tag value is not found");
        }
        return parser.getText();
    }

    private void skip(XmlPullParser parser) throws PolicyXmlException, XmlPullParserException,
            IOException {
        int type = parser.getEventType();
        if (type != START_TAG && type != TEXT) {
            throw new PolicyXmlException("tag pair doesn't match");
        }
        int depth = 1;
        while (depth != 0) {
            switch (parser.next()) {
                case END_TAG:
                    depth--;
                    break;
                case START_TAG:
                    depth++;
                    break;
            }
        }
    }

    void checkSystemPowerPolicyComponents(int[] components, Set<Integer> visited) throws
            PolicyXmlException {
        for (int i = 0; i < components.length; i++) {
            int component = components[i];
            if (!isOverridableComponent(component)) {
                throw new PolicyXmlException("Power component(" + powerComponentToString(component)
                        + ") cannot be overridden");
            }
            if (visited.contains(component)) {
                throw new PolicyXmlException("Power component(" + powerComponentToString(component)
                        + ") is specified more than once");
            }
            visited.add(component);
        }
    }

    boolean isOverridableComponent(int component) {
        return SYSTEM_POLICY_CONFIGURABLE_COMPONENTS.contains(component);
    }

    private String toString(CarPowerPolicy policy) {
        return policy.getPolicyId() + "(enabledComponents: "
                + componentsToString(policy.getEnabledComponents()) + " | disabledComponents: "
                + componentsToString(policy.getDisabledComponents()) + ")";
    }

    private String componentsToString(int[] components) {
        StringBuffer buffer = new StringBuffer();
        for (int i = 0; i < components.length; i++) {
            if (i > 0) buffer.append(", ");
            buffer.append(powerComponentToString(components[i]));
        }
        return buffer.toString();
    }

    @PolicyOperationStatus.ErrorCode
    int parseComponents(String[] componentArr, boolean enabled, SparseBooleanArray components) {
        for (int i = 0; i < componentArr.length; i++) {
            int component = toPowerComponent(componentArr[i], false);
            if (component == INVALID_POWER_COMPONENT) {
                int error = PolicyOperationStatus.ERROR_INVALID_POWER_COMPONENT;
                Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, componentArr[i]));
                return error;
            }
            if (components.indexOfKey(component) >= 0) {
                int error = PolicyOperationStatus.ERROR_DUPLICATED_POWER_COMPONENT;
                Slogf.w(TAG, PolicyOperationStatus.errorCodeToString(error, componentArr[i]));
                return error;
            }
            components.put(component, enabled);
        }
        return PolicyOperationStatus.OK;
    }

    static int toPowerState(String state) {
        if (state == null) {
            return INVALID_POWER_STATE;
        }
        switch (state) {
            case POWER_STATE_WAIT_FOR_VHAL:
                return VehicleApPowerStateReport.WAIT_FOR_VHAL;
            case POWER_STATE_ON:
                return VehicleApPowerStateReport.ON;
            default:
                return INVALID_POWER_STATE;
        }
    }

    static String powerStateToString(int state) {
        switch (state) {
            case VehicleApPowerStateReport.WAIT_FOR_VHAL:
                return POWER_STATE_WAIT_FOR_VHAL;
            case VehicleApPowerStateReport.ON:
                return POWER_STATE_ON;
            default:
                return "unknown power state";
        }
    }

    static boolean isSystemPowerPolicy(String policyId) {
        return policyId == null ? false : policyId.startsWith(SYSTEM_POWER_POLICY_PREFIX);
    }

    private static int[] toIntArray(SparseBooleanArray array, boolean value) {
        int arraySize = array.size();
        int returnSize = 0;
        for (int i = 0; i < arraySize; i++) {
            if (array.valueAt(i) == value) returnSize++;
        }
        int[] ret = new int[returnSize];
        int count = 0;
        for (int i = 0; i < arraySize; i++) {
            if (array.valueAt(i) == value) {
                ret[count++] = array.keyAt(i);
            }
        }
        return ret;
    }

    private static boolean containsComponent(int[] arr, int component) {
        for (int element : arr) {
            if (element == component) return true;
        }
        return false;
    }

    @VisibleForTesting
    static final class PolicyXmlException extends Exception {
        PolicyXmlException(String message) {
            super(message);
        }
    }
}
