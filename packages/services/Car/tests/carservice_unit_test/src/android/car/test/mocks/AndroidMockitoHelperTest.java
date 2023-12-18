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

package android.car.test.mocks;

import static android.car.test.mocks.AndroidMockitoHelper.mockAmGetCurrentUser;
import static android.car.test.mocks.AndroidMockitoHelper.mockBinderGetCallingUserHandle;
import static android.car.test.mocks.AndroidMockitoHelper.mockContextGetService;
import static android.car.test.mocks.AndroidMockitoHelper.mockQueryService;
import static android.car.test.mocks.AndroidMockitoHelper.mockSmGetService;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmCreateUser;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmGetAliveUsers;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmGetSystemUser;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmGetUserInfo;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmIsHeadlessSystemUserMode;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmIsUserRunning;
import static android.car.test.mocks.AndroidMockitoHelper.mockUmRemoveUserOrSetEphemeral;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;

import android.app.ActivityManager;
import android.app.Service;
import android.car.test.util.UserTestingHelper;
import android.car.test.util.Visitor;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.os.Binder;
import android.os.IBinder;
import android.os.IInterface;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;

import com.android.dx.mockito.inline.extended.StaticMockitoSession;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.quality.Strictness;

public final class AndroidMockitoHelperTest {

    private static final int TEST_USER_ID = 100;

    private UserInfo mTestUser;

    private StaticMockitoSession mMockSession;

    @Mock private UserManager mMockedUserManager;

    @Before
    public void setUp() {
        mMockSession = mockitoSession()
                .initMocks(this)
                .strictness(Strictness.LENIENT)
                .spyStatic(UserManager.class)
                .spyStatic(ActivityManager.class)
                .spyStatic(ServiceManager.class)
                .spyStatic(Binder.class)
                .startMocking();

        mTestUser = mockUmCreateUser(mMockedUserManager, "testUser",
                UserManager.USER_TYPE_FULL_SYSTEM, UserInfo.FLAG_ADMIN, TEST_USER_ID);
    }

    @After
    public void tearDown() {
        mMockSession.finishMocking();
    }

    @Test
    public void testMockUmCreateUser() {
        assertThat(mMockedUserManager.createUser(mTestUser.name, mTestUser.userType,
                mTestUser.flags).getUserHandle()).isEqualTo(mTestUser.getUserHandle());
    }

    @Test
    public void testMockAmGetCurrentUser() {
        mockAmGetCurrentUser(UserHandle.USER_NULL);

        assertThat(ActivityManager.getCurrentUser()).isEqualTo(UserHandle.USER_NULL);
    }

    @Test
    public void testMockUmIsHeadlessSystemUserMode_true() {
        mockUmIsHeadlessSystemUserMode(/* mode= */ true);

        assertThat(UserManager.isHeadlessSystemUserMode()).isTrue();
    }

    @Test
    public void testMockUmIsHeadlessSystemUserMode_false() {
        mockUmIsHeadlessSystemUserMode(/* mode= */ false);

        assertThat(UserManager.isHeadlessSystemUserMode()).isFalse();
    }

    @Test
    public void testMockUmGetUserInfo() {
        mockUmGetUserInfo(mMockedUserManager, mTestUser);

        assertThat(mMockedUserManager.getUserInfo(TEST_USER_ID)).isSameInstanceAs(mTestUser);
    }

    @Test
    public void testMockUmGetSystemUser() {
        mockUmGetSystemUser(mMockedUserManager);

        UserInfo expectedUser = new UserTestingHelper.UserInfoBuilder(UserHandle.USER_SYSTEM)
                .setFlags(UserInfo.FLAG_SYSTEM).build();

        assertThat(
                mMockedUserManager.getUserInfo(UserHandle.USER_SYSTEM).getUserHandle()).isEqualTo(
                expectedUser.getUserHandle());
    }

    @Test
    public void testMockUmIsUserRunning_running() {
        mockUmIsUserRunning(mMockedUserManager, TEST_USER_ID, /* isRunning= */ true);

        assertThat(mMockedUserManager.isUserRunning(TEST_USER_ID)).isTrue();
    }

    @Test
    public void testMockUmIsUserRunning_notRunning() {
        mockUmIsUserRunning(mMockedUserManager, TEST_USER_ID, /* isRunning= */ false);

        assertThat(mMockedUserManager.isUserRunning(TEST_USER_ID)).isFalse();
    }

    @Test
    public void testMockUmGetAliveUsers() {
        UserInfo user1 = mMockedUserManager.createUser("test1",
                UserManager.USER_TYPE_FULL_SECONDARY, UserInfo.FLAG_ADMIN);
        UserInfo user2 = mMockedUserManager.createUser("test2", UserManager.USER_TYPE_FULL_GUEST,
                UserInfo.FLAG_EPHEMERAL);

        mockUmGetAliveUsers(mMockedUserManager, user1, user2);

        assertThat(mMockedUserManager.getAliveUsers()).containsExactly(user1, user2);
    }

    @Test
    public void testMockBinderGetCallingUserHandle() {
        mockBinderGetCallingUserHandle(TEST_USER_ID);

        assertThat(Binder.getCallingUserHandle()).isEqualTo(UserHandle.of(TEST_USER_ID));
    }

    @Test
    public void testMockSmGetService() {
        IBinder someBinder = mock(IBinder.class);
        mockSmGetService("someServiceName", someBinder);

        assertThat(ServiceManager.getService("someServiceName")).isEqualTo(someBinder);
    }

    @Test
    public void testMockQueryService() {
        IInterface someService = mock(IInterface.class);
        IBinder someBinder = mock(IBinder.class);

        mockQueryService("someServiceName", someBinder, someService);

        assertThat(ServiceManager.getService("someServiceName")).isEqualTo(someBinder);
        assertThat(someBinder.queryLocalInterface("anyString")).isEqualTo(someService);
    }

    @Test
    public void testMockUmRemoveUserOrSetEphemeral() {
        VisitorImpl<UserInfo> visitor = new VisitorImpl<>();

        mockUmRemoveUserOrSetEphemeral(mMockedUserManager, mTestUser,
                /* evenWhenDisallowed= */ true, /* result= */ 1, visitor);

        mMockedUserManager.removeUserOrSetEphemeral(TEST_USER_ID, /* evenWhenDisallowed= */ true);

        assertThat(visitor.mVisited).isEqualTo(mTestUser);
    }

    private static final class VisitorImpl<T> implements Visitor<T> {

        T mVisited;

        @Override
        public void visit(T visited) {
            mVisited = visited;
        }
    }

    @Test
    public void testMockContextGetService() {
        Context context = mock(Context.class);
        Service someService = mock(Service.class);

        mockContextGetService(context, Service.class, someService);

        assertThat(context.getSystemService(Service.class)).isEqualTo(someService);
    }

    @Test
    public void testMockContextGetService_PackageManager() {
        Context context = mock(Context.class);
        PackageManager packageManager = mock(PackageManager.class);

        mockContextGetService(context, PackageManager.class, packageManager);

        assertThat(context.getPackageManager()).isEqualTo(packageManager);
    }
}
