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

package com.android.bedstead.harrier;

import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;

import static com.android.bedstead.nene.permissions.Permissions.NOTIFY_PENDING_SYSTEM_UPDATE;
import static com.android.bedstead.nene.users.UserType.MANAGED_PROFILE_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SECONDARY_USER_TYPE_NAME;
import static com.android.bedstead.nene.utils.Versions.meetsSdkVersionRequirements;
import static com.android.bedstead.remotedpc.Configuration.REMOTE_DPC_COMPONENT_NAME;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertFalse;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.bedstead.harrier.annotations.AfterClass;
import com.android.bedstead.harrier.annotations.BeforeClass;
import com.android.bedstead.harrier.annotations.EnsureDoesNotHavePermission;
import com.android.bedstead.harrier.annotations.EnsureHasPermission;
import com.android.bedstead.harrier.annotations.EnsurePackageNotInstalled;
import com.android.bedstead.harrier.annotations.FailureMode;
import com.android.bedstead.harrier.annotations.RequireDoesNotHaveFeature;
import com.android.bedstead.harrier.annotations.RequireFeature;
import com.android.bedstead.harrier.annotations.RequireGmsInstrumentation;
import com.android.bedstead.harrier.annotations.RequirePackageInstalled;
import com.android.bedstead.harrier.annotations.RequirePackageNotInstalled;
import com.android.bedstead.harrier.annotations.RequireSdkVersion;
import com.android.bedstead.harrier.annotations.RequireUserSupported;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoProfileOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasProfileOwner;
import com.android.bedstead.harrier.annotations.meta.EnsureHasNoProfileAnnotation;
import com.android.bedstead.harrier.annotations.meta.EnsureHasNoUserAnnotation;
import com.android.bedstead.harrier.annotations.meta.EnsureHasProfileAnnotation;
import com.android.bedstead.harrier.annotations.meta.EnsureHasUserAnnotation;
import com.android.bedstead.harrier.annotations.meta.ParameterizedAnnotation;
import com.android.bedstead.harrier.annotations.meta.RequireRunOnProfileAnnotation;
import com.android.bedstead.harrier.annotations.meta.RequireRunOnUserAnnotation;
import com.android.bedstead.harrier.annotations.meta.RequiresBedsteadJUnit4;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.devicepolicy.DeviceOwner;
import com.android.bedstead.nene.devicepolicy.DevicePolicyController;
import com.android.bedstead.nene.devicepolicy.ProfileOwner;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.packages.Package;
import com.android.bedstead.nene.permissions.PermissionContextImpl;
import com.android.bedstead.nene.users.User;
import com.android.bedstead.nene.users.UserBuilder;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.bedstead.nene.utils.Versions;
import com.android.bedstead.remotedpc.RemoteDpc;
import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import com.google.common.base.Objects;

import junit.framework.AssertionFailedError;

import org.junit.AssumptionViolatedException;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.Statement;
import org.junit.runners.model.TestClass;

import java.lang.annotation.Annotation;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;


/**
 * A Junit rule which exposes methods for efficiently changing and querying device state.
 *
 * <p>States set by the methods on this class will by default be cleaned up after the test.
 *
 *
 * <p>Using this rule also enforces preconditions in annotations from the
 * {@code com.android.comaptibility.common.util.enterprise.annotations} package.
 *
 * {@code assumeTrue} will be used, so tests which do not meet preconditions will be skipped.
 */
public final class DeviceState implements TestRule {

    private static final String GMS_PKG = "com.google.android.gms";

    private final Context mContext = ApplicationProvider.getApplicationContext();
    private static final TestApis sTestApis = new TestApis();
    private static final String SKIP_TEST_TEARDOWN_KEY = "skip-test-teardown";
    private static final String SKIP_CLASS_TEARDOWN_KEY = "skip-class-teardown";
    private static final String SKIP_TESTS_REASON_KEY = "skip-tests-reason";
    private boolean mSkipTestTeardown;
    private boolean mSkipClassTeardown;
    private boolean mSkipTests;
    private boolean mFailTests;
    private boolean mUsingBedsteadJUnit4 = false;
    private String mSkipTestsReason;
    private String mFailTestsReason;

    // Marks if the conditions for requiring running under GMS instrumentation have been set
    // if not - we assume the test should never run under GMS instrumentation
    private boolean mHasRequireGmsInstrumentation = false;

    private static final String TV_PROFILE_TYPE_NAME = "com.android.tv.profile";

    public DeviceState() {
        Bundle arguments = InstrumentationRegistry.getArguments();
        mSkipTestTeardown = Boolean.parseBoolean(
                arguments.getString(SKIP_TEST_TEARDOWN_KEY, "false"));
        mSkipClassTeardown = Boolean.parseBoolean(
                arguments.getString(SKIP_CLASS_TEARDOWN_KEY, "false"));
        mSkipTestsReason = arguments.getString(SKIP_TESTS_REASON_KEY, "");
        mSkipTests = !mSkipTestsReason.isEmpty();
    }

    void setSkipTestTeardown(boolean skipTestTeardown) {
        mSkipTestTeardown = skipTestTeardown;
    }

    void setUsingBedsteadJUnit4(boolean usingBedsteadJUnit4) {
        mUsingBedsteadJUnit4 = usingBedsteadJUnit4;
    }

    @Override public Statement apply(final Statement base,
            final Description description) {

        if (description.isTest()) {
            return applyTest(base, description);
        } else if (description.isSuite()) {
            return applySuite(base, description);
        }
        throw new IllegalStateException("Unknown description type: " + description);
    }

    private Statement applyTest(final Statement base, final Description description) {
        return new Statement() {
            @Override public void evaluate() throws Throwable {
                Log.d(LOG_TAG, "Preparing state for test " + description.getMethodName());

                assumeFalse(mSkipTestsReason, mSkipTests);
                assertFalse(mFailTestsReason, mFailTests);

                List<Annotation> annotations = getAnnotations(description);
                PermissionContextImpl permissionContext = applyAnnotations(annotations);

                Log.d(LOG_TAG,
                        "Finished preparing state for test " + description.getMethodName());

                try {
                    base.evaluate();
                } finally {
                    Log.d(LOG_TAG,
                            "Tearing down state for test " + description.getMethodName());

                    if (permissionContext != null) {
                        permissionContext.close();
                    }

                    teardownNonShareableState();
                    if (!mSkipTestTeardown) {
                        teardownShareableState();
                    }
                    Log.d(LOG_TAG,
                            "Finished tearing down state for test "
                                    + description.getMethodName());
                }
            }};
    }

