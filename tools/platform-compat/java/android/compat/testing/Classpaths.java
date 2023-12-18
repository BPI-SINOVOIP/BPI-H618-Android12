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

package android.compat.testing;

import static com.google.common.truth.Truth.assertThat;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.INativeDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;

import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.dexbacked.DexBackedDexFile;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.MultiDexContainer;

import java.io.File;
import java.io.IOException;
import java.util.Objects;

/**
 * Testing utilities for parsing *CLASSPATH environ variables on a test device.
 */
public final class Classpaths {

    private Classpaths() {
    }

    public enum ClasspathType {
        BOOTCLASSPATH,
        DEX2OATBOOTCLASSPATH,
        SYSTEMSERVERCLASSPATH,
    }

    /** Returns on device filepaths to the jars that are part of a given classpath. */
    public static ImmutableList<String> getJarsOnClasspath(INativeDevice device,
            ClasspathType classpath) throws DeviceNotAvailableException {
        CommandResult shellResult = device.executeShellV2Command("echo $" + classpath);
        assertThat(shellResult.getStatus()).isEqualTo(CommandStatus.SUCCESS);
        assertThat(shellResult.getExitCode()).isEqualTo(0);

        String value = shellResult.getStdout().trim();
        assertThat(value).isNotEmpty();
        return ImmutableList.copyOf(value.split(":"));
    }

    /** Returns classes defined a given jar file on the test device. */
    public static ImmutableSet<ClassDef> getClassDefsFromJar(INativeDevice device,
            String remoteJarPath) throws DeviceNotAvailableException, IOException {
        File jar = null;
        try {
            jar = device.pullFile(remoteJarPath);
            if (jar == null) {
                throw new IllegalStateException("could not pull remote file " + remoteJarPath);
            }
            MultiDexContainer<? extends DexBackedDexFile> container =
                    DexFileFactory.loadDexContainer(jar, Opcodes.getDefault());
            ImmutableSet.Builder<ClassDef> set = ImmutableSet.builder();
            for (String dexName : container.getDexEntryNames()) {
                set.addAll(Objects.requireNonNull(container.getEntry(dexName)).getClasses());
            }
            return set.build();
        } finally {
            FileUtil.deleteFile(jar);
        }
    }

}
