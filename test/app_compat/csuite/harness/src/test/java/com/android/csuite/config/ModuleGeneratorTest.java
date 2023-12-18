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

package com.android.csuite.config;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import com.android.csuite.core.PackageNameProvider;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.invoker.TestInformation;

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.ListMultimap;
import com.google.common.jimfs.Jimfs;
import com.google.common.truth.IterableSubject;
import com.google.common.truth.StringSubject;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.FileSystem;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

@RunWith(JUnit4.class)
public final class ModuleGeneratorTest {
    private static final String TEST_PACKAGE_NAME1 = "test.package.name1";
    private static final String TEST_PACKAGE_NAME2 = "test.package.name2";
    private static final String PACKAGE_PLACEHOLDER = "{package}";
    private static final Exception NO_EXCEPTION = null;

    private final FileSystem mFileSystem =
            Jimfs.newFileSystem(com.google.common.jimfs.Configuration.unix());

    @Test
    public void tearDown_packageNamesProvided_deletesGeneratedModules() throws Exception {
        Path testsDir = createTestsDir();
        ModuleGenerator generator1 =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackage(TEST_PACKAGE_NAME1)
                        .addPackage(TEST_PACKAGE_NAME2)
                        .build();
        generator1.split();
        ModuleGenerator generator2 =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackage(TEST_PACKAGE_NAME1)
                        .addPackage(TEST_PACKAGE_NAME2)
                        .build();

        generator2.tearDown(createTestInfo(), NO_EXCEPTION);

        assertThatListDirectory(testsDir).isEmpty();
    }

    @Test
    public void tearDown_packageNamesNotProvided_doesNotThrowError() throws Exception {
        ModuleGenerator generator = createGeneratorBuilder().setTestsDir(createTestsDir()).build();
        generator.split();

        generator.tearDown(createTestInfo(), NO_EXCEPTION);
    }

    @Test
    public void split_packageNameIsEmptyString_throwsError() throws Exception {
        ModuleGenerator generator = createGeneratorBuilder().addPackage("").build();

        assertThrows(IllegalArgumentException.class, () -> generator.split());
    }

    @Test
    public void split_packageNameContainsPlaceholder_throwsError() throws Exception {
        ModuleGenerator generator =
                createGeneratorBuilder().addPackage("a" + PACKAGE_PLACEHOLDER + "b").build();

        assertThrows(IllegalArgumentException.class, () -> generator.split());
    }

    @Test
    public void split_multiplePackageNameProviders_generateModulesForAll() throws Exception {
        Path testsDir = createTestsDir();
        ModuleGenerator generator =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackageNameProvider(() -> ImmutableSet.of(TEST_PACKAGE_NAME1))
                        // Simulates package providers providing duplicated package names.
                        .addPackageNameProvider(() -> ImmutableSet.of(TEST_PACKAGE_NAME1))
                        .addPackageNameProvider(() -> ImmutableSet.of(TEST_PACKAGE_NAME2))
                        .build();

        generator.split();

        assertThatListDirectory(testsDir)
                .containsExactly(
                        getModuleConfigFile(testsDir, TEST_PACKAGE_NAME1),
                        getModuleConfigFile(testsDir, TEST_PACKAGE_NAME2));
    }

    @Test
    public void split_packageNameProviderThrowsException_throwsException() throws Exception {
        Path testsDir = createTestsDir();
        ModuleGenerator generator =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackageNameProvider(
                                () -> {
                                    throw new IOException();
                                })
                        .build();

        assertThrows(UncheckedIOException.class, () -> generator.split());
    }

    @Test
    public void split_packageNamesNotProvided_doesNotGenerate() throws Exception {
        Path testsDir = createTestsDir();
        ModuleGenerator generator = createGeneratorBuilder().setTestsDir(testsDir).build();

        generator.split();

        assertThatListDirectory(testsDir).isEmpty();
    }

