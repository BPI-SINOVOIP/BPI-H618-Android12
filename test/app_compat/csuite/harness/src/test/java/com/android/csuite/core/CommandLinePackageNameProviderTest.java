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

package com.android.csuite.core;

import static com.google.common.truth.Truth.assertThat;

import com.android.tradefed.config.OptionSetter;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Set;

@RunWith(JUnit4.class)
public final class CommandLinePackageNameProviderTest {

    @Test
    public void get_packageNamesProvided_returnsPackageNames() throws Exception {
        CommandLinePackageNameProvider provider = new CommandLinePackageNameProvider();
        String package1 = "package.name1";
        String package2 = "package.name2";
        OptionSetter optionSetter = new OptionSetter(provider);
        optionSetter.setOptionValue(CommandLinePackageNameProvider.PACKAGE, package1);
        optionSetter.setOptionValue(CommandLinePackageNameProvider.PACKAGE, package2);

        Set<String> packageNames = provider.get();

        assertThat(packageNames).containsExactly(package1, package2);
    }
}
