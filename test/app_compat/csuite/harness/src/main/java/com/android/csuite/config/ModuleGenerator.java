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

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.csuite.core.PackageNameProvider;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.invoker.TestInformation;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.io.Resources;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * A tool for generating TradeFed suite modules during runtime.
 *
 * <p>This class generates module config files into TradeFed's test directory at runtime using a
 * template. Since the content of the test directory relies on what is being generated in a test
 * run, there can only be one instance executing at a given time.
 *
 * <p>The intention of this class is to generate test modules at the beginning of a test run and
 * cleans up after all tests finish, which resembles a target preparer. However, a target preparer
 * is executed after the sharding process has finished. The only way to make the generated modules
 * available for sharding without making changes to TradeFed's core code is to disguise this module
 * generator as an instance of IShardableTest and declare it separately in test plan config. This is
 * hacky, and in the long term a TradeFed centered solution is desired. For more details, see
 * go/sharding-hack-for-module-gen. Note that since the generate step is executed as a test instance
 * and cleanup step is executed as a target preparer, there should be no saved states between
 * generating and cleaning up module files.
 *
 * <p>This module generator collects package names from all PackageNameProvider objects specified in
 * the test configs.
 *
 * <h2>Syntax and usage</h2>
 *
 * <p>References to package name providers in TradeFed test configs must have the following syntax:
 *
 * <blockquote>
 *
 * <b>&lt;object type="PACKAGE_NAME_PROVIDER" class="</b><i>provider_class_name</i><b>"/&gt;</b>
 *
 * </blockquote>
 *
 * where <i>provider_class_name</i> is the fully-qualified class name of an PackageNameProvider
 * implementation class.
 */
public final class ModuleGenerator
        implements IRemoteTest,
                IShardableTest,
                IBuildReceiver,
                ITargetPreparer,
                IConfigurationReceiver {

    @VisibleForTesting static final String MODULE_FILE_EXTENSION = ".config";
    @VisibleForTesting static final String OPTION_TEMPLATE = "template";
    @VisibleForTesting static final String PACKAGE_NAME_PROVIDER = "PACKAGE_NAME_PROVIDER";
    private static final String TEMPLATE_PACKAGE_PATTERN = "\\{package\\}";
    private static final Collection<IRemoteTest> NOT_SPLITABLE = null;

    @Option(
            name = OPTION_TEMPLATE,
            description = "Module config template resource path.",
            importance = Importance.ALWAYS)
    private String mTemplate;

    private final TestDirectoryProvider mTestDirectoryProvider;
    private final ResourceLoader mResourceLoader;
    private final FileSystem mFileSystem;
    private IBuildInfo mBuildInfo;
    private IConfiguration mConfiguration;

    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfiguration = configuration;
    }

    public ModuleGenerator() {
        this(FileSystems.getDefault());
    }

    private ModuleGenerator(FileSystem fileSystem) {
        this(
                fileSystem,
                new CompatibilityTestDirectoryProvider(fileSystem),
                new ClassResourceLoader());
    }

    @VisibleForTesting
    ModuleGenerator(
            FileSystem fileSystem,
            TestDirectoryProvider testDirectoryProvider,
            ResourceLoader resourceLoader) {
        mFileSystem = fileSystem;
        mTestDirectoryProvider = testDirectoryProvider;
        mResourceLoader = resourceLoader;
    }

    @Override
    public void run(final TestInformation testInfo, final ITestInvocationListener listener) {
        // Intentionally left blank since this class is not really a test.
    }

    @Override
    public void setUp(TestInformation testInfo) {
        // Intentionally left blank.
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * Generates test modules. Note that the implementation of this method is not related to
     * sharding in any way.
     */
    @Override
    public Collection<IRemoteTest> split() {
        try {
            // Executes the generate step.
            generateModules();
        } catch (IOException e) {
            throw new UncheckedIOException("Failed to generate modules", e);
        }

        return NOT_SPLITABLE;
    }

    /** Cleans up generated test modules. */
    @Override
    public void tearDown(TestInformation testInfo, Throwable e) throws DeviceNotAvailableException {
        // Gets build info from test info as when the class is executed as a ITargetPreparer
        // preparer, it is not considered as a IBuildReceiver instance.
        mBuildInfo = testInfo.getBuildInfo();

        try {
            // Executes the clean up step.
            cleanUpModules();
        } catch (IOException ioException) {
            throw new UncheckedIOException("Failed to clean up generated modules", ioException);
        }
    }

    private Set<String> getPackageNames() throws IOException {
        Set<String> packages = new HashSet<>();
        for (Object provider : mConfiguration.getConfigurationObjectList(PACKAGE_NAME_PROVIDER)) {
            packages.addAll(((PackageNameProvider) provider).get());
        }
        return packages;
    }

    private void generateModules() throws IOException {
        String templateContent = mResourceLoader.load(mTemplate);

        for (String packageName : getPackageNames()) {
            validatePackageName(packageName);
            Files.write(
                    getModulePath(packageName),
                    templateContent.replaceAll(TEMPLATE_PACKAGE_PATTERN, packageName).getBytes());
        }
    }

    private void cleanUpModules() throws IOException {
        getPackageNames()
                .forEach(
                        packageName -> {
                            try {
                                Files.delete(getModulePath(packageName));
                            } catch (IOException ioException) {
                                CLog.e(
                                        "Failed to delete the generated module for package "
                                                + packageName,
                                        ioException);
                            }
                        });
    }

    private Path getModulePath(String packageName) throws IOException {
        Path testsDir = mTestDirectoryProvider.get(mBuildInfo);
        return testsDir.resolve(packageName + MODULE_FILE_EXTENSION);
    }

    private static void validatePackageName(String packageName) {
        if (packageName.isEmpty() || packageName.matches(".*" + TEMPLATE_PACKAGE_PATTERN + ".*")) {
            throw new IllegalArgumentException(
                    "Package name cannot be empty or contains package placeholder: "
                            + TEMPLATE_PACKAGE_PATTERN);
        }
    }

    @VisibleForTesting
    interface ResourceLoader {
        String load(String resourceName) throws IOException;
    }

    private static final class ClassResourceLoader implements ResourceLoader {
        @Override
        public String load(String resourceName) throws IOException {
            return Resources.toString(
                    getClass().getClassLoader().getResource(resourceName), StandardCharsets.UTF_8);
        }
    }

    @VisibleForTesting
    interface TestDirectoryProvider {
        Path get(IBuildInfo buildInfo) throws IOException;
    }

    private static final class CompatibilityTestDirectoryProvider implements TestDirectoryProvider {
        private final FileSystem mFileSystem;

        private CompatibilityTestDirectoryProvider(FileSystem fileSystem) {
            mFileSystem = fileSystem;
        }

        @Override
        public Path get(IBuildInfo buildInfo) throws IOException {
            return mFileSystem.getPath(
                    new CompatibilityBuildHelper(buildInfo).getTestsDir().getPath());
        }
    }
}
