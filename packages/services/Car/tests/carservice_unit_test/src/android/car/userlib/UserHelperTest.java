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

package android.car.userlib;

import static android.car.test.util.UserTestingHelper.newUser;

import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.content.Context;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

public final class UserHelperTest extends AbstractExtendedMockitoTestCase {

    @Mock private Context mContext;
    @Mock private UserManager mUserManager;

    // Not worth to mock because it would need to mock a Drawable used by UserIcons.
    private final Resources mResources = InstrumentationRegistry.getTargetContext().getResources();

    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder session) {
        session.spyStatic(UserManager.class);
    }

    @Before
    public void setUp() {
        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mContext.getApplicationContext()).thenReturn(mContext);
        when(mContext.getResources()).thenReturn(mResources);
    }

    @Test
    public void testDefaultNonAdminRestrictions() {
        int userId = 20;
        UserInfo newNonAdmin = newUser(userId);

        UserHelper.setDefaultNonAdminRestrictions(mContext, newNonAdmin, /* enable= */ true);

        verify(mUserManager).setUserRestriction(
                UserManager.DISALLOW_FACTORY_RESET, /* enable= */ true, UserHandle.of(userId));
    }

    @Test
    public void testDefaultNonAdminRestrictions_nullContext_throwsException() {
        int userId = 20;
        UserInfo newNonAdmin = newUser(userId);

        assertThrows(IllegalArgumentException.class,
                () -> UserHelper.setDefaultNonAdminRestrictions(/* context= */ null,
                        newNonAdmin, /* enable= */ true));
    }

    @Test
    public void testDefaultNonAdminRestrictions_nullUser_throwsException() {
        assertThrows(IllegalArgumentException.class,
                () -> UserHelper.setDefaultNonAdminRestrictions(mContext, /* user= */
                        null, /* enable= */ true));
    }

    @Test
    public void testAssignDefaultIcon() {
        int userId = 20;
        UserInfo newNonAdmin = newUser(userId);

        Bitmap bitmap = UserHelper.assignDefaultIcon(mContext, newNonAdmin);

        verify(mUserManager).setUserIcon(userId, bitmap);
    }

    @Test
    public void testAssignDefaultIcon_nullContext_throwsException() {
        int userId = 20;
        UserInfo newNonAdmin = newUser(userId);

        assertThrows(IllegalArgumentException.class,
                () -> UserHelper.assignDefaultIcon(/* context= */ null, newNonAdmin));
    }

    @Test
    public void testAssignDefaultIcon_nullUser_throwsException() {
        assertThrows(IllegalArgumentException.class,
                () -> UserHelper.assignDefaultIcon(mContext, /* user= */ null));
    }

    @Test
    public void testGrantAdminPermissions_nonAdmin() {
        int userId = 30;
        UserInfo testInfo = newUser(userId);

        // Test that non-admins cannot grant admin permissions.
        when(mUserManager.isAdminUser()).thenReturn(false);
        UserHelper.grantAdminPermissions(mContext, testInfo);
        verify(mUserManager, never()).setUserAdmin(userId);
    }

    @Test
    public void testGrantAdminPermissions_admin() {
        int userId = 30;
        UserInfo testInfo = newUser(userId);

        // Admins can grant admin permissions.
        when(mUserManager.isAdminUser()).thenReturn(true);
        UserHelper.grantAdminPermissions(mContext, testInfo);
        verify(mUserManager).setUserAdmin(userId);
    }

    @Test
    public void testGrantingAdminPermissionsRemovesNonAdminRestrictions() {
        int testUserId = 30;
        boolean restrictionEnabled = false;
        UserInfo testInfo = newUser(testUserId);

        // Only admins can grant permissions.
        when(mUserManager.isAdminUser()).thenReturn(true);

        UserHelper.grantAdminPermissions(mContext, testInfo);

        // verify all restrictions
        for (String restriction : UserHelper.DEFAULT_NON_ADMIN_RESTRICTIONS) {
            verify(mUserManager).setUserRestriction(restriction, restrictionEnabled,
                    UserHandle.of(testUserId));
        }
        for (String restriction : UserHelper.DEFAULT_NON_ADMIN_RESTRICTIONS) {
            verify(mUserManager).setUserRestriction(restriction, restrictionEnabled,
                    UserHandle.of(testUserId));
        }
    }
}
