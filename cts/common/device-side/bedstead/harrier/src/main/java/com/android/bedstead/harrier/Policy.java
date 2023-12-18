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

package com.android.bedstead.harrier;

import com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy;
import com.android.bedstead.harrier.annotations.parameterized.IncludeNone;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnAffiliatedDeviceOwnerSecondaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnAffiliatedProfileOwnerSecondaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnDeviceOwnerUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnParentOfCorporateOwnedProfileOwner;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnParentOfProfileOwner;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnProfileOwnerPrimaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnProfileOwnerProfile;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner;

import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.List;

/**
 * Utility class for enterprise policy tests.
 */
@IncludeNone
@IncludeRunOnDeviceOwnerUser
@IncludeRunOnAffiliatedDeviceOwnerSecondaryUser
@IncludeRunOnAffiliatedProfileOwnerSecondaryUser
@IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser
@IncludeRunOnProfileOwnerProfile
@IncludeRunOnParentOfCorporateOwnedProfileOwner
@IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner
@IncludeRunOnParentOfProfileOwner
@IncludeRunOnProfileOwnerPrimaryUser
public final class Policy {

    private Policy() {

    }

    private static final IncludeNone INCLUDE_NONE_ANNOTATION =
            Policy.class.getAnnotation(IncludeNone.class);
    private static final IncludeRunOnDeviceOwnerUser INCLUDE_RUN_ON_DEVICE_OWNER_USER =
            Policy.class.getAnnotation(IncludeRunOnDeviceOwnerUser.class);
    private static final IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser
            INCLUDE_RUN_ON_NON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER =
            Policy.class.getAnnotation(IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser.class);
    private static final IncludeRunOnAffiliatedDeviceOwnerSecondaryUser
            INCLUDE_RUN_ON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER =
            Policy.class.getAnnotation(IncludeRunOnAffiliatedDeviceOwnerSecondaryUser.class);
    private static final IncludeRunOnAffiliatedProfileOwnerSecondaryUser
            INCLUDE_RUN_ON_AFFILIATED_PROFILE_OWNER_SECONDARY_USER =
            Policy.class.getAnnotation(IncludeRunOnAffiliatedProfileOwnerSecondaryUser.class);
    private static final IncludeRunOnProfileOwnerProfile
            INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE =
            Policy.class.getAnnotation(IncludeRunOnProfileOwnerProfile.class);
    private static final IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner
            INCLUDE_RUN_ON_SECONDARY_USER_IN_DIFFERENT_PROFILE_GROUP_TO_PROFILE_OWNER =
            Policy.class.getAnnotation(
                    IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner.class);
    private static final IncludeRunOnParentOfProfileOwner INCLUDE_RUN_ON_PARENT_OF_PROFILE_OWNER =
            Policy.class.getAnnotation(IncludeRunOnParentOfProfileOwner.class);
    private static final IncludeRunOnParentOfCorporateOwnedProfileOwner
            INCLUDE_RUN_ON_PARENT_OF_CORPORATE_OWNED_PROFILE_OWNER =
            Policy.class.getAnnotation(IncludeRunOnParentOfCorporateOwnedProfileOwner.class);
    private static final IncludeRunOnProfileOwnerPrimaryUser INCLUDE_RUN_ON_PROFILE_OWNER_PRIMARY_USER =
            Policy.class.getAnnotation(IncludeRunOnProfileOwnerPrimaryUser.class);

    /**
     * Get positive state annotations for the given policy.
     *
     * <p>These are states which should be run where the policy is able to be applied.
     */
    public static List<Annotation> positiveStates(EnterprisePolicy enterprisePolicy) {
        List<Annotation> annotations = new ArrayList<>();

        switch (enterprisePolicy.deviceOwner()) {
            case NO:
                break;
            case GLOBAL:
                annotations.add(INCLUDE_RUN_ON_DEVICE_OWNER_USER);
                annotations.add(INCLUDE_RUN_ON_NON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER);
                break;
            case AFFILIATED:
                annotations.add(INCLUDE_RUN_ON_DEVICE_OWNER_USER);
                annotations.add(INCLUDE_RUN_ON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER);
                break;
            case USER:
                annotations.add(INCLUDE_RUN_ON_DEVICE_OWNER_USER);
                break;
            default:
                throw new IllegalStateException(
                        "Unknown policy control: " + enterprisePolicy.deviceOwner());
        }

        switch (enterprisePolicy.profileOwner()) {
            case NO:
                break;
            case AFFILIATED:
                annotations.add(INCLUDE_RUN_ON_AFFILIATED_PROFILE_OWNER_SECONDARY_USER);
                break;
            case AFFILIATED_OR_NO_DO:
                annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PRIMARY_USER);
                annotations.add(INCLUDE_RUN_ON_AFFILIATED_PROFILE_OWNER_SECONDARY_USER);
                break;
            case PARENT:
                annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
                annotations.add(INCLUDE_RUN_ON_PARENT_OF_PROFILE_OWNER);
                break;
            case COPE_PARENT:
                annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
                //                TODO(scottjonathan): Re-add when we can setup this state
//                annotations.add(INCLUDE_RUN_ON_PARENT_OF_CORPORATE_OWNED_PROFILE_OWNER);
                break;
            case PROFILE:
                annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
                break;
            default:
                throw new IllegalStateException(
                        "Unknown policy control: " + enterprisePolicy.profileOwner());
        }