    private PermissionContextImpl applyAnnotations(List<Annotation> annotations)
            throws Throwable {
        PermissionContextImpl permissionContext = null;
        for (Annotation annotation : annotations) {
            Class<? extends Annotation> annotationType = annotation.annotationType();

            EnsureHasNoProfileAnnotation ensureHasNoProfileAnnotation =
                    annotationType.getAnnotation(EnsureHasNoProfileAnnotation.class);
            if (ensureHasNoProfileAnnotation != null) {
                UserType userType = (UserType) annotation.annotationType()
                        .getMethod("forUser").invoke(annotation);
                ensureHasNoProfile(ensureHasNoProfileAnnotation.value(), userType);
                continue;
            }

            EnsureHasProfileAnnotation ensureHasProfileAnnotation =
                    annotationType.getAnnotation(EnsureHasProfileAnnotation.class);
            if (ensureHasProfileAnnotation != null) {
                UserType forUser = (UserType) annotation.annotationType()
                        .getMethod("forUser").invoke(annotation);
                OptionalBoolean installInstrumentedApp = (OptionalBoolean)
                        annotation.annotationType()
                                .getMethod("installInstrumentedApp").invoke(annotation);

                boolean dpcIsPrimary = false;
                if (ensureHasProfileAnnotation.hasProfileOwner()) {
                    dpcIsPrimary = (boolean)
                            annotation.annotationType()
                                    .getMethod("dpcIsPrimary").invoke(annotation);
                }

                ensureHasProfile(
                        ensureHasProfileAnnotation.value(), installInstrumentedApp,
                        forUser, ensureHasProfileAnnotation.hasProfileOwner(),
                        dpcIsPrimary);
                continue;
            }

            EnsureHasNoUserAnnotation ensureHasNoUserAnnotation =
                    annotationType.getAnnotation(EnsureHasNoUserAnnotation.class);
            if (ensureHasNoUserAnnotation != null) {
                ensureHasNoUser(ensureHasNoUserAnnotation.value());
                continue;
            }

            EnsureHasUserAnnotation ensureHasUserAnnotation =
                    annotationType.getAnnotation(EnsureHasUserAnnotation.class);
            if (ensureHasUserAnnotation != null) {
                OptionalBoolean installInstrumentedApp = (OptionalBoolean)
                        annotation.getClass()
                                .getMethod("installInstrumentedApp").invoke(annotation);
                ensureHasUser(ensureHasUserAnnotation.value(), installInstrumentedApp);
                continue;
            }

            RequireRunOnUserAnnotation requireRunOnUserAnnotation =
                    annotationType.getAnnotation(RequireRunOnUserAnnotation.class);
            if (requireRunOnUserAnnotation != null) {
                requireRunOnUser(requireRunOnUserAnnotation.value());
                continue;
            }

            RequireRunOnProfileAnnotation requireRunOnProfileAnnotation =
                    annotationType.getAnnotation(RequireRunOnProfileAnnotation.class);
            if (requireRunOnProfileAnnotation != null) {
                OptionalBoolean installInstrumentedAppInParent = (OptionalBoolean)
                        annotation.getClass()
                                .getMethod("installInstrumentedAppInParent")
                                .invoke(annotation);


                boolean dpcIsPrimary = false;
                Set<String> affiliationIds = null;
                if (requireRunOnProfileAnnotation.hasProfileOwner()) {
                    dpcIsPrimary = (boolean)
                            annotation.annotationType()
                                    .getMethod("dpcIsPrimary").invoke(annotation);
                    affiliationIds = new HashSet<>(Arrays.asList((String[])
                            annotation.annotationType()
                                    .getMethod("affiliationIds").invoke(annotation)));
                }

                requireRunOnProfile(requireRunOnProfileAnnotation.value(),
                        installInstrumentedAppInParent,
                        requireRunOnProfileAnnotation.hasProfileOwner(),
                        dpcIsPrimary, affiliationIds);
                continue;
            }

            if (annotation instanceof EnsureHasDeviceOwner) {
                EnsureHasDeviceOwner ensureHasDeviceOwnerAnnotation =
                        (EnsureHasDeviceOwner) annotation;
                ensureHasDeviceOwner(ensureHasDeviceOwnerAnnotation.onUser(),
                        ensureHasDeviceOwnerAnnotation.failureMode(),
                        ensureHasDeviceOwnerAnnotation.isPrimary(),
                        new HashSet<>(Arrays.asList(ensureHasDeviceOwnerAnnotation.affiliationIds())));
                continue;
            }

            if (annotation instanceof EnsureHasNoDeviceOwner) {
                ensureHasNoDeviceOwner();
                continue;
            }

            if (annotation instanceof RequireFeature) {
                RequireFeature requireFeatureAnnotation = (RequireFeature) annotation;
                requireFeature(
                        requireFeatureAnnotation.value(),
                        requireFeatureAnnotation.failureMode());
                continue;
            }

            if (annotation instanceof RequireDoesNotHaveFeature) {
                RequireDoesNotHaveFeature requireDoesNotHaveFeatureAnnotation =
                        (RequireDoesNotHaveFeature) annotation;
                requireDoesNotHaveFeature(
                        requireDoesNotHaveFeatureAnnotation.value(),
                        requireDoesNotHaveFeatureAnnotation.failureMode());
                continue;
            }

            if (annotation instanceof EnsureHasProfileOwner) {
                EnsureHasProfileOwner ensureHasProfileOwnerAnnotation =
                        (EnsureHasProfileOwner) annotation;
                ensureHasProfileOwner(ensureHasProfileOwnerAnnotation.onUser(),
                        ensureHasProfileOwnerAnnotation.isPrimary(),
                        new HashSet<>(Arrays.asList(ensureHasProfileOwnerAnnotation.affiliationIds())));
                continue;
            }

            if (annotationType.equals(EnsureHasNoProfileOwner.class)) {
                EnsureHasNoProfileOwner ensureHasNoProfileOwnerAnnotation =
                        (EnsureHasNoProfileOwner) annotation;
                ensureHasNoProfileOwner(ensureHasNoProfileOwnerAnnotation.onUser());
                continue;
            }

            if (annotation instanceof RequireUserSupported) {
                RequireUserSupported requireUserSupportedAnnotation =
                        (RequireUserSupported) annotation;
                requireUserSupported(
                        requireUserSupportedAnnotation.value(),
                        requireUserSupportedAnnotation.failureMode());
                continue;
            }

            if (annotation instanceof RequireGmsInstrumentation) {
                RequireGmsInstrumentation requireGmsInstrumentationAnnotation =
                        (RequireGmsInstrumentation) annotation;
                requireGmsInstrumentation(requireGmsInstrumentationAnnotation.min(),
                        requireGmsInstrumentationAnnotation.max());
                continue;
            }

            if (annotation instanceof RequireSdkVersion) {
                RequireSdkVersion requireSdkVersionAnnotation =
                        (RequireSdkVersion) annotation;

                requireSdkVersion(
                        requireSdkVersionAnnotation.min(),
                        requireSdkVersionAnnotation.max(),
                        requireSdkVersionAnnotation.failureMode());
                continue;
            }

            if (annotation instanceof RequirePackageInstalled) {
                RequirePackageInstalled requirePackageInstalledAnnotation =
                        (RequirePackageInstalled) annotation;
                requirePackageInstalled(
                        requirePackageInstalledAnnotation.value(),
                        requirePackageInstalledAnnotation.onUser(),
                        requirePackageInstalledAnnotation.failureMode());
                continue;
            }

            if (annotation instanceof RequirePackageNotInstalled) {
                RequirePackageNotInstalled requirePackageNotInstalledAnnotation =
                        (RequirePackageNotInstalled) annotation;
                requirePackageNotInstalled(
                        requirePackageNotInstalledAnnotation.value(),
                        requirePackageNotInstalledAnnotation.onUser(),
                        requirePackageNotInstalledAnnotation.failureMode()
                );
                continue;
            }

            if (annotation instanceof EnsurePackageNotInstalled) {
                EnsurePackageNotInstalled ensurePackageNotInstalledAnnotation =
                        (EnsurePackageNotInstalled) annotation;
                ensurePackageNotInstalled(
                        ensurePackageNotInstalledAnnotation.value(),
                        ensurePackageNotInstalledAnnotation.onUser()
                );
                continue;
            }

            if (annotation instanceof EnsureHasPermission) {
                EnsureHasPermission ensureHasPermissionAnnotation =
                        (EnsureHasPermission) annotation;

                for (String permission : ensureHasPermissionAnnotation.value()) {
                    ensureCanGetPermission(permission);
                }


                try {
                    if (permissionContext == null) {
                        permissionContext = sTestApis.permissions().withPermission(
                                ensureHasPermissionAnnotation.value());
                    } else {
                        permissionContext = permissionContext.withPermission(
                                ensureHasPermissionAnnotation.value());
                    }
                } catch (NeneException e) {
                    failOrSkip("Error getting permission: " + e,
                            ensureHasPermissionAnnotation.failureMode());
                }
                continue;
            }

            if (annotation instanceof EnsureDoesNotHavePermission) {
                EnsureDoesNotHavePermission ensureDoesNotHavePermission =
                        (EnsureDoesNotHavePermission) annotation;

                try {
                    if (permissionContext == null) {
                        permissionContext = sTestApis.permissions().withoutPermission(
                                ensureDoesNotHavePermission.value());
                    } else {
                        permissionContext = permissionContext.withoutPermission(
                                ensureDoesNotHavePermission.value());
                    }
                } catch (NeneException e) {
                    failOrSkip("Error denying permission: " + e,
                            ensureDoesNotHavePermission.failureMode());
                }
                continue;
            }
        }

        if (!mHasRequireGmsInstrumentation) {
            // TODO(scottjonathan): Only enforce if we've configured GMS Instrumentation
            requireNoGmsInstrumentation();
        }

        return permissionContext;
    }

