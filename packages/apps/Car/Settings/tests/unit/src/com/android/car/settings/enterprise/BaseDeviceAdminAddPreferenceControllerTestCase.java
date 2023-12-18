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
package com.android.car.settings.enterprise;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

abstract class BaseDeviceAdminAddPreferenceControllerTestCase<T extends
        BaseDeviceAdminAddPreferenceController<?>> extends BasePreferenceControllerTestCase {

    protected final void verifyPreferenceTitleNeverSet() {
        verify(mPreference, never()).setTitle(any());
    }

    protected final void verifyPreferenceTitleSet(CharSequence title) {
        verify(mPreference).setTitle(title);
    }

    protected final void verifyPreferenceTitleSet(int resId) {
        verify(mPreference).setTitle(resId);
    }

    protected final void verifyPreferenceSummarySet(CharSequence title) {
        verify(mPreference).setSummary(title);
    }

    protected final void verifyPreferenceSummaryNeverSet() {
        verify(mPreference, never()).setSummary(any());
    }

    protected final void verifyPreferenceIconSet() {
        verify(mPreference).setIcon(notNull());
    }

    protected final void verifyPreferenceIconNeverSet() {
        verify(mPreference, never()).setIcon(notNull());
    }

    protected final void verifyPreferenceDisabled() {
        verify(mPreference).setEnabled(false);
    }

    protected final void verifyPreferenceEnabled() {
        verify(mPreference).setEnabled(true);
    }
}