    @Test
    public void split_templateContainsPlaceholders_replacesPlaceholdersInOutput() throws Exception {
        Path testsDir = createTestsDir();
        String content = "hello placeholder%s%s world";
        ModuleGenerator generator =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackage(TEST_PACKAGE_NAME1)
                        .addPackage(TEST_PACKAGE_NAME2)
                        .setTemplateContent(
                                String.format(content, PACKAGE_PLACEHOLDER, PACKAGE_PLACEHOLDER))
                        .build();

        generator.split();

        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME1)
                .isEqualTo(String.format(content, TEST_PACKAGE_NAME1, TEST_PACKAGE_NAME1));
        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME2)
                .isEqualTo(String.format(content, TEST_PACKAGE_NAME2, TEST_PACKAGE_NAME2));
    }

    @Test
    public void split_templateDoesNotContainPlaceholder_outputsTemplateContent() throws Exception {
        Path testsDir = createTestsDir();
        String content = "no placeholder";
        ModuleGenerator generator =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackage(TEST_PACKAGE_NAME1)
                        .addPackage(TEST_PACKAGE_NAME2)
                        .setTemplateContent(content)
                        .build();

        generator.split();

        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME1).isEqualTo(content);
        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME2).isEqualTo(content);
    }

    @Test
    public void split_templateContentIsEmpty_outputsTemplateContent() throws Exception {
        Path testsDir = createTestsDir();
        String content = "";
        ModuleGenerator generator =
                createGeneratorBuilder()
                        .setTestsDir(testsDir)
                        .addPackage(TEST_PACKAGE_NAME1)
                        .addPackage(TEST_PACKAGE_NAME2)
                        .setTemplateContent(content)
                        .build();

        generator.split();

        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME1).isEqualTo(content);
        assertThatModuleConfigFileContent(testsDir, TEST_PACKAGE_NAME2).isEqualTo(content);
    }

    private static StringSubject assertThatModuleConfigFileContent(
            Path testsDir, String packageName) throws IOException {
        return assertThat(
                new String(Files.readAllBytes(getModuleConfigFile(testsDir, packageName))));
    }

    private static IterableSubject assertThatListDirectory(Path dir) throws IOException {
        // Convert stream to list because com.google.common.truth.Truth8 is not available.
        return assertThat(
                Files.walk(dir)
                        .filter(p -> !p.equals(dir))
                        .collect(ImmutableList.toImmutableList()));
    }

    private static Path getModuleConfigFile(Path baseDir, String packageName) {
        return baseDir.resolve(packageName + ".config");
    }

    private Path createTestsDir() throws IOException {
        Path rootPath = mFileSystem.getPath("csuite");
        Files.createDirectories(rootPath);
        return Files.createTempDirectory(rootPath, "testDir");
    }

    private static TestInformation createTestInfo() {
        IInvocationContext context = new InvocationContext();
        context.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        context.addDeviceBuildInfo("device1", new BuildInfo());
        return TestInformation.newBuilder().setInvocationContext(context).build();
    }

    private GeneratorBuilder createGeneratorBuilder() throws IOException {
        return new GeneratorBuilder()
                .setFileSystem(mFileSystem)
                .setTemplateContent(MODULE_TEMPLATE_CONTENT)
                .setOption(ModuleGenerator.OPTION_TEMPLATE, "empty_path");
    }

    private static final class GeneratorBuilder {
        private final ListMultimap<String, String> mOptions = ArrayListMultimap.create();
        private final Set<String> mPackages = new HashSet<>();
        private final List<PackageNameProvider> mPackageNameProviders = new ArrayList<>();
        private Path mTestsDir;
        private String mTemplateContent;
        private FileSystem mFileSystem;

        GeneratorBuilder addPackage(String packageName) {
            mPackages.add(packageName);
            return this;
        }

        GeneratorBuilder addPackageNameProvider(PackageNameProvider packageNameProvider) {
            mPackageNameProviders.add(packageNameProvider);
            return this;
        }

        GeneratorBuilder setFileSystem(FileSystem fileSystem) {
            mFileSystem = fileSystem;
            return this;
        }

        GeneratorBuilder setTemplateContent(String templateContent) {
            mTemplateContent = templateContent;
            return this;
        }

        GeneratorBuilder setTestsDir(Path testsDir) {
            mTestsDir = testsDir;
            return this;
        }

        GeneratorBuilder setOption(String key, String value) {
            mOptions.put(key, value);
            return this;
        }

        ModuleGenerator build() throws Exception {
            ModuleGenerator generator =
                    new ModuleGenerator(
                            mFileSystem, buildInfo -> mTestsDir, resourcePath -> mTemplateContent);

            OptionSetter optionSetter = new OptionSetter(generator);
            for (Map.Entry<String, String> entry : mOptions.entries()) {
                optionSetter.setOptionValue(entry.getKey(), entry.getValue());
            }

            List<PackageNameProvider> packageNameProviders = new ArrayList<>(mPackageNameProviders);
            packageNameProviders.add(() -> mPackages);

            IConfiguration configuration = new Configuration("name", "description");
            configuration.setConfigurationObjectList(
                    ModuleGenerator.PACKAGE_NAME_PROVIDER, packageNameProviders);
            generator.setConfiguration(configuration);

            return generator;
        }
    }

    private static final String MODULE_TEMPLATE_CONTENT =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                    + "<configuration description=\"description\">\n"
                    + "    <option name=\"package-name\" value=\"{package}\"/>\n"
                    + "    <target_generator class=\"some.generator.class\">\n"
                    + "        <option name=\"test-file-name\" value=\"app://{package}\"/>\n"
                    + "    </target_generator>\n"
                    + "    <test class=\"some.test.class\"/>\n"
                    + "</configuration>";
}