    private List<Annotation> getAnnotations(Description description) {
        if (mUsingBedsteadJUnit4 && description.isTest()) {
            // The annotations are already exploded for tests
            return new ArrayList<>(description.getAnnotations());
        }

        // Otherwise we should build a new collection by recursively gathering annotations
        // if we find any which don't work without the runner we should error and fail the test
        List<Annotation> annotations = new ArrayList<>();

        if (description.isTest()) {
            annotations =
                    new ArrayList<>(Arrays.asList(description.getTestClass().getAnnotations()));
        }

        annotations.addAll(description.getAnnotations());

        checkAnnotations(annotations);

        BedsteadJUnit4.resolveRecursiveAnnotations(annotations,
                /* parameterizedAnnotation= */ null);

        checkAnnotations(annotations);

        return annotations;
    }

    private void checkAnnotations(Collection<Annotation> annotations) {
        for (Annotation annotation : annotations) {
            if (annotation.annotationType().getAnnotation(RequiresBedsteadJUnit4.class) != null
                    || annotation.annotationType().getAnnotation(
                            ParameterizedAnnotation.class) != null) {
                throw new AssertionFailedError("Test is annotated "
                        + annotation.annotationType().getSimpleName()
                        + " which requires using the BedsteadJUnit4 test runner");
            }
        }
    }

