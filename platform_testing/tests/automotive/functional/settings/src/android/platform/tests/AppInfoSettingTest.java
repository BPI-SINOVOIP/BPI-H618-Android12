package android.platform.tests;

import static junit.framework.Assert.assertTrue;

import android.platform.helpers.AutoConfigConstants;
import android.platform.helpers.AutoConfigConstants;
import android.platform.helpers.AutoUtility;
import android.platform.helpers.IAutoAppInfoSettingsHelper;
import android.platform.helpers.IAutoAppInfoSettingsHelper.State;
import android.platform.helpers.IAutoSettingHelper;
import android.platform.helpers.HelperAccessor;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class AppInfoSettingTest {
    private HelperAccessor<IAutoAppInfoSettingsHelper> mAppInfoSettingsHelper;
    private HelperAccessor<IAutoSettingHelper> mSettingHelper;

    private static final String CONTACTS_APP = "Contacts";
    private static final String PHONE_PERMISSION = "Phone";
    private static final String CONTACT_PACKAGE = "com.android.contacts";

    public AppInfoSettingTest() throws Exception {
        mAppInfoSettingsHelper = new HelperAccessor<>(IAutoAppInfoSettingsHelper.class);
        mSettingHelper = new HelperAccessor<>(IAutoSettingHelper.class);
    }

    @BeforeClass
    public static void exitSuw() {
        AutoUtility.exitSuw();
    }

    @Before
    public void openAppInfoFacet() {
        mSettingHelper.get().openSetting(AutoConfigConstants.APPS_SETTINGS);
        mAppInfoSettingsHelper.get().showAllApps();
    }

    @After
    public void goBackToSettingsScreen() {
        mSettingHelper.get().goBackToSettingsScreen();
    }

    @Test
    public void testDisableEnableApplication() {
        mAppInfoSettingsHelper.get().selectApp(CONTACTS_APP);
        mAppInfoSettingsHelper.get().enableDisableApplication(State.DISABLE);
        assertTrue(
                "Application is not disabled",
                mAppInfoSettingsHelper.get().isApplicationDisabled(CONTACT_PACKAGE));
        mAppInfoSettingsHelper.get().enableDisableApplication(State.ENABLE);
        assertTrue(
                "Application is not enabled",
                !mAppInfoSettingsHelper.get().isApplicationDisabled(CONTACT_PACKAGE));
    }

    @Test
    public void testApplicationPermissions() {
        mAppInfoSettingsHelper.get().selectApp(CONTACTS_APP);
        mAppInfoSettingsHelper.get().setAppPermission(PHONE_PERMISSION, State.DISABLE);
        assertTrue(
                "Permission is not disabled",
                !mAppInfoSettingsHelper.get().getCurrentPermissions().contains(PHONE_PERMISSION));
        mAppInfoSettingsHelper.get().setAppPermission(PHONE_PERMISSION, State.ENABLE);
        assertTrue(
                "Permission is disabled",
                mAppInfoSettingsHelper.get().getCurrentPermissions().contains(PHONE_PERMISSION));
    }
}
