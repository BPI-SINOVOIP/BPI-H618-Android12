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

package com.android.car.dialer.inject;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.SharedPreferences;

import androidx.fragment.app.Fragment;
import androidx.preference.PreferenceManager;

import com.android.car.dialer.ui.common.DialerBaseFragment;
import com.android.car.dialer.ui.common.OnItemClickedListener;
import com.android.car.dialer.ui.contact.ContactDetailsFragment;
import com.android.car.telephony.common.Contact;

import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.components.FragmentComponent;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;

/** Dialer modules. */
public final class DialerModules {

    /** Application level module. */
    @InstallIn(SingletonComponent.class)
    @Module
    public static final class AppModule {

        @Provides
        static BluetoothAdapter provideBluetoothAdapter() {
            return BluetoothAdapter.getDefaultAdapter();
        }

        @Singleton
        @Provides
        static SharedPreferences provideSharedPreferences(@ApplicationContext Context context) {
            return PreferenceManager.getDefaultSharedPreferences(context);
        }
    }

    /** Module providing dependencies for fragments. */
    @InstallIn(FragmentComponent.class)
    @Module
    public static final class FragmentModule {
        @Provides
        static OnItemClickedListener<Contact> provideShowContactDetailListener(Fragment fragment) {
            return contact -> {
                Fragment contactDetailsFragment = ContactDetailsFragment.newInstance(contact);
                if (fragment instanceof DialerBaseFragment) {
                    ((DialerBaseFragment) fragment).pushContentFragment(contactDetailsFragment,
                            ContactDetailsFragment.FRAGMENT_TAG);
                }
            };
        }
    }

    /** Do not initialize. */
    private DialerModules() {
    }
}
