/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.car.cts.powerpolicy;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Objects;

public final class PowerPolicyGroups {
    private final HashMap<String, PowerPolicyGroupDef> mPolicyGroups = new HashMap<>();

    public PowerPolicyGroups() { }

    public PowerPolicyGroups(PowerPolicyGroupDef[] defs) {
        for (int i = 0; i < defs.length; i++) {
            mPolicyGroups.put(defs[i].mGroupId, defs[i]);
        }
    }

    public void add(String id, String waitForVHALPolicy, String onPolicy) throws Exception {
        if (mPolicyGroups.containsKey(id)) {
            throw new IllegalArgumentException(id + " policy group already exists");
        }
        PowerPolicyGroupDef groupDef = new PowerPolicyGroupDef(id, waitForVHALPolicy, onPolicy);
        mPolicyGroups.put(id, groupDef);
    }

    @Override
    public String toString() {
        StringBuilder str = new StringBuilder();
        str.append("Power policy groups:\n");
        mPolicyGroups.forEach((k, v) -> str.append(v.toString()));
        return str.toString();
    }

    @Override
    public int hashCode() {
        return mPolicyGroups.hashCode();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        PowerPolicyGroups peer = (PowerPolicyGroups) o;
        return mPolicyGroups.equals(peer.mPolicyGroups);
    }

    public static PowerPolicyGroups parse(ArrayList<String> defStrs) throws Exception {
        if ((defStrs.size() % 3) != 0) {
            throw new IllegalArgumentException("each policy group def needs 3 lines of data");
        }

        PowerPolicyGroups policyGroups = new PowerPolicyGroups();
        for (int i = 0; i < defStrs.size(); i += 3) {
            String groupId = defStrs.get(i).trim();
            String waitForVHALPolicy = parsePolicyGroupDef("WaitForVHAL", defStrs.get(i + 1));
            String onPolicy = parsePolicyGroupDef("On", defStrs.get(i + 2));
            policyGroups.add(groupId, waitForVHALPolicy, onPolicy);
        }
        return policyGroups;
    }

    private static String parsePolicyGroupDef(String stateName, String defStr) throws Exception {
        String[] tokens = defStr.trim().split("(\\s*)(-{1,2})(>?)(\\s*)");
        if (tokens.length != 3) {
            throw new IllegalArgumentException("malformatted policy group def str: " + defStr);
        }

        if (!stateName.equals(tokens[1].trim())) {
            String errMsg = String.format("expected power state: %s but got: %s",
                    stateName, tokens[1]);
            throw new IllegalArgumentException(errMsg);
        }

        return tokens[2].trim();
    }

    public static final class PowerPolicyGroupDef {
        private final String mGroupId;
        private final String mWaitForVHALStatePolicy;
        private final String mOnStatePolicy;

        private PowerPolicyGroupDef(String groupId, String waitForVHALPolicy, String onPolicy) {
            mGroupId = groupId;
            mWaitForVHALStatePolicy = waitForVHALPolicy;
            mOnStatePolicy = onPolicy;
        }

        public String getGroupId() {
            return mGroupId;
        }

        public String getWaitForVHALStatePolicy() {
            return mWaitForVHALStatePolicy;
        }

        public String getOnStatePolicy() {
            return mOnStatePolicy;
        }

        public String toShellCommandString() {
            return String.format("%s WaitForVHAL:%s On:%s", mGroupId,
                    mWaitForVHALStatePolicy, mOnStatePolicy);
        }

        @Override
        public String toString() {
            StringBuilder str = new StringBuilder();
            str.append("  ").append(mGroupId).append('\n');
            str.append("    - WaitForVHAL --> ").append(mWaitForVHALStatePolicy).append('\n');
            str.append("    - On --> ").append(mOnStatePolicy).append('\n');
            return str.toString();
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            PowerPolicyGroupDef that = (PowerPolicyGroupDef) o;
            return Objects.equals(mGroupId, that.mGroupId)
                    && Objects.equals(mWaitForVHALStatePolicy, that.mWaitForVHALStatePolicy)
                    && Objects.equals(mOnStatePolicy, that.mOnStatePolicy);
        }

        @Override
        public int hashCode() {
            return Objects.hash(mGroupId, mWaitForVHALStatePolicy, mOnStatePolicy);
        }
    }

    public static final class TestSet {
        public static final String GROUP_ID1 = "policy_group1";
        public static final String GROUP_ID2 = "policy_group2";

        public static final PowerPolicyGroupDef POLICY_GROUP_DEF1 =
                new PowerPolicyGroupDef(GROUP_ID1, PowerPolicyDef.IdSet.TEST1,
                    PowerPolicyDef.IdSet.TEST2);

        public static final PowerPolicyGroupDef POLICY_GROUP_DEF2 =
                new PowerPolicyGroupDef(GROUP_ID2, PowerPolicyDef.IdSet.TEST2,
                    PowerPolicyDef.IdSet.TEST1);

        public static final PowerPolicyGroups POLICY_GROUPS1 = new PowerPolicyGroups(
                new PowerPolicyGroupDef[]{POLICY_GROUP_DEF1, POLICY_GROUP_DEF2});

        private TestSet() { }
    }
}
