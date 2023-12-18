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

import com.android.bedstead.nene.TestApis;

/**
 * Default implementation of {@link UserReference}.
 *
 * <p>Represents the abstract idea of a {@link User}, which may or may not exist.
 */
public final class UnresolvedUser extends UserReference {
    UnresolvedUser(TestApis testApis, int id) {
        super(testApis, id);
    }

    @Override
    public String toString() {
        return "UnresolvedUser{id=" + id() + "}";
    }
}
