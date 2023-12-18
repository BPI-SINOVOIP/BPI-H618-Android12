/*
 * Copyright (C) 2020 The Android Open Source Project
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

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Set;

@RunWith(JUnit4.class)
public final class FileBasedPackageNameProviderTest {
    private static final String TEST_PACKAGE_NAME1 = "test.package.name1";
    private static final String TEST_PACKAGE_NAME2 = "test.package.name2";
    private static final String PACKAGE_PLACEHOLDER = "{package}";
    private static final Exception NO_EXCEPTION = null;

    @Rule public final TemporaryFolder tempFolder = new TemporaryFolder();

    @Test
    public void get_fileNotSpecified_returnsEmptySet() throws Exception {
        FileBasedPackageNameProvider provider = createProvider();

        Set<String> packageNames = provider.get();

        assertThat(packageNames).isEmpty();
    }

    @Test
    public void get_multipleFileSpecified_returnsAllEntries() throws Exception {
        String packageName1 = "a";
        String packageName2 = "b";
        String packageName3 = "c";
        String packageName4 = "d";
        FileBasedPackageNameProvider provider =
                createProvider(
                        createPackagesFile(packageName1 + "\n" + packageName2),
                        createPackagesFile(packageName3 + "\n" + packageName4));

        Set<String> packageNames = provider.get();

        assertThat(packageNames)
                .containsExactly(packageName1, packageName2, packageName3, packageName4);
    }

    @Test
    public void get_fileContainsEmptyLines_ignoresEmptyLines() throws Exception {
        String packageName1 = "a";
        String packageName2 = "b";
        FileBasedPackageNameProvider provider =
                createProvider(createPackagesFile(packageName1 + "\n \n\n" + packageName2));

        Set<String> packageNames = provider.get();

        assertThat(packageNames).containsExactly(packageName1, packageName2);
    }

    @Test
    public void get_fileContainsCommentLines_ignoresCommentLines() throws Exception {
        String packageName1 = "a";
        String packageName2 = "b";
        FileBasedPackageNameProvider provider =
                createProvider(
                        createPackagesFile(
                                packageName1
                                        + "\n"
                                        + FileBasedPackageNameProvider.COMMENT_LINE_PREFIX
                                        + " Some comments\n"
                                        + packageName2));

        Set<String> packageNames = provider.get();

        assertThat(packageNames).containsExactly(packageName1, packageName2);
    }

    private FileBasedPackageNameProvider createProvider(Path... packagesFiles)
            throws IOException, ConfigurationException {
        FileBasedPackageNameProvider provider = new FileBasedPackageNameProvider();
        OptionSetter optionSetter = new OptionSetter(provider);
        for (Path packagesFile : packagesFiles) {
            optionSetter.setOptionValue(
                    FileBasedPackageNameProvider.PACKAGES_FILE, packagesFile.toString());
        }
        return provider;
    }

    private Path createPackagesFile(String content) throws IOException {
        Path tempFile = Files.createTempFile(tempFolder.getRoot().toPath(), "packages", ".txt");
        Files.write(tempFile, content.getBytes());
        return tempFile;
    }
}
