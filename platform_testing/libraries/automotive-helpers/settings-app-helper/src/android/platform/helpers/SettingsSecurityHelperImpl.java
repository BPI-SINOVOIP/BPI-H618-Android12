package android.platform.helpers;

import android.app.Instrumentation;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;

import java.util.List;

public class SettingsSecurityHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoSecuritySettingsHelper {
    public SettingsSecurityHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void setLockByPassword(String password) {
        openChooseLockTypeMenu();
        UiObject2 password_menu =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.LOCK_TYPE_PASSWORD));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), password_menu);
        typePasswordOnTextEditor(password);
        pressEnter();
        typePasswordOnTextEditor(password);
        pressEnter();
    }

    private void openChooseLockTypeMenu() {
        List<UiObject2> titles =
                findUiObjects(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.TITLE));
        if (titles == null || titles.isEmpty()) {
            throw new RuntimeException("Unable to find Setting title");
        }
        UiObject2 title = titles.get(titles.size() - 1);
        if (title != null
                && title.getText()
                        .equalsIgnoreCase(
                                getResourceValue(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.SECURITY_SETTINGS,
                                        AutoConfigConstants.CHOOSE_LOCK_TYPE))) {
            // CHOOSE_LOCK_TYPE is already open
            return;
        }
        UiObject2 profileLockMenu =
                scrollAndFindUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.PROFILE_LOCK));
        if (profileLockMenu == null) {
            throw new RuntimeException("Unable to find Choose a lock type menu.");
        }
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), profileLockMenu);
    }

    private void typePasswordOnTextEditor(String password) {
        UiObject2 textEditor =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.ENTER_PASSWORD));
        textEditor.setText(password);
    }

    /** {@inheritDoc} */
    @Override
    public void setLockByPin(String pin) {
        openChooseLockTypeMenu();
        UiObject2 pin_menu =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.LOCK_TYPE_PIN));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), pin_menu);
        selectPinOnPinPad(pin);
        UiObject2 continue_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.CONTINUE_BUTTON));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), continue_button);
        selectPinOnPinPad(pin);
        UiObject2 confirm_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.CONFIRM_BUTTON));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), confirm_button);
    }

    private void selectPinOnPinPad(String pin) {
        int length = pin.length();
        for (int i = 0; i < length; i++) {
            char c = pin.charAt(i);
            String numberText = "" + c;
            BySelector number_selector = By.text(numberText);
            UiObject2 number = findUiObject(number_selector);
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), number);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void unlockByPassword(String password) {
        UiObject2 textEditor =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.ENTER_PASSWORD));
        textEditor.setText(password);
        pressEnter();
    }

    /** {@inheritDoc} */
    @Override
    public void unlockByPin(String pin) {
        selectPinOnPinPad(pin);
        UiObject2 enter_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.ENTER_PIN_BUTTON));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), enter_button);
        UiObject2 pinPad =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.PIN_PAD));
        if (pinPad != null) {
            throw new RuntimeException("PIN input is not corrected");
        }
    }

    /** {@inheritDoc} */
    @Override
    public void removeLock() {
        openChooseLockTypeMenu();
        UiObject2 none_menu =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.LOCK_TYPE_NONE));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), none_menu);
        UiObject2 remove_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.REMOVE_BUTTON));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), remove_button);
    }

    /** {@inheritDoc} */
    @Override
    public boolean isDeviceLocked() {
        openChooseLockTypeMenu();
        UiObject2 textEditor =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.ENTER_PASSWORD));
        UiObject2 pinPad =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SECURITY_SETTINGS,
                                AutoConfigConstants.PIN_PAD));
        return textEditor != null || pinPad != null;
    }
}