    private Statement applySuite(final Statement base, final Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                checkValidAnnotations(description);

                TestClass testClass = new TestClass(description.getTestClass());

                boolean skipAfterClass = true;
                PermissionContextImpl permissionContext = null;

                if (!mSkipTests && !mFailTests) {
                    skipAfterClass = false;
                    Log.d(LOG_TAG, "Preparing state for suite " + description.getClassName());

                    try {
                        List<Annotation> annotations =
                                new ArrayList<>(getAnnotations(description));
                        permissionContext = applyAnnotations(annotations);
                    } catch (AssumptionViolatedException e) {
                        mSkipTests = true;
                        mSkipTestsReason = e.getMessage();
                    } catch (AssertionError e) {
                        mFailTests = true;
                        mFailTestsReason = e.getMessage();
                    }

                    Log.d(LOG_TAG,
                            "Finished preparing state for suite "
                                    + description.getClassName());
                }

                if (!mSkipTests && !mFailTests) {
                    runAnnotatedMethods(testClass, BeforeClass.class);
                }

                base.evaluate();

                if (!skipAfterClass) {
                    runAnnotatedMethods(testClass, AfterClass.class);
                }

                if (permissionContext != null) {
                    permissionContext.close();
                }

                if (!mSkipClassTeardown) {
                    teardownShareableState();
                }
            }
        };
    }

    private static final Map<Class<? extends Annotation>, Class<? extends Annotation>>
            BANNED_ANNOTATIONS_TO_REPLACEMENTS = getBannedAnnotationsToReplacements();
    private static Map<
            Class<? extends Annotation>,
            Class<? extends Annotation>> getBannedAnnotationsToReplacements() {
        Map<
                Class<? extends Annotation>,
                Class<? extends Annotation>> bannedAnnotationsToReplacements = new HashMap<>();
        bannedAnnotationsToReplacements.put(org.junit.BeforeClass.class, BeforeClass.class);
        bannedAnnotationsToReplacements.put(org.junit.AfterClass.class, AfterClass.class);
        return bannedAnnotationsToReplacements;
    }

    private void checkValidAnnotations(Description classDescription) {
        for (Method method : classDescription.getTestClass().getMethods()) {
            for (Map.Entry<
                    Class<? extends Annotation>,
                    Class<? extends Annotation>> bannedAnnotation
                    : BANNED_ANNOTATIONS_TO_REPLACEMENTS.entrySet()) {
                if (method.isAnnotationPresent(bannedAnnotation.getKey())) {
                    throw new IllegalStateException("Do not use "
                            + bannedAnnotation.getKey().getCanonicalName()
                            + " when using DeviceState, replace with "
                            + bannedAnnotation.getValue().getCanonicalName());
                }
            }

            if (method.getAnnotation(BeforeClass.class) != null
                    || method.getAnnotation(AfterClass.class) != null) {
                checkPublicStaticVoidNoArgs(method);
            }
        }
    }

    private void checkPublicStaticVoidNoArgs(Method method) {
        if (method.getParameterTypes().length > 0) {
            throw new IllegalStateException(
                    "Method " + method.getName() + " should have no parameters");
        }
        if (method.getReturnType() != Void.TYPE) {
            throw new IllegalStateException("Method " + method.getName() + "() should be void");
        }
        if (!Modifier.isStatic(method.getModifiers())) {
            throw new IllegalStateException("Method " + method.getName() + "() should be static");
        }
        if (!Modifier.isPublic(method.getModifiers())) {
            throw new IllegalStateException("Method " + method.getName() + "() should be public");
        }
    }

    private void runAnnotatedMethods(
            TestClass testClass, Class<? extends Annotation> annotation) throws Throwable {

        List<FrameworkMethod> methods = new ArrayList<>(testClass.getAnnotatedMethods(annotation));
        Collections.reverse(methods);
        for (FrameworkMethod method : methods) {
            try {
                method.invokeExplosively(testClass.getJavaClass());
            } catch (InvocationTargetException e) {
                throw e.getCause();
            }
        }
    }

    private void requireRunOnUser(String userType) {
        User instrumentedUser = sTestApis.users().instrumented().resolve();

        assumeTrue("This test only runs on users of type " + userType,
                instrumentedUser.type().name().equals(userType));

        mUsers.put(instrumentedUser.type(), instrumentedUser);
    }

    private void requireRunOnProfile(String userType,
            OptionalBoolean installInstrumentedAppInParent,
            boolean hasProfileOwner, boolean dpcIsPrimary, Set<String> affiliationIds) {
        User instrumentedUser = sTestApis.users().instrumented().resolve();

        assumeTrue("This test only runs on users of type " + userType,
                instrumentedUser.type().name().equals(userType));

        if (!mProfiles.containsKey(instrumentedUser.type())) {
            mProfiles.put(instrumentedUser.type(), new HashMap<>());
        }

        mProfiles.get(instrumentedUser.type()).put(instrumentedUser.parent(), instrumentedUser);

        if (installInstrumentedAppInParent.equals(OptionalBoolean.TRUE)) {
            sTestApis.packages().find(sContext.getPackageName()).install(
                    instrumentedUser.parent());
        } else if (installInstrumentedAppInParent.equals(OptionalBoolean.FALSE)) {
            sTestApis.packages().find(sContext.getPackageName()).uninstall(
                    instrumentedUser.parent());
        }

        if (hasProfileOwner) {
            ensureHasProfileOwner(instrumentedUser, dpcIsPrimary, affiliationIds);
        } else {
            ensureHasNoProfileOwner(instrumentedUser);
        }
    }

    private void requireFeature(String feature, FailureMode failureMode) {
        checkFailOrSkip("Device must have feature " + feature,
                sTestApis.packages().features().contains(feature), failureMode);
    }

    private void requireDoesNotHaveFeature(String feature, FailureMode failureMode) {
        checkFailOrSkip("Device must not have feature " + feature,
                !sTestApis.packages().features().contains(feature), failureMode);
    }

    private void requireNoGmsInstrumentation() {
        boolean instrumentingGms =
                sTestApis.context().instrumentedContext().getPackageName().equals(GMS_PKG);

        checkFailOrSkip(
                "This test never runs using gms instrumentation",
                !instrumentingGms,
                FailureMode.SKIP
        );
    }

    private void requireGmsInstrumentation(int min, int max) {
        mHasRequireGmsInstrumentation = true;
        boolean instrumentingGms =
                sTestApis.context().instrumentedContext().getPackageName().equals(GMS_PKG);

        if (meetsSdkVersionRequirements(min, max)) {
            checkFailOrSkip(
                    "For SDK versions between " + min +  " and " + max
                            + " (inclusive), this test only runs when using gms instrumentation",
                    instrumentingGms,
                    FailureMode.SKIP
            );
        } else {
            checkFailOrSkip(
                    "For SDK versions between " + min +  " and " + max
                            + " (inclusive), this test only runs when not using gms "
                            + "instrumentation",
                    !instrumentingGms,
                    FailureMode.SKIP
            );
        }
    }

    private void requireSdkVersion(int min, int max, FailureMode failureMode) {
        requireSdkVersion(min, max, failureMode,
                "Sdk version must be between " + min +  " and " + max + " (inclusive)");
    }

    private void requireSdkVersion(
            int min, int max, FailureMode failureMode, String failureMessage) {
        checkFailOrSkip(
                failureMessage,
                meetsSdkVersionRequirements(min, max),
                failureMode
        );
    }

    private com.android.bedstead.nene.users.UserType requireUserSupported(
            String userType, FailureMode failureMode) {
        com.android.bedstead.nene.users.UserType resolvedUserType =
                sTestApis.users().supportedType(userType);

        checkFailOrSkip(
                "Device must support user type " + userType
                + " only supports: " + sTestApis.users().supportedTypes(),
                resolvedUserType != null, failureMode);

        return resolvedUserType;
    }

    private void checkFailOrSkip(String message, boolean value, FailureMode failureMode) {
        if (failureMode.equals(FailureMode.FAIL)) {
            assertWithMessage(message).that(value).isTrue();
        } else if (failureMode.equals(FailureMode.SKIP)) {
            assumeTrue(message, value);
        } else {
            throw new IllegalStateException("Unknown failure mode: " + failureMode);
        }
    }

    private void failOrSkip(String message, FailureMode failureMode) {
        if (failureMode.equals(FailureMode.FAIL)) {
            throw new AssertionError(message);
        } else if (failureMode.equals(FailureMode.SKIP)) {
            throw new AssumptionViolatedException(message);
        } else {
            throw new IllegalStateException("Unknown failure mode: " + failureMode);
        }
    }

    public enum UserType {
        /** Only to be used with annotations. */
        ANY,
        SYSTEM_USER,
        CURRENT_USER,
        PRIMARY_USER,
        SECONDARY_USER,
        WORK_PROFILE,
        TV_PROFILE,
    }

    private static final String LOG_TAG = "DeviceState";

    private static final Context sContext = sTestApis.context().instrumentedContext();

    private final Map<com.android.bedstead.nene.users.UserType, UserReference> mUsers =
            new HashMap<>();
    private final Map<com.android.bedstead.nene.users.UserType, Map<UserReference, UserReference>>
            mProfiles = new HashMap<>();
    private DevicePolicyController mDeviceOwner;
    private Map<UserReference, DevicePolicyController> mProfileOwners = new HashMap<>();
    private DevicePolicyController mPrimaryDpc;

    private final List<UserReference> mCreatedUsers = new ArrayList<>();
    private final List<UserBuilder> mRemovedUsers = new ArrayList<>();
    private final List<BlockingBroadcastReceiver> mRegisteredBroadcastReceivers = new ArrayList<>();
    private boolean mHasChangedDeviceOwner = false;
    private DevicePolicyController mOriginalDeviceOwner = null;
    private Map<UserReference, DevicePolicyController> mChangedProfileOwners = new HashMap<>();

    /**
     * Get the {@link UserReference} of the work profile for the current user.
     *
     * <p>If the current user is a work profile, then the current user will be returned.
     *
     * <p>This should only be used to get work profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed work profile
     */
    public UserReference workProfile() {
        return workProfile(/* forUser= */ UserType.CURRENT_USER);
    }

    /**
     * Get the {@link UserReference} of the work profile.
     *
     * <p>This should only be used to get work profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed work profile for the given user
     */
    public UserReference workProfile(UserType forUser) {
        return workProfile(resolveUserTypeToUser(forUser));
    }

    /**
     * Get the {@link UserReference} of the work profile.
     *
     * <p>This should only be used to get work profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed work profile for the given user
     */
    public UserReference workProfile(UserReference forUser) {
        return profile(MANAGED_PROFILE_TYPE_NAME, forUser);
    }

    /**
     * Get the {@link UserReference} of the profile of the given type for the given user.
     *
     * <p>This should only be used to get profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed profile for the given user
     */
    public UserReference profile(String profileType, UserType forUser) {
        return profile(profileType, resolveUserTypeToUser(forUser));
    }

    /**
     * Get the {@link UserReference} of the profile for the current user.
     *
     * <p>If the current user is a profile of the correct type, then the current user will be
     * returned.
     *
     * <p>This should only be used to get profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed profile
     */
    public UserReference profile(String profileType) {
        return profile(profileType, /* forUser= */ UserType.CURRENT_USER);
    }

    /**
     * Get the {@link UserReference} of the profile of the given type for the given user.
     *
     * <p>This should only be used to get profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed profile for the given user
     */
    public UserReference profile(String profileType, UserReference forUser) {
        com.android.bedstead.nene.users.UserType resolvedUserType =
                sTestApis.users().supportedType(profileType);

        if (resolvedUserType == null) {
            throw new IllegalStateException("Can not have a profile of type " + profileType
                    + " as they are not supported on this device");
        }

        return profile(resolvedUserType, forUser);
    }

    /**
     * Get the {@link UserReference} of the profile of the given type for the given user.
     *
     * <p>This should only be used to get profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed profile for the given user
     */
    public UserReference profile(
            com.android.bedstead.nene.users.UserType userType, UserReference forUser) {
        if (userType == null || forUser == null) {
            throw new NullPointerException();
        }

        if (!mProfiles.containsKey(userType) || !mProfiles.get(userType).containsKey(forUser)) {
            UserReference parentUser = sTestApis.users().instrumented().resolve().parent();

            if (parentUser != null) {
                if (mProfiles.containsKey(userType)
                        && mProfiles.get(userType).containsKey(parentUser)) {
                    return mProfiles.get(userType).get(parentUser);
                }
            }

            throw new IllegalStateException(
                    "No harrier-managed profile of type " + userType + ". This method should only"
                            + " be used when Harrier has been used to create the profile.");
        }

        return mProfiles.get(userType).get(forUser);
    }

    /**
     * Get the {@link UserReference} of the tv profile for the current user
     *
     * <p>This should only be used to get tv profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed tv profile
     */
    public UserReference tvProfile() {
        return tvProfile(/* forUser= */ UserType.CURRENT_USER);
    }

    /**
     * Get the {@link UserReference} of the tv profile.
     *
     * <p>This should only be used to get tv profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed tv profile
     */
    public UserReference tvProfile(UserType forUser) {
        return tvProfile(resolveUserTypeToUser(forUser));
    }

    /**
     * Get the {@link UserReference} of the tv profile.
     *
     * <p>This should only be used to get tv profiles managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed tv profile
     */
    public UserReference tvProfile(UserReference forUser) {
        return profile(TV_PROFILE_TYPE_NAME, forUser);
    }

    /**
     * Get the user ID of the first human user on the device.
     */
    public UserReference primaryUser() {
        return sTestApis.users().all()
                .stream().filter(User::isPrimary).findFirst()
                .orElseThrow(IllegalStateException::new);
    }

    /**
     * Get a secondary user.
     *
     * <p>This should only be used to get secondary users managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed secondary user
     */
    public UserReference secondaryUser() {
        return user(SECONDARY_USER_TYPE_NAME);
    }

    /**
     * Get a user of the given type.
     *
     * <p>This should only be used to get users managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed user of the correct type
     */
    public UserReference user(String userType) {
        com.android.bedstead.nene.users.UserType resolvedUserType =
                sTestApis.users().supportedType(userType);

        if (resolvedUserType == null) {
            throw new IllegalStateException("Can not have a user of type " + userType
                    + " as they are not supported on this device");
        }

        return user(resolvedUserType);
    }

    /**
     * Get a user of the given type.
     *
     * <p>This should only be used to get users managed by Harrier (using either the
     * annotations or calls to the {@link DeviceState} class.
     *
     * @throws IllegalStateException if there is no harrier-managed user of the correct type
     */
    public UserReference user(com.android.bedstead.nene.users.UserType userType) {
        if (userType == null) {
            throw new NullPointerException();
        }

        if (!mUsers.containsKey(userType)) {
            throw new IllegalStateException(
                    "No harrier-managed user of type " + userType + ". This method should only be"
                            + "used when Harrier has been used to create the user.");
        }

        return mUsers.get(userType);
    }

    private UserReference ensureHasProfile(
            String profileType,
            OptionalBoolean installInstrumentedApp,
            UserType forUser,
            boolean hasProfileOwner,
            boolean profileOwnerIsPrimary) {
        requireFeature("android.software.managed_users", FailureMode.SKIP);
        com.android.bedstead.nene.users.UserType resolvedUserType =
                requireUserSupported(profileType, FailureMode.SKIP);

        UserReference forUserReference = resolveUserTypeToUser(forUser);

        UserReference profile =
                sTestApis.users().findProfileOfType(resolvedUserType, forUserReference);
        if (profile == null) {
            profile = createProfile(resolvedUserType, forUserReference);
        }

        profile.start();

        if (installInstrumentedApp.equals(OptionalBoolean.TRUE)) {
            sTestApis.packages().find(sContext.getPackageName()).install(profile);
        } else if (installInstrumentedApp.equals(OptionalBoolean.FALSE)) {
            sTestApis.packages().find(sContext.getPackageName()).uninstall(profile);
        }

        if (!mProfiles.containsKey(resolvedUserType)) {
            mProfiles.put(resolvedUserType, new HashMap<>());
        }

        mProfiles.get(resolvedUserType).put(forUserReference, profile);

        if (hasProfileOwner) {
            ensureHasProfileOwner(profile, profileOwnerIsPrimary, /* affiliationIds= */ null);
        }
        return profile;
    }

    private void ensureHasNoProfile(String profileType, UserType forUser) {
        requireFeature("android.software.managed_users", FailureMode.SKIP);

        UserReference forUserReference = resolveUserTypeToUser(forUser);
        com.android.bedstead.nene.users.UserType resolvedProfileType =
                sTestApis.users().supportedType(profileType);

        if (resolvedProfileType == null) {
            // These profile types don't exist so there can't be any
            return;
        }

        UserReference profile =
                sTestApis.users().findProfileOfType(
                        resolvedProfileType,
                        forUserReference);
        if (profile != null) {
            removeAndRecordUser(profile);
        }
    }

    private void ensureHasUser(String userType, OptionalBoolean installInstrumentedApp) {
        com.android.bedstead.nene.users.UserType resolvedUserType =
                requireUserSupported(userType, FailureMode.SKIP);

        Collection<UserReference> users = sTestApis.users().findUsersOfType(resolvedUserType);

        UserReference user = users.isEmpty() ? createUser(resolvedUserType)
                : users.iterator().next();

        user.start();

        if (installInstrumentedApp.equals(OptionalBoolean.TRUE)) {
            sTestApis.packages().find(sContext.getPackageName()).install(user);
        } else if (installInstrumentedApp.equals(OptionalBoolean.FALSE)) {
            sTestApis.packages().find(sContext.getPackageName()).uninstall(user);
        }

        mUsers.put(resolvedUserType, user);
    }

    /**
     * Ensure that there is no user of the given type.
     */
    private void ensureHasNoUser(String userType) {
        com.android.bedstead.nene.users.UserType resolvedUserType =
                sTestApis.users().supportedType(userType);

        if (resolvedUserType == null) {
            // These user types don't exist so there can't be any
            return;
        }

        for (UserReference secondaryUser : sTestApis.users().findUsersOfType(resolvedUserType)) {
            removeAndRecordUser(secondaryUser);
        }
    }

    private void removeAndRecordUser(UserReference userReference) {
        if (userReference == null) {
            return; // Nothing to remove
        }

        User user = userReference.resolve();

        if (!mCreatedUsers.remove(user)) {
            mRemovedUsers.add(sTestApis.users().createUser()
                    .name(user.name())
                    .type(user.type())
                    .parent(user.parent()));
        }

        user.remove();
    }

    public void requireCanSupportAdditionalUser() {
        int maxUsers = getMaxNumberOfUsersSupported();
        int currentUsers = sTestApis.users().all().size();

        assumeTrue("The device does not have space for an additional user (" + currentUsers +
                " current users, " + maxUsers + " max users)", currentUsers + 1 <= maxUsers);
    }

    /**
     * Create and register a {@link BlockingBroadcastReceiver} which will be unregistered after the
     * test has run.
     */
    public BlockingBroadcastReceiver registerBroadcastReceiver(String action) {
        return registerBroadcastReceiver(action, /* checker= */ null);
    }

    /**
     * Create and register a {@link BlockingBroadcastReceiver} which will be unregistered after the
     * test has run.
     */
    public BlockingBroadcastReceiver registerBroadcastReceiver(
            String action, Function<Intent, Boolean> checker) {
        BlockingBroadcastReceiver broadcastReceiver =
                new BlockingBroadcastReceiver(mContext, action, checker);
        broadcastReceiver.register();
        mRegisteredBroadcastReceivers.add(broadcastReceiver);

        return broadcastReceiver;
    }

    private UserReference resolveUserTypeToUser(UserType userType) {
        switch (userType) {
            case SYSTEM_USER:
                return sTestApis.users().system();
            case CURRENT_USER:
                return sTestApis.users().instrumented();
            case PRIMARY_USER:
                return primaryUser();
            case SECONDARY_USER:
                return secondaryUser();
            case WORK_PROFILE:
                return workProfile();
            case TV_PROFILE:
                return tvProfile();
            case ANY:
                throw new IllegalStateException("ANY UserType can not be used here");
            default:
                throw new IllegalArgumentException("Unknown user type " + userType);
        }
    }

    private void teardownNonShareableState() {
        mProfiles.clear();
        mUsers.clear();

        for (BlockingBroadcastReceiver broadcastReceiver : mRegisteredBroadcastReceivers) {
            broadcastReceiver.unregisterQuietly();
        }
        mRegisteredBroadcastReceivers.clear();
        mPrimaryDpc = null;
    }

    private void teardownShareableState() {
        if (mHasChangedDeviceOwner) {
            if (mOriginalDeviceOwner == null) {
                if (mDeviceOwner != null) {
                    mDeviceOwner.remove();
                }
            } else if (!mOriginalDeviceOwner.equals(mDeviceOwner)) {
                mDeviceOwner.remove();
                sTestApis.devicePolicy().setDeviceOwner(
                        mOriginalDeviceOwner.user(), mOriginalDeviceOwner.componentName());
            }
            mHasChangedDeviceOwner = false;
            mOriginalDeviceOwner = null;
        }

        for (Map.Entry<UserReference, DevicePolicyController> originalProfileOwner :
                mChangedProfileOwners.entrySet()) {

            ProfileOwner currentProfileOwner =
                    sTestApis.devicePolicy().getProfileOwner(originalProfileOwner.getKey());

            if (Objects.equal(currentProfileOwner, originalProfileOwner.getValue())) {
                continue; // No need to restore
            }

            if (currentProfileOwner != null) {
                currentProfileOwner.remove();
            }

            if (originalProfileOwner.getValue() != null) {
                sTestApis.devicePolicy().setProfileOwner(originalProfileOwner.getKey(),
                        originalProfileOwner.getValue().componentName());
            }
        }
        mChangedProfileOwners.clear();

        for (UserReference user : mCreatedUsers) {
            user.remove();
        }

        mCreatedUsers.clear();

        for (UserBuilder userBuilder : mRemovedUsers) {
            userBuilder.create();
        }

        mRemovedUsers.clear();
    }

    private UserReference createProfile(
            com.android.bedstead.nene.users.UserType profileType, UserReference parent) {
        requireCanSupportAdditionalUser();
        try {
            UserReference user = sTestApis.users().createUser()
                    .parent(parent)
                    .type(profileType)
                    .createAndStart();
            mCreatedUsers.add(user);
            return user;
        } catch (NeneException e) {
            throw new IllegalStateException("Error creating profile of type " + profileType, e);
        }
    }

    private UserReference createUser(com.android.bedstead.nene.users.UserType userType) {
        requireCanSupportAdditionalUser();
        try {
            UserReference user = sTestApis.users().createUser()
                    .type(userType)
                    .createAndStart();
            mCreatedUsers.add(user);
            return user;
        } catch (NeneException e) {
            throw new IllegalStateException("Error creating user of type " + userType, e);
        }
    }

    private int getMaxNumberOfUsersSupported() {
        try {
            return ShellCommand.builder("pm get-max-users")
                    .validate((output) -> output.startsWith("Maximum supported users:"))
                    .executeAndParseOutput(
                            (output) -> Integer.parseInt(output.split(": ", 2)[1].trim()));
        } catch (AdbException e) {
            throw new IllegalStateException("Invalid command output", e);
        }
    }

    private void ensureHasDeviceOwner(UserType onUser, FailureMode failureMode, boolean isPrimary, Set<String> affiliationIds) {
        // TODO(scottjonathan): Should support non-remotedpc device owner (default to remotedpc)
        // TODO(scottjonathan): Should allow setting the device owner on a different user
        UserReference userReference = resolveUserTypeToUser(onUser);
        if (isPrimary && mPrimaryDpc != null && !userReference.equals(mPrimaryDpc.user())) {
            throw new IllegalStateException("Only one DPC can be marked as primary per test (current primary is " + mPrimaryDpc + ")");
        }
        if (!userReference.equals(sTestApis.users().instrumented())) {
            // INTERACT_ACROSS_USERS_FULL is required for RemoteDPC
            ensureCanGetPermission(INTERACT_ACROSS_USERS_FULL);
        }

        DeviceOwner currentDeviceOwner = sTestApis.devicePolicy().getDeviceOwner();

        if (currentDeviceOwner != null
                && currentDeviceOwner.componentName().equals(RemoteDpc.DPC_COMPONENT_NAME)) {
            mDeviceOwner = currentDeviceOwner;
        } else {
            UserReference instrumentedUser = sTestApis.users().instrumented();

            if (!Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S)) {
                // Prior to S we can't set device owner if there are other users on the device
                for (UserReference u : sTestApis.users().all()) {
                    if (u.equals(instrumentedUser)) {
                        continue;
                    }
                    try {
                        removeAndRecordUser(u);
                    } catch (NeneException e) {
                        failOrSkip(
                                "Error removing user to prepare for DeviceOwner: " + e.toString(),
                                failureMode);
                    }
                }
            }

            // TODO(scottjonathan): Remove accounts
            ensureHasNoProfileOwner(userReference);

            if (!mHasChangedDeviceOwner) {
                mOriginalDeviceOwner = currentDeviceOwner;
                mHasChangedDeviceOwner = true;
            }

            mDeviceOwner = RemoteDpc.setAsDeviceOwner(userReference)
                    .devicePolicyController();
        }

        if (isPrimary) {
            mPrimaryDpc = mDeviceOwner;
        }
        
        RemoteDpc.forDevicePolicyController(mDeviceOwner).devicePolicyManager()
                .setAffiliationIds(affiliationIds);
    }

    private void ensureHasProfileOwner(UserType onUser, boolean isPrimary, Set<String> affiliationIds) {
        // TODO(scottjonathan): Should support non-remotedpc profile owner (default to remotedpc)
        UserReference user = resolveUserTypeToUser(onUser);
        ensureHasProfileOwner(user, isPrimary, affiliationIds);
    }

    private void ensureHasProfileOwner(
            UserReference user, boolean isPrimary, Set<String> affiliationIds) {
        if (isPrimary && mPrimaryDpc != null && !user.equals(mPrimaryDpc.user())) {
            throw new IllegalStateException("Only one DPC can be marked as primary per test");
        }

        if (!user.equals(sTestApis.users().instrumented())) {
            // INTERACT_ACROSS_USERS_FULL is required for RemoteDPC
            ensureCanGetPermission(INTERACT_ACROSS_USERS_FULL);
        }

        ProfileOwner currentProfileOwner = sTestApis.devicePolicy().getProfileOwner(user);
        DeviceOwner currentDeviceOwner = sTestApis.devicePolicy().getDeviceOwner();

        if (currentDeviceOwner != null && currentDeviceOwner.user().equals(user)) {
            // Can't have DO and PO on the same user
            ensureHasNoDeviceOwner();
        }

        if (currentProfileOwner != null
                && currentProfileOwner.componentName().equals(
                RemoteDpc.DPC_COMPONENT_NAME)) {
            mProfileOwners.put(user, currentProfileOwner);
        } else {
            if (!mChangedProfileOwners.containsKey(user)) {
                mChangedProfileOwners.put(user, currentProfileOwner);
            }

            mProfileOwners.put(user, RemoteDpc.setAsProfileOwner(user).devicePolicyController());
        }

        if (isPrimary) {
            mPrimaryDpc = mProfileOwners.get(user);
        }

        if (affiliationIds != null) {
            RemoteDpc profileOwner = profileOwner(user);
            profileOwner.devicePolicyManager()
                    .setAffiliationIds(affiliationIds);
        }
    }

    private void ensureHasNoDeviceOwner() {
        DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();

        if (deviceOwner == null) {
            return;
        }

        if (!mHasChangedDeviceOwner) {
            mOriginalDeviceOwner = deviceOwner;
            mHasChangedDeviceOwner = true;
        }

        mDeviceOwner = null;
        deviceOwner.remove();
    }

    private void ensureHasNoProfileOwner(UserType onUser) {
        UserReference user = resolveUserTypeToUser(onUser);

        ensureHasNoProfileOwner(user);
    }

    private void ensureHasNoProfileOwner(UserReference user) {

        ProfileOwner currentProfileOwner = sTestApis.devicePolicy().getProfileOwner(user);

        if (currentProfileOwner == null) {
            return;
        }

        if (!mChangedProfileOwners.containsKey(user)) {
            mChangedProfileOwners.put(user, currentProfileOwner);
        }

        sTestApis.devicePolicy().getProfileOwner(user).remove();
        mProfileOwners.remove(user);
    }

    /**
     * Get the {@link RemoteDpc} for the device owner controlled by Harrier.
     *
     * <p>If no Harrier-managed device owner exists, an exception will be thrown.
     *
     * <p>If the device owner is not a RemoteDPC then an exception will be thrown
     */
    public RemoteDpc deviceOwner() {
        if (mDeviceOwner == null) {
            throw new IllegalStateException("No Harrier-managed device owner. This method should "
                    + "only be used when Harrier was used to set the Device Owner.");
        }
        if (!mDeviceOwner.componentName().equals(REMOTE_DPC_COMPONENT_NAME)) {
            throw new IllegalStateException("The device owner is not a RemoteDPC."
                    + " You must use Nene to query for this device owner.");
        }

        return RemoteDpc.forDevicePolicyController(mDeviceOwner);
    }

    /**
     * Get the {@link RemoteDpc} for the profile owner on the current user controlled by Harrier.
     *
     * <p>If no Harrier-managed profile owner exists, an exception will be thrown.
     *
     * <p>If the profile owner is not a RemoteDPC then an exception will be thrown.
     */
    public RemoteDpc profileOwner() {
        return profileOwner(UserType.CURRENT_USER);
    }

    /**
     * Get the {@link RemoteDpc} for the profile owner on the given user controlled by Harrier.
     *
     * <p>If no Harrier-managed profile owner exists, an exception will be thrown.
     *
     * <p>If the profile owner is not a RemoteDPC then an exception will be thrown.
     */
    public RemoteDpc profileOwner(UserType onUser) {
        if (onUser == null) {
            throw new NullPointerException();
        }

        return profileOwner(resolveUserTypeToUser(onUser));
    }

    /**
     * Get the {@link RemoteDpc} for the profile owner on the given user controlled by Harrier.
     *
     * <p>If no Harrier-managed profile owner exists, an exception will be thrown.
     *
     * <p>If the profile owner is not a RemoteDPC then an exception will be thrown.
     */
    public RemoteDpc profileOwner(UserReference onUser) {
        if (onUser == null) {
            throw new NullPointerException();
        }

        if (!mProfileOwners.containsKey(onUser)) {
            throw new IllegalStateException("No Harrier-managed profile owner. This method should "
                    + "only be used when Harrier was used to set the Profile Owner.");
        }

        DevicePolicyController profileOwner = mProfileOwners.get(onUser);

        if (!profileOwner.componentName().equals(REMOTE_DPC_COMPONENT_NAME)) {
            throw new IllegalStateException("The profile owner is not a RemoteDPC."
                    + " You must use Nene to query for this profile owner.");
        }

        return RemoteDpc.forDevicePolicyController(profileOwner);
    }

    private void requirePackageInstalled(
            String packageName, UserType forUser, FailureMode failureMode) {

        Package pkg = sTestApis.packages().find(packageName).resolve();
        checkFailOrSkip(
                packageName + " is required to be installed for " + forUser,
                pkg != null,
                failureMode);

        if (forUser.equals(UserType.ANY)) {
            checkFailOrSkip(
                    packageName + " is required to be installed",
                    !pkg.installedOnUsers().isEmpty(),
                    failureMode);
        } else {
            checkFailOrSkip(
                    packageName + " is required to be installed for " + forUser,
                    pkg.installedOnUsers().contains(resolveUserTypeToUser(forUser)),
                    failureMode);
        }
    }

    private void requirePackageNotInstalled(
            String packageName, UserType forUser, FailureMode failureMode) {
        Package pkg = sTestApis.packages().find(packageName).resolve();
        if (pkg == null) {
            // Definitely not installed
            return;
        }

        if (forUser.equals(UserType.ANY)) {
            checkFailOrSkip(
                    packageName + " is required to be not installed",
                    pkg.installedOnUsers().isEmpty(),
                    failureMode);
        } else {
            checkFailOrSkip(
                    packageName + " is required to be not installed for " + forUser,
                    !pkg.installedOnUsers().contains(resolveUserTypeToUser(forUser)),
                    failureMode);
        }
    }

    private void ensurePackageNotInstalled(
            String packageName, UserType forUser) {

        Package pkg = sTestApis.packages().find(packageName).resolve();
        if (pkg == null) {
            // Definitely not installed
            return;
        }

        if (forUser.equals(UserType.ANY)) {
            if (!pkg.installedOnUsers().isEmpty()) {
                pkg.uninstallFromAllUsers();
            }
        } else {
            UserReference user = resolveUserTypeToUser(forUser);
            if (pkg.installedOnUsers().contains(user)) {
                pkg.uninstall(user);
            }
        }
    }

    /**
     * Get the most appropriate {@link RemoteDpc} instance for the device state.
     *
     * <p>This method should only be used by tests which are annotated with {@link PolicyTest}.
     *
     * <p>If no DPC is set as the "primary" DPC for the device state, then this method will first
     * check for a profile owner in the current user, or else check for a device owner.
     *
     * <p>If no Harrier-managed profile owner or device owner exists, an exception will be thrown.
     *
     * <p>If the profile owner or device owner is not a RemoteDPC then an exception will be thrown.
     */
    public RemoteDpc dpc() {
        if (mPrimaryDpc != null) {
            return RemoteDpc.forDevicePolicyController(mPrimaryDpc);
        }

        if (mProfileOwners.containsKey(sTestApis.users().instrumented())) {
            DevicePolicyController profileOwner =
                    mProfileOwners.get(sTestApis.users().instrumented());


            if (profileOwner.componentName().equals(REMOTE_DPC_COMPONENT_NAME)) {
                return RemoteDpc.forDevicePolicyController(profileOwner);
            }
        }

        if (mDeviceOwner != null) {
            if (mDeviceOwner.componentName().equals(REMOTE_DPC_COMPONENT_NAME)) {
                return RemoteDpc.forDevicePolicyController(mDeviceOwner);
            }

        }

        throw new IllegalStateException("No Harrier-managed profile owner or device owner.");
    }

    private void ensureCanGetPermission(String permission) {
        // TODO(scottjonathan): Apply gms permission switches automatically rather than hard-coding
        // TODO(scottjonathan): Add a config to only enforce gms permission when needed
        if (permission.equals(NOTIFY_PENDING_SYSTEM_UPDATE)) {
            requireGmsInstrumentation(1, Build.VERSION_CODES.R);
        }
        // TODO(scottjonathan): Apply version-specific constraints automatically
        if (permission.equals(INTERACT_ACROSS_USERS_FULL)) {
            requireSdkVersion(
                    Build.VERSION_CODES.Q, Integer.MAX_VALUE, FailureMode.SKIP,
                    "This test requires INTERACT_ACROSS_USERS_FULL which can only be used on Q+");
        }
    }
}
