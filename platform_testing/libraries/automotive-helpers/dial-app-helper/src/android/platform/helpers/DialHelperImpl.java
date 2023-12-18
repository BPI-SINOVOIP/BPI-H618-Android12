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

package android.platform.helpers;

import android.app.Instrumentation;
import android.content.Context;
import android.platform.helpers.exceptions.UnknownUiException;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;

import java.util.regex.Pattern;
import android.os.SystemClock;

public class DialHelperImpl extends AbstractAutoStandardAppHelper implements IAutoDialHelper {
    private static final String LOG_TAG = DialHelperImpl.class.getSimpleName();

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;

    public DialHelperImpl(Instrumentation instr) {
        super(instr);
        mBluetoothManager =
                (BluetoothManager)
                        mInstrumentation.getContext().getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = mBluetoothManager.getAdapter();
    }

    /** {@inheritDoc} */
    public void open() {
        pressHome();
        waitForIdle();
        String phone_launch_command =
                "am start -n " + getApplicationConfig(AutoConfigConstants.PHONE_ACTIVITY);
        executeShellCommand(phone_launch_command);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.DIAL_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public String getLauncherName() {
        return "Phone";
    }

    /** {@inheritDoc} */
    public void makeCall() {
        BySelector dialButtonSelector =
                getResourceFromConfig(
                        AutoConfigConstants.PHONE,
                        AutoConfigConstants.DIAL_PAD_VIEW,
                        AutoConfigConstants.MAKE_CALL);
        UiObject2 dialButton = findUiObject(dialButtonSelector);
        if (dialButton != null) {
            clickAndWaitForGone(dialButton, dialButtonSelector);
        } else {
            throw new UnknownUiException("Unable to find call button.");
        }
        waitForWindowUpdate(getApplicationConfig(AutoConfigConstants.DIAL_PACKAGE));
        SystemClock.sleep(3000); // Wait for the call to go through
    }

    /** {@inheritDoc} */
    public void endCall() {
        BySelector endButtonSelector =
                getResourceFromConfig(
                        AutoConfigConstants.PHONE,
                        AutoConfigConstants.IN_CALL_VIEW,
                        AutoConfigConstants.END_CALL);
        UiObject2 endButton = findUiObject(endButtonSelector);
        if (endButton != null) {
            clickAndWaitForGone(endButton, endButtonSelector);
        } else {
            throw new UnknownUiException("Unable to find end call button.");
        }
        waitForWindowUpdate(getApplicationConfig(AutoConfigConstants.DIAL_PACKAGE));
    }

    /** {@inheritDoc} */
    public void dialANumber(String phoneNumber) {
        enterNumber(phoneNumber);
        waitForIdle();
    }

    /** {@inheritDoc} */
    public void openCallHistory() {
        BySelector callHistorySelector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CALL_HISTORY_VIEW,
                                        AutoConfigConstants.CALL_HISTORY_MENU));
        UiObject2 historyMenuButton = findUiObject(callHistorySelector);
        if (historyMenuButton != null) {
            clickAndWaitForIdleScreen(historyMenuButton);
        } else {
            throw new UnknownUiException("Unable to find call history menu.");
        }
    }

    /** {@inheritDoc} */
    public void callContact(String contactName) {
        openContacts();
        dialFromList(contactName);
        SystemClock.sleep(3000); // Wait for the call to go through
    }

    /** {@inheritDoc} */
    public String getRecentCallHistory() {
        UiObject2 recentCallHistory = getCallHistory();
        if (recentCallHistory != null) {
            return recentCallHistory.getText();
        } else {
            throw new UnknownUiException("Unable to find history");
        }
    }

    /** {@inheritDoc} */
    public void callMostRecentHistory() {
        UiObject2 recentCallHistory = getCallHistory();
        clickAndWaitForIdleScreen(recentCallHistory);
    }

    /** {@inheritDoc} */
    public void deleteDialedNumber() {
        String phoneNumber = getDialInNumber();
        UiObject2 deleteButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.DIAL_PAD_VIEW,
                                AutoConfigConstants.DELETE_NUMBER));
        for (int index = 0; index < phoneNumber.length(); ++index) {
            if (deleteButton != null) {
                clickAndWaitForIdleScreen(deleteButton);
            } else {
                throw new UnknownUiException("Unable to find delete button");
            }
        }
    }

    /** {@inheritDoc} */
    public String getDialInNumber() {
        UiObject2 dialInNumber =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.DIAL_PAD_VIEW,
                                AutoConfigConstants.DIALED_NUMBER));
        String phoneNumber = dialInNumber.getText();
        return phoneNumber;
    }

    /** {@inheritDoc} */
    public String getDialedNumber() {
        UiObject2 dialedNumber =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.DIALED_CONTACT_TITLE));
        String phoneNumber = dialedNumber.getText();
        return phoneNumber;
    }

    /** {@inheritDoc} */
    public String getDialedContactName() {
        UiObject2 dialedContactName =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.DIALED_CONTACT_TITLE));
        String callerName = dialedContactName.getText();
        return callerName;
    }

    /** {@inheritDoc} */
    public void inCallDialPad(String phoneNumber) {
        UiObject2 dialPad =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.SWITCH_TO_DIAL_PAD));
        if (dialPad != null) {
            clickAndWaitForIdleScreen(dialPad);
        } else {
            throw new UnknownUiException("Unable to find in-call dial pad");
        }
        enterNumber(phoneNumber);
        waitForIdle();
    }

    /** {@inheritDoc} */
    public void muteCall() {
        UiObject2 muteButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.MUTE_CALL));
        if (muteButton != null) {
            clickAndWaitForIdleScreen(muteButton);
        } else {
            throw new UnknownUiException("Unable to find mute call button.");
        }
    }

    /** {@inheritDoc} */
    public void unmuteCall() {
        UiObject2 muteButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.MUTE_CALL));
        if (muteButton != null) {
            clickAndWaitForIdleScreen(muteButton);
        } else {
            throw new UnknownUiException("Unable to find unmute call button.");
        }
    }

    /** {@inheritDoc} */
    public void dialFromList(String contact) {
        UiObject2 contactToCall = findUiObject(By.text(contact));
        if (contactToCall != null) {
            clickAndWaitForIdleScreen(contactToCall);
        } else {
            scrollThroughCallList(contact);
        }
    }

    /** {@inheritDoc} */
    public void changeAudioSource(AudioSource source) {
        UiObject2 voiceChannelButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.CHANGE_VOICE_CHANNEL));
        clickAndWaitForIdleScreen(voiceChannelButton);
        UiObject2 channelButton;
        if (source == AudioSource.PHONE) {
            channelButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.IN_CALL_VIEW,
                                    AutoConfigConstants.VOICE_CHANNEL_PHONE));
        } else {
            channelButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.IN_CALL_VIEW,
                                    AutoConfigConstants.VOICE_CHANNEL_CAR));
        }
        clickAndWaitForIdleScreen(channelButton);
        SystemClock.sleep(3000);
    }

    /** {@inheritDoc} */
    public String getContactName() {
        UiObject2 contactName =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.DIALED_CONTACT_TITLE));
        if (contactName != null) {
            return contactName.getText();
        } else {
            throw new UnknownUiException("Unable to find contact name.");
        }
    }

    /** {@inheritDoc} */
    public String getContactType() {
        // Contact number displayed on screen contains type
        // e.g. Mobile xxx-xxx-xxxx , Work xxx-xxx-xxxx
        UiObject2 contactDetail =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.IN_CALL_VIEW,
                                AutoConfigConstants.DIALED_CONTACT_NUMBER));
        if (contactDetail != null) {
            return contactDetail.getText();
        } else {
            throw new UnknownUiException("Unable to find contact details.");
        }
    }

    /** {@inheritDoc} */
    public void searchContactsByName(String contact) {
        openSearchContact();
        UiObject2 searchBox =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.CONTACT_SEARCH_BAR));
        if (searchBox != null) {
            searchBox.setText(contact);
        } else {
            throw new UnknownUiException("Unable to find the search box.");
        }
    }

    /** {@inheritDoc} */
    public void searchContactsByNumber(String number) {
        openSearchContact();
        UiObject2 searchBox =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.CONTACT_SEARCH_BAR));
        if (searchBox != null) {
            searchBox.setText(number);
        } else {
            throw new UnknownUiException("Unable to find the search box.");
        }
    }

    /** {@inheritDoc} */
    public String getFirstSearchResult() {
        UiObject2 searchResult =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.SEARCH_RESULT));
        String result;
        if (searchResult != null) {
            result = searchResult.getText();
        } else {
            throw new UnknownUiException("Unable to find the search result");
        }
        pressBack();
        return result;
    }

    /** {@inheritDoc} */
    public void sortContactListBy(OrderType orderType) {
        openContacts();
        openSettings();
        UiObject2 contactOrderButton =
                findUiObject(
                        By.clickable(true)
                                .hasDescendant(
                                        getResourceFromConfig(
                                                AutoConfigConstants.PHONE,
                                                AutoConfigConstants.CONTACTS_VIEW,
                                                AutoConfigConstants.CONTACT_ORDER)));
        clickAndWaitForIdleScreen(contactOrderButton);
        UiObject2 orderButton;
        if (orderType == OrderType.FIRST_NAME) {
            orderButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.CONTACTS_VIEW,
                                    AutoConfigConstants.SORT_BY_FIRST_NAME));
        } else {
            orderButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.CONTACTS_VIEW,
                                    AutoConfigConstants.SORT_BY_LAST_NAME));
        }
        if (orderButton != null) {
            clickAndWaitForIdleScreen(orderButton);
        } else {
            throw new UnknownUiException("Unable to find dialer settings button");
        }
        UiObject2 contactsMenu =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.CONTACTS_MENU));
        while (contactsMenu == null) {
            pressBack();
            contactsMenu =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.CONTACTS_VIEW,
                                    AutoConfigConstants.CONTACTS_MENU));
        }
    }

    /** {@inheritDoc} */
    public String getFirstContactFromContactList() {
        openContacts();
        scrollToTop();
        UiObject2 firstContact =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.CONTACT_NAME));
        if (firstContact != null) {
            return firstContact.getText();
        } else {
            throw new UnknownUiException("Unable to find first contact from contact list");
        }
    }

    /** {@inheritDoc} */
    public boolean isContactInFavorites(String contact) {
        openFavorites();
        UiObject2 obj = findUiObject(By.text(contact));
        return obj != null;
    }

    /** {@inheritDoc} */
    public void openDetailsPage(String contactName) {
        openContacts();
        UiObject2 contact =
                scrollAndFindUiObject(
                        By.text(Pattern.compile(contactName, Pattern.CASE_INSENSITIVE)));
        if (contact != null) {
            UiObject2 contactDetailButton;
            UiObject2 showDetailsButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.PHONE,
                                    AutoConfigConstants.CONTACTS_VIEW,
                                    AutoConfigConstants.CONTACT_DETAIL));
            if (showDetailsButton == null) {
                contactDetailButton = contact;
            } else {
                int lastIndex = contact.getParent().getChildren().size() - 1;
                contactDetailButton = contact.getParent().getChildren().get(lastIndex);
            }
            clickAndWaitForIdleScreen(contactDetailButton);
        } else {
            throw new UnknownUiException(
                    String.format("Unable to find contact name %s.", contactName));
        }
    }

    /** This method is used to get the first history in the Recents tab. */
    private UiObject2 getCallHistory() {
        UiObject2 callHistory =
                findUiObject(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CALL_HISTORY_VIEW,
                                        AutoConfigConstants.CALL_HISTORY_INFO))
                        .getParent();
        UiObject2 recentCallHistory = callHistory.getChildren().get(2);
        return recentCallHistory;
    }

    /** This method is used to open the contacts menu */
    public void openContacts() {
        BySelector contactMenuSelector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CONTACTS_VIEW,
                                        AutoConfigConstants.CONTACTS_MENU));
        UiObject2 contactMenuButton = findUiObject(contactMenuSelector);
        if (contactMenuButton != null) {
            clickAndWaitForIdleScreen(contactMenuButton);
        } else {
            throw new UnknownUiException("Unable to find Contacts menu.");
        }
    }

    /** This method opens the contact search window. */
    private void openSearchContact() {
        UiObject2 searchContact =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.SEARCH_CONTACT));
        if (searchContact != null) {
            clickAndWaitForIdleScreen(searchContact);
        } else {
            throw new UnknownUiException("Unable to find the search contact button.");
        }
    }

    /** This method opens the settings for contact. */
    private void openSettings() {
        UiObject2 settingButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.CONTACT_SETTINGS));
        if (settingButton != null) {
            clickAndWaitForIdleScreen(settingButton);
        } else {
            throw new UnknownUiException("Unable to find dialer settings button");
        }
    }

    /** This method opens the Favorites tab. */
    private void openFavorites() {
        BySelector favoritesMenuSelector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.FAVORITES_VIEW,
                                        AutoConfigConstants.FAVORITES_MENU));
        UiObject2 favoritesMenuButton = findUiObject(favoritesMenuSelector);
        if (favoritesMenuButton != null) {
            clickAndWaitForIdleScreen(favoritesMenuButton);
        } else {
            throw new UnknownUiException("Unable to find Favorites menu.");
        }
    }

    public boolean isPhonePaired() {
        return mBluetoothAdapter.getBondedDevices().size() != 0;
    }

    /**
     * This method is used to scroll through the list (Favorites, Call History, Contact)
     *
     * <p>in search of the contact number or name and click if found
     *
     * @param contact contact number or name to be dialed
     */
    private void scrollThroughCallList(String contact) {
        UiObject2 contactObject =
                scrollAndFindUiObject(By.text(Pattern.compile(contact, Pattern.CASE_INSENSITIVE)));
        if (contactObject != null) {
            clickAndWaitForIdleScreen(contactObject);
        }
    }

    /**
     * This method is used to enter phonenumber from the on-screen numberpad
     *
     * @param phoneNumber number to be dialed
     */
    private void enterNumber(String phoneNumber) {
        if (phoneNumber == null) {
            throw new UnknownUiException("No phone number provided");
        }
        pressHome();
        waitForIdle();
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_DIAL_PAD_COMMAND));
        UiObject2 dial_pad =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.DIAL_PAD_VIEW,
                                AutoConfigConstants.DIAL_PAD_FRAGMENT));
        if (dial_pad == null) {
            throw new UnknownUiException("Unable to find dial pad");
        }
        char[] array = phoneNumber.toCharArray();
        for (char ch : array) {
            UiObject2 numberButton = findUiObject(By.text(Character.toString(ch)));
            if (numberButton == null) {
                throw new UnknownUiException("Unable to find number" + phoneNumber);
            }
            clickAndWaitForIdleScreen(numberButton);
        }
    }
}
