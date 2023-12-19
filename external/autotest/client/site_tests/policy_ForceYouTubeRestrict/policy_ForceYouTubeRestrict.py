# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base

class policy_ForceYouTubeRestrict(
        enterprise_policy_base.EnterprisePolicyTest):
    """
    Tests the ForceYouTubeRestrict policy in Chrome OS.

    If the policy is set to strict than the user will not be able to view any
    restricted videos on YouTube. If the policy is set to moderate than the
    user will not be able to watch some restricted videos. In both cases
    the user will not be able to toggle restricted settings on the website.

    Note: This test doesn't cover the ARC++ app.

    """
    version = 1

    POLICY_NAME = 'ForceYouTubeRestrict'
    TEST_CASES = {
        'Strict': 2,
        'Moderate': 1,
        'Disabled': 0,
        'NotSet': None}

    def _search_for_adult_content(self):
        SEARCH_QUERY = 'https://www.youtube.com/results?search_query=adult'
        BURGER_MENU = (
                "document.querySelector('* /deep/ #masthead-container /deep/"
                " #end /deep/ ytd-topbar-menu-button-renderer:last-of-type').innerHTML;"
        )
        self.search_tab = self.navigate_to_url(SEARCH_QUERY)
        utils.poll_for_condition(lambda: self.check_page_readiness(
                self.search_tab, BURGER_MENU),
                                 exception=error.TestFail(
                                         'Page is not ready.'),
                                 timeout=5,
                                 sleep_interval=1)

    def _open_restricted_menu(self):
        BURGER_MENU_CLICK = (
                "document.querySelector('* /deep/ #masthead-container /de"
                "ep/ #end /deep/ ytd-topbar-menu-button-renderer:last-of-type').click();"
        )
        RESTRICTED_MENU_CLICK = """
buttons=document.querySelectorAll('a#endpoint.yt-simple-endpoint.style-scope.ytd-compact-link-renderer');
for (let i = 0; i < buttons.length; i++) {
  button=buttons[i];
  if (button.innerText.startsWith("Restricted Mode:")) {
     button.click();
     break;
  }
}
"""
        self.search_tab.EvaluateJavaScript(BURGER_MENU_CLICK)
        time.sleep(1)
        self.search_tab.EvaluateJavaScript(RESTRICTED_MENU_CLICK)
        time.sleep(1)

    def _restricted_mode_by_policy(self):
        RESTRICTED_MODE_SELECTOR = "document.querySelector('ytd-text-header-renderer.style-scope.ytd-section-list-renderer').innerText"
        return self.search_tab.EvaluateJavaScript(RESTRICTED_MODE_SELECTOR)

    def _restricted_mode_by_policy_strict(self):
        return "Restricted Mode is enabled by your network administrator" in \
           self._restricted_mode_by_policy()

    def _restricted_mode_by_policy_moderate(self):
        return self._restricted_mode_by_policy_strict()

    def _get_content(self, restriction_type_url):
        """
        Checks the contents of the watch page.

        @param restriction_type: URL with either strict or moderate content.

        @returns text content of the element with video status.

        """
        VERIFY_VIDEO_NOT_LOADED_CMD = ("document.getElementById"
                                       "('error-screen').innerText;")
        active_tab = self.navigate_to_url(restriction_type_url)
        utils.poll_for_condition(
            lambda: self.check_page_readiness(
                active_tab, VERIFY_VIDEO_NOT_LOADED_CMD),
            exception=error.TestFail('Page is not ready.'),
            timeout=5,
            sleep_interval=1)
        return active_tab.EvaluateJavaScript(VERIFY_VIDEO_NOT_LOADED_CMD)

    def _strict_content(self):
        RESTRICTED_ONLY_ON_STRICT = 'https://www.youtube.com/watch?v=Fmwfmee2ZTE'
        return self._get_content(RESTRICTED_ONLY_ON_STRICT)

    def _moderated_content(self):
        RESTRICTED_ON_MODERATE = 'https://www.youtube.com/watch?v=yR79oLrI1g4'
        return self._get_content(RESTRICTED_ON_MODERATE)

    def _check_restricted_mode(self, case):
        """
        Checks restricted settings by verifying that user is unable to play
        certain videos as well as toggle restricted settings.

        @param case: policy value expected.

        """
        # Navigates to the search page to search for adult content
        self._search_for_adult_content()
        # We could check for the status shown in restricted menu but
        # unfortunately this is broken for the policy and therefore
        # doesn't add value to the test
        #self._open_restricted_menu()

        if case == 'Strict':
            if 'restricted' in self._strict_content() \
               and self._restricted_mode_by_policy_strict():
                return True
            raise error.TestFail(
                    "Restricted mode is not on, user can view restricted video."
            )
        elif case == 'Moderate':
            if 'restricted' in self._moderated_content() \
               and self._restricted_mode_by_policy_moderate():
                return True
            raise error.TestFail(
                    "Restricted mode is not on, user can view restricted video."
            )
        else:
            return True


    def run_once(self, case):
        """
        Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        self.POLICIES = {self.POLICY_NAME: self.TEST_CASES[case]}
        self.setup_case(user_policies=self.POLICIES)
        self._check_restricted_mode(case)
