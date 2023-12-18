/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.ndkports

import java.io.File

@Suppress("unused")
fun executeProcessStep(
    args: List<String>,
    workingDirectory: File,
    additionalEnvironment: Map<String, String>? = null
): Result<Unit, String> {
    val pb = ProcessBuilder(args)
        .redirectOutput(ProcessBuilder.Redirect.INHERIT)
        .redirectError(ProcessBuilder.Redirect.INHERIT)
        .directory(workingDirectory)

    if (additionalEnvironment != null) {
        pb.environment().putAll(additionalEnvironment)
    }

    return pb.start()
        .waitFor().let {
            if (it == 0) {
                Result.Ok(Unit)
            } else {
                Result.Error("Process failed with exit code $it")
            }
        }
}

fun installDirectoryForPort(
    name: String,
    workingDirectory: File,
    toolchain: Toolchain
): File = workingDirectory.parentFile.resolve("$name/install/${toolchain.abi}")

/**
 * A module exported by the package.
 *
 * As currently implemented by ndkports, one module is exactly one library.
 * Prefab supports header-only libraries, but ndkports does not support these
 * yet.
 *
 * Static libraries are not currently supported by ndkports.
 *
 * @property[name] The name of the module. Note that currently the name of the
 * installed library file must be exactly `lib$name.so`.
 * @property[includesPerAbi] Set to true if a different set of headers should be
 * exposed per-ABI. Not currently implemented.
 * @property[dependencies] A list of other modules required by this module, in
 * the format described by https://google.github.io/prefab/.
 */
data class Module(
    val name: String,
    val includesPerAbi: Boolean = false,
    val dependencies: List<String> = emptyList()
)

/**
 * The base class for all ports.
 */
abstract class Port {
    /**
     * The name of the port. Will be used as the package name in prefab.json.
     */
    abstract val name: String

    /**
     * The version of the package.
     *
     * Used as the default for [prefabVersion] and [mavenVersion] when
     * appropriate.
     */
    abstract val version: String

    /**
     * The version to encode in the prefab.json.
     *
     * This version must be compatible with CMake's `find_package` for
     * config-style packages. This means that it must be one to four decimal
     * separated integers. No other format is allowed.
     *
     * If not set, the default is [version] as interpreted by
     * [CMakeCompatibleVersion.parse].
     *
     * For example, OpenSSL 1.1.1g will set this value to
     * `CMakeCompatibleVersion(1, 1, 1, 7)`.
     */
    open val prefabVersion: CMakeCompatibleVersion
        get() = CMakeCompatibleVersion.parse(version)

    /**
     * The version to use for the maven package.
     *
     * This field allows versioning the maven package differently from the
     * package itself, which is sometimes necessary given CMake's strict version
     * format requirements.
     *
     * If not set, the default is [version].
     *
     * For example, this could be set to `"$name-$version-alpha-1"` to
     * distribute an alpha of the package.
     */
    open val mavenVersion: String
        get() = version

    /**
     * The relative path to the license file of this package.
     *
     * This file will be packaged in the AAR in the META-INF directory.
     */
    open val licensePath: String = "LICENSE"

    /**
     * A description of the license (name and URL) for use in the pom.xml.
     */
    abstract val license: License

    /**
     * A list of dependencies for this package.
     *
     * For example, curl depends on `listOf("openssl")`.
     */
    open val dependencies: List<String> = emptyList()

    /**
     * A list of modules exported by this package.
     */
    abstract val modules: List<Module>

    /**
     * The number of CPUs available for building.
     *
     * May be passed to the build system if required.
     */
    protected val ncpus = Runtime.getRuntime().availableProcessors()

    fun run(
        toolchain: Toolchain,
        sourceDirectory: File,
        buildDirectory: File,
        installDirectory: File,
        workingDirectory: File
    ): Result<Unit, String> {
        configure(
            toolchain,
            sourceDirectory,
            buildDirectory,
            installDirectory,
            workingDirectory
        ).onFailure { return Result.Error(it) }

        build(toolchain, buildDirectory).onFailure { return Result.Error(it) }

        install(
            toolchain,
            buildDirectory,
            installDirectory
        ).onFailure { return Result.Error(it) }

        return Result.Ok(Unit)
    }

    /**
     * Overridable build step for extracting the source package.
     *
     * @param[sourceTarball] The path to the source tarball.
     * @param[sourceDirectory] The destination directory for the extracted
     * source.
     * @param[workingDirectory] The working top-level directory for this
     * package.
     * @return A [Result<Unit, String>][Result] describing the result of the
     * operation. On failure, [Result.Error<String>][Result.Error] containing an
     * error message is returned.
     */
    open fun extractSource(
        sourceTarball: File,
        sourceDirectory: File,
        workingDirectory: File
    ): Result<Unit, String> {
        sourceDirectory.mkdirs()
        return executeProcessStep(
            listOf(
                "tar",
                "xf",
                sourceTarball.absolutePath,
                "--strip-components=1"
            ), sourceDirectory
        )
    }

    /**
     * Overridable build step for configuring the build.
     *
     * Any pre-build steps should be run here.
     *
     * In an autoconf build, this runs `configure`.
     *
     * @param[toolchain] The toolchain used for this build.
     * @param[sourceDirectory] The directory containing the extracted package
     * source.
     * @param[buildDirectory] The directory to use for building.
     * @param[installDirectory] The destination directory for this package's
     * installed headers and libraries.
     * @param[workingDirectory] The top-level working directory for this
     * package.
     * @return A [Result<Unit, String>][Result] describing the result of the
     * operation. On failure, [Result.Error<String>][Result.Error] containing an
     * error message is returned.
     */
    open fun configure(
        toolchain: Toolchain,
        sourceDirectory: File,
        buildDirectory: File,
        installDirectory: File,
        workingDirectory: File
    ): Result<Unit, String> = Result.Ok(Unit)

    /**
     * Overridable build step for building the package.
     *
     * In an autoconf build, this runs `make`.
     *
     * @param[toolchain] The toolchain used for this build.
     * @param[buildDirectory] The directory to use for building.
     * @return A [Result<Unit, String>][Result] describing the result of the
     * operation. On failure, [Result.Error<String>][Result.Error] containing an
     * error message is returned.
     */
    open fun build(
        toolchain: Toolchain,
        buildDirectory: File
    ): Result<Unit, String> = Result.Ok(Unit)

    /**
     * Overridable build step for installing built artifacts for packaging.
     *
     * The install step is expected to install headers and libraries to the
     * [installDirectory] with the following layout:
     *
     * [installDirectory]
     *     include/
     *         <package headers>
     *     lib/
     *         <package libraries>
     *
     * A matching `lib${module.name}.so` must be present in the `lib` directory
     * for every item in [modules].
     *
     * Note that it is expected for all modules to use the same headers. This is
     * currently the case for all ports currently maintained, but could change
     * in the future.
     *
     * In an autoconf build, this runs `make install`.
     *
     * @param[toolchain] The toolchain used for this build.
     * @param[buildDirectory] The directory containing build artifacts.
     * @param[installDirectory] The destination directory for this package's
     * installed headers and libraries.
     * @return A [Result<Unit, String>][Result] describing the result of the
     * operation. On failure, [Result.Error<String>][Result.Error] containing an
     * error message is returned.
     */
    open fun install(
        toolchain: Toolchain,
        buildDirectory: File,
        installDirectory: File
    ): Result<Unit, String> = Result.Ok(Unit)
}