        if (annotations.isEmpty()) {
            // Don't run the original test unparameterized
            annotations.add(INCLUDE_NONE_ANNOTATION);
        }

        return annotations;
    }

    /**
     * Get negative state annotations for the given policy.
     *
     * <p>These are states which should be run where the policy is not able to be applied.
     */
    public static List<Annotation> negativeStates(EnterprisePolicy enterprisePolicy) {
        List<Annotation> annotations = new ArrayList<>();

        switch (enterprisePolicy.deviceOwner()) {
            case NO:
                break;
            case GLOBAL:
                break;
            case AFFILIATED:
                annotations.add(INCLUDE_RUN_ON_NON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER);
                break;
            case USER:
                annotations.add(INCLUDE_RUN_ON_AFFILIATED_DEVICE_OWNER_SECONDARY_USER);
                break;
            default:
                throw new IllegalStateException(
                        "Unknown policy control: " + enterprisePolicy.deviceOwner());
        }

        switch (enterprisePolicy.profileOwner()) {
            case NO:
                break;
            case AFFILIATED:
                // TODO(scottjonathan): Define negative states
                break;
            case AFFILIATED_OR_NO_DO:
                // TODO(scottjonathan): Define negative states
                break;
            case PARENT:
                annotations.add(
                        INCLUDE_RUN_ON_SECONDARY_USER_IN_DIFFERENT_PROFILE_GROUP_TO_PROFILE_OWNER);
                break;
            case COPE_PARENT:
                annotations.add(
                        INCLUDE_RUN_ON_SECONDARY_USER_IN_DIFFERENT_PROFILE_GROUP_TO_PROFILE_OWNER);
                annotations.add(
                        INCLUDE_RUN_ON_PARENT_OF_PROFILE_OWNER);
                break;
            case PROFILE:
                annotations.add(
                        INCLUDE_RUN_ON_SECONDARY_USER_IN_DIFFERENT_PROFILE_GROUP_TO_PROFILE_OWNER);
                //                TODO(scottjonathan): Re-add when we can setup this state
//                annotations.add(
//                        INCLUDE_RUN_ON_PARENT_OF_CORPORATE_OWNED_PROFILE_OWNER);
                break;
            default:
                throw new IllegalStateException(
                        "Unknown policy control: " + enterprisePolicy.profileOwner());
        }

        if (annotations.isEmpty()) {
            // Don't run the original test unparameterized
            annotations.add(INCLUDE_NONE_ANNOTATION);
        }

        return annotations;
    }

    /**
     * Get state annotations where the policy cannot be set for the given policy.
     */
    public static List<Annotation> cannotSetPolicyStates(EnterprisePolicy enterprisePolicy) {
        List<Annotation> annotations = new ArrayList<>();

        // TODO(scottjonathan): Always include a state without a dpc

        if (enterprisePolicy.deviceOwner() == EnterprisePolicy.DeviceOwnerControl.NO) {
            annotations.add(INCLUDE_RUN_ON_DEVICE_OWNER_USER);
        }

        if (enterprisePolicy.profileOwner() == EnterprisePolicy.ProfileOwnerControl.NO) {
            annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
        } else if (enterprisePolicy.profileOwner() == EnterprisePolicy.ProfileOwnerControl.AFFILIATED) {
            annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
        } else if (enterprisePolicy.profileOwner() == EnterprisePolicy.ProfileOwnerControl.AFFILIATED_OR_NO_DO) {
            annotations.add(INCLUDE_RUN_ON_PROFILE_OWNER_PROFILE);
        }

        if (annotations.isEmpty()) {
            // Don't run the original test unparameterized
            annotations.add(INCLUDE_NONE_ANNOTATION);
        }

        return annotations;
    }
}
