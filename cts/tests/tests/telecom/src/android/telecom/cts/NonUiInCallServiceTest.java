package android.telecom.cts;

import static android.telecom.cts.TestUtils.TEST_PHONE_ACCOUNT_HANDLE;
import static android.telecom.cts.TestUtils.waitOnAllHandlers;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.DeadObjectException;
import android.telecom.cts.api29incallservice.CtsApi29InCallService;
import android.telecom.cts.api29incallservice.ICtsApi29InCallServiceControl;
import android.util.Pair;

import androidx.test.InstrumentationRegistry;

public class NonUiInCallServiceTest extends BaseTelecomTestWithMockServices {
    private static final String LOG_TAG = NonUiInCallServiceTest.class.getSimpleName();

    private ServiceConnection mServiceConnection;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (mShouldTestTelecom) {
            setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mShouldTestTelecom) {
            mTelecomManager.unregisterPhoneAccount(TEST_PHONE_ACCOUNT_HANDLE);
        }
        super.tearDown();
        waitOnAllHandlers(getInstrumentation());
    }

    public void testMidCallComponentEnablement() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(
                        "android.permission.CONTROL_INCALL_EXPERIENCE",
                        "android.permission.CHANGE_COMPONENT_ENABLED_STATE");
        try {
            mContext.getPackageManager().setComponentEnabledSetting(
                    ComponentName.createRelative(CtsApi29InCallService.PACKAGE_NAME,
                            "." + CtsApi29InCallService.class.getSimpleName()),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED, 0);
            ICtsApi29InCallServiceControl controlInterface = setUpControl();

            addAndVerifyNewIncomingCall(createTestNumber(), new Bundle());
            waitOnAllHandlers(getInstrumentation());
            assertFalse("Non-UI incall incorrectly bound to despite being disabled",
                    controlInterface.hasReceivedBindRequest());

            mContext.getPackageManager().setComponentEnabledSetting(
                    ComponentName.createRelative(CtsApi29InCallService.PACKAGE_NAME,
                            "." + CtsApi29InCallService.class.getSimpleName()),
                    PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                    PackageManager.DONT_KILL_APP);

            boolean hasBound = controlInterface.waitForBindRequest();
            assertTrue("InCall was not bound to", hasBound);
            waitOnAllHandlers(getInstrumentation());

            assertEquals("Call was not sent to incall", 1, controlInterface.getLocalCallCount());

            try {
                controlInterface.kill();
            } catch (DeadObjectException e) {
                //expected
            }
            tearDownControl();
        } finally {
            InstrumentationRegistry.getInstrumentation().getUiAutomation()
                    .dropShellPermissionIdentity();
        }
    }

    public void testNullBinding() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(
                        "android.permission.CONTROL_INCALL_EXPERIENCE",
                        "android.permission.CHANGE_COMPONENT_ENABLED_STATE");
        try {
            mContext.getPackageManager().setComponentEnabledSetting(
                    ComponentName.createRelative(CtsApi29InCallService.PACKAGE_NAME,
                            "." + CtsApi29InCallService.class.getSimpleName()),
                    PackageManager.COMPONENT_ENABLED_STATE_ENABLED, 0);
            ICtsApi29InCallServiceControl controlInterface = setUpControl();
            controlInterface.setShouldReturnNullBinding(true);

            int currentCallCount = addNewIncomingCall(createTestNumber(), new Bundle());
            // The test InCallService can be bound and unbound before this test gets a chance to
            // validate. Ensure that the test verifies it is bound before checking if onCallAdded
            // was called.
            assertTrue("Non-UI incall incorrectly not bound to despite being enabled",
                    controlInterface.waitForBindRequest());
            verifyNewIncomingCall(currentCallCount);

            assertEquals("Call was sent to incall despite null binding",
                    0, controlInterface.getLocalCallCount());

            try {
                controlInterface.kill();
            } catch (DeadObjectException e) {
                //expected
            }
            tearDownControl();
        } finally {
            InstrumentationRegistry.getInstrumentation().getUiAutomation()
                    .dropShellPermissionIdentity();
        }
    }
    private ICtsApi29InCallServiceControl setUpControl() throws Exception {
        Pair<ServiceConnection, ICtsApi29InCallServiceControl> setupResult =
                Api29InCallUtils.setupControl(mContext);
        mServiceConnection = setupResult.first;
        return setupResult.second;
    }

    private void tearDownControl() throws Exception {
        Api29InCallUtils.tearDownControl(mContext,
                mServiceConnection);
    }
}