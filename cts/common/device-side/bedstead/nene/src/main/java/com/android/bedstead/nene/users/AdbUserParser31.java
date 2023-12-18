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

package com.android.bedstead.nene.users;

import android.os.Build;

import androidx.annotation.RequiresApi;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.AdbParseException;

/**
 * Parser for "adb dumpsys user" on Android 30+
 *
 * <p>Example output:
 * {@code
 *
 * }
 */
@RequiresApi(Build.VERSION_CODES.S)
// TODO(scottjonathan): Replace ADB calls for S with test apis
public class AdbUserParser31 extends AdbUserParser30 {

    AdbUserParser31(TestApis testApis) {
        super(testApis);
    }

    @Override
    User.MutableUser parseUser(String userString) throws AdbParseException {
        User.MutableUser user = super.parseUser(userString);

        if (user.mType.baseType().contains(UserType.BaseType.PROFILE)) {
            try {
                user.mParent = mTestApis.users().find(
                        Integer.parseInt(userString.split("parentId=")[1].split("[ \n]")[0]));
            } catch (IndexOutOfBoundsException e) {
                throw new AdbParseException("Error parsing user", userString, e);
            }
        }

        return user;
    }
}
