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

package android.devicepolicy.cts;

import static com.android.bedstead.harrier.DeviceState.UserType.PRIMARY_USER;
import static com.android.bedstead.harrier.OptionalBoolean.FALSE;
import static com.android.bedstead.harrier.OptionalBoolean.TRUE;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.testng.Assert.assertThrows;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.CrossProfileApps;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.uiautomator.By;
import androidx.test.uiautomator.UiDevice;
import androidx.test.uiautomator.UiObject2;
import androidx.test.uiautomator.Until;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.EnsureHasSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasWorkProfile;
import com.android.bedstead.harrier.annotations.Postsubmit;
import com.android.bedstead.harrier.annotations.RequireRunOnPrimaryUser;
import com.android.bedstead.harrier.annotations.RequireRunOnSecondaryUser;
import com.android.bedstead.harrier.annotations.RequireRunOnWorkProfile;

import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.concurrent.TimeUnit;

@RunWith(BedsteadJUnit4.class)
public final class CrossProfileAppsTest {

    private static final String ID_USER_TEXTVIEW =
            "com.android.cts.devicepolicy:id/user_textview";
    private static final long TIMEOUT_WAIT_UI = TimeUnit.SECONDS.toMillis(10);
    private static final Context sContext = ApplicationProvider.getApplicationContext();
    private static final CrossProfileApps sCrossProfileApps =
            sContext.getSystemService(CrossProfileApps.class);
    private static final UserManager sUserManager = sContext.getSystemService(UserManager.class);

    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    @Test
    @RequireRunOnPrimaryUser
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromPrimaryUser_doesNotContainPrimaryUser() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).doesNotContain(sDeviceState.primaryUser().userHandle());
    }
    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasSecondaryUser
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromPrimaryUser_doesNotContainSecondaryUser() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).doesNotContain(sDeviceState.secondaryUser().userHandle());
    }

    @Test
    @RequireRunOnWorkProfile(installInstrumentedAppInParent = TRUE)
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromWorkProfile_containsPrimaryUser() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).contains(sDeviceState.primaryUser().userHandle());
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasWorkProfile
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromPrimaryUser_containsWorkProfile() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).contains(sDeviceState.workProfile().userHandle());
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasWorkProfile(installInstrumentedApp = FALSE)
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromPrimaryUser_appNotInstalledInWorkProfile_doesNotContainWorkProfile() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).doesNotContain(sDeviceState.workProfile().userHandle());
    }

    @Test
    @RequireRunOnSecondaryUser
    @EnsureHasWorkProfile(forUser = PRIMARY_USER)
    @Postsubmit(reason="new test")
    public void getTargetUserProfiles_callingFromSecondaryUser_doesNotContainWorkProfile() {
        List<UserHandle> targetProfiles = sCrossProfileApps.getTargetUserProfiles();

        assertThat(targetProfiles).doesNotContain(
                sDeviceState.workProfile(/* forUser= */ PRIMARY_USER).userHandle());
    }

    @Test
    @RequireRunOnWorkProfile(installInstrumentedAppInParent = TRUE)
    @Ignore // TODO(scottjonathan): Replace use of UIAutomator
    @Postsubmit(reason="new test")
    public void startMainActivity_callingFromWorkProfile_targetIsPrimaryUser_launches() {
        sCrossProfileApps.startMainActivity(
                new ComponentName(sContext, MainActivity.class),
                sDeviceState.workProfile().userHandle());

        assertMainActivityLaunchedForUser(sDeviceState.primaryUser().userHandle());
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasWorkProfile
    @Ignore // TODO(scottjonathan): Replace use of UIAutomator
    @Postsubmit(reason="new test")
    public void startMainActivity_callingFromPrimaryUser_targetIsWorkProfile_launches() {
        sCrossProfileApps.startMainActivity(
                new ComponentName(sContext, MainActivity.class),
                sDeviceState.workProfile().userHandle());

        assertMainActivityLaunchedForUser(sDeviceState.workProfile().userHandle());
    }

    private void assertMainActivityLaunchedForUser(UserHandle user) {
        // TODO(scottjonathan): Replace this with a standard event log or similar to avoid UI
        // Look for the text view to verify that MainActivity is started.
        UiObject2 textView = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation())
                .wait(
                        Until.findObject(By.res(ID_USER_TEXTVIEW)),
                        TIMEOUT_WAIT_UI);
        assertWithMessage("Failed to start activity in target user")
                .that(textView).isNotNull();
        // Look for the text in textview, it should be the serial number of target user.
        assertWithMessage("Activity is started in wrong user")
                .that(textView.getText())
                .isEqualTo(String.valueOf(sUserManager.getSerialNumberForUser(user)));
    }

    @Test
    @Postsubmit(reason="new test")
    public void startMainActivity_activityNotExported_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.startMainActivity(
                    new ComponentName(sContext, NonExportedActivity.class),
                    sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @Postsubmit(reason="new test")
    public void startMainActivity_activityNotMain_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.startMainActivity(
                    new ComponentName(sContext, NonMainActivity.class),
                    sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @Ignore // TODO(scottjonathan): This requires another app to be installed which can be launched
    @Postsubmit(reason="new test")
    public void startMainActivity_activityIncorrectPackage_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {

        });
    }

    @Test
    @RequireRunOnPrimaryUser
    @Postsubmit(reason="new test")
    public void
            startMainActivity_callingFromPrimaryUser_targetIsPrimaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.startMainActivity(
                    new ComponentName(sContext, MainActivity.class),
                    sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasSecondaryUser
    @Postsubmit(reason="new test")
    public void
    startMainActivity_callingFromPrimaryUser_targetIsSecondaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.startMainActivity(
                    new ComponentName(sContext, MainActivity.class),
                    sDeviceState.secondaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnSecondaryUser
    @EnsureHasWorkProfile(forUser = PRIMARY_USER)
    @Postsubmit(reason="new test")
    public void
    startMainActivity_callingFromSecondaryUser_targetIsWorkProfile_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.startMainActivity(
                    new ComponentName(sContext, MainActivity.class),
                    sDeviceState.workProfile(/* forUser= */ PRIMARY_USER).userHandle());
        });
    }

    @Test
    @RequireRunOnPrimaryUser
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabel_callingFromPrimaryUser_targetIsPrimaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingLabel(sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasSecondaryUser
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabel_callingFromPrimaryUser_targetIsSecondaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingLabel(sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnSecondaryUser
    @EnsureHasWorkProfile(forUser = PRIMARY_USER)
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabel_callingFromSecondaryUser_targetIsWorkProfile_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingLabel(
                    sDeviceState.workProfile(/* forUser= */ PRIMARY_USER).userHandle());
        });
    }

    @Test
    @RequireRunOnWorkProfile(installInstrumentedAppInParent = TRUE)
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabel_callingFromWorProfile_targetIsPrimaryUser_notNull() {
        assertThat(sCrossProfileApps.getProfileSwitchingLabel(
                sDeviceState.primaryUser().userHandle())).isNotNull();
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasWorkProfile
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabel_callingFromPrimaryUser_targetIsWorkProfile_notNull() {
        assertThat(sCrossProfileApps.getProfileSwitchingLabel(
                sDeviceState.workProfile().userHandle())).isNotNull();
    }

    @Test
    @RequireRunOnPrimaryUser
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabelIconDrawable_callingFromPrimaryUser_targetIsPrimaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingIconDrawable(
                    sDeviceState.primaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasSecondaryUser
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabelIconDrawable_callingFromPrimaryUser_targetIsSecondaryUser_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingIconDrawable(
                    sDeviceState.secondaryUser().userHandle());
        });
    }

    @Test
    @RequireRunOnSecondaryUser
    @EnsureHasWorkProfile(forUser = PRIMARY_USER)
    @Postsubmit(reason="new test")
    public void getProfileSwitchingLabelIconDrawable_callingFromSecondaryUser_targetIsWorkProfile_throwsSecurityException() {
        assertThrows(SecurityException.class, () -> {
            sCrossProfileApps.getProfileSwitchingIconDrawable(
                    sDeviceState.workProfile(/* forUser= */ PRIMARY_USER).userHandle());
        });
    }

    @Test
    @RequireRunOnWorkProfile(installInstrumentedAppInParent = TRUE)
    @Postsubmit(reason="new test")
    public void getProfileSwitchingIconDrawable_callingFromWorkProfile_targetIsPrimaryUser_notNull() {
        assertThat(sCrossProfileApps.getProfileSwitchingIconDrawable(
                sDeviceState.primaryUser().userHandle())).isNotNull();
    }

    @Test
    @RequireRunOnPrimaryUser
    @EnsureHasWorkProfile
    @Postsubmit(reason="new test")
    public void getProfileSwitchingIconDrawable_callingFromPrimaryUser_targetIsWorkProfile_notNull() {
        assertThat(sCrossProfileApps.getProfileSwitchingIconDrawable(
                sDeviceState.workProfile().userHandle())).isNotNull();
    }
}