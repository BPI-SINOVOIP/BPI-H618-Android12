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

package com.android.tools.metalava

import com.android.tools.metalava.LibraryBuildInfoFile.Check
import com.google.gson.GsonBuilder
import org.gradle.api.DefaultTask
import org.gradle.api.GradleException
import org.gradle.api.Project
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.TaskProvider
import java.io.File
import java.util.concurrent.TimeUnit

const val CREATE_BUILD_INFO_TASK = "createBuildInfo"

abstract class CreateLibraryBuildInfoTask : DefaultTask() {
    @get:Input
    abstract val artifactId: Property<String>
    @get:Input
    abstract val groupId: Property<String>
    @get:Input
    abstract val version: Property<String>
    @get:Input
    abstract val sha: Property<String>

    @get:OutputFile
    abstract val outputFile: Property<File>

    @get:OutputFile
    abstract val aggregateOutputFile: Property<File>

    @TaskAction
    fun createFile() {
        val info = LibraryBuildInfoFile()
        info.artifactId = artifactId.get()
        info.groupId = groupId.get()
        info.groupIdRequiresSameVersion = false
        info.version = version.get()
        info.path = "/"
        info.sha = sha.get()
        info.dependencies = arrayListOf()
        info.checks = arrayListOf()
        val gson = GsonBuilder().setPrettyPrinting().create()
        val serializedInfo: String = gson.toJson(info)
        outputFile.get().writeText(serializedInfo)

        aggregateOutputFile.get().let {
            it.writeText("{ \"artifacts\": [\n")
            it.appendText(serializedInfo)
            it.appendText("]}")
        }
    }
}

fun configureBuildInfoTask(
    project: Project,
    inCI: Boolean,
    distributionDirectory: File
): TaskProvider<CreateLibraryBuildInfoTask> {
    return project.tasks.register(CREATE_BUILD_INFO_TASK, CreateLibraryBuildInfoTask::class.java) {
        it.artifactId.set(project.provider<String> { project.name })
        it.groupId.set(project.provider<String> { project.group as String })
        it.version.set(project.provider<String> { project.version as String })
        // Only set sha when in CI to keep local builds faster
        it.sha.set(project.provider<String> { if (inCI) getGitSha(project.projectDir) else "" })
        it.outputFile.set(project.provider<File> {
            File(
                distributionDirectory,
                "build-info/${project.group}_${project.name}_build_info.txt"
            )
        })
        it.aggregateOutputFile.set(project.provider<File> {
            File(distributionDirectory, "androidx_aggregate_build_info.txt")}
        )
    }
}

fun getGitSha(directory: File): String {
    val process = ProcessBuilder("git", "rev-parse", "--verify", "HEAD")
        .directory(directory)
        .redirectOutput(ProcessBuilder.Redirect.PIPE)
        .redirectError(ProcessBuilder.Redirect.PIPE)
        .start()
    // Read output, waiting for process to finish, as needed
    val stdout = process.inputStream.bufferedReader().readText()
    val stderr = process.errorStream.bufferedReader().readText()
    val message = stdout + stderr
    // wait potentially a little bit longer in case Git was waiting for us to
    // read its response before it exited
    process.waitFor(10, TimeUnit.SECONDS)
    if (stderr != "") {
        throw GradleException("Unable to call git. Response was: $message")
    }
    check(process.exitValue() == 0) { "Nonzero exit value running git command." }
    return stdout.trim()
}

/**
 * Object outlining the format of a library's build info file.
 * This object will be serialized to json.
 * This file should match the corresponding class in Jetpad because
 * this object will be serialized to json and the result will be parsed by Jetpad.
 * DO NOT TOUCH.
 *
 * @property groupId library maven group Id
 * @property artifactId library maven artifact Id
 * @property version library maven version
 * @property path local project directory path used for development, rooted at framework/support
 * @property sha the sha of the latest commit to modify the library (aka a commit that
 * touches a file within [path])
 * @property groupIdRequiresSameVersion boolean that determines if all libraries with [groupId]
 * have the same version
 * @property dependencies a list of dependencies on other androidx libraries
 * @property checks arraylist of [Check]s that is used by Jetpad
 */
@Suppress("UNUSED")
class LibraryBuildInfoFile {
    var groupId: String? = null
    var artifactId: String? = null
    var version: String? = null
    var path: String? = null
    var sha: String? = null
    var groupIdRequiresSameVersion: Boolean? = null
    var dependencies: ArrayList<Dependency> = arrayListOf()
    var checks: ArrayList<Check> = arrayListOf()

    /**
     * @property isTipOfTree boolean that specifies whether the dependency is tip-of-tree
     */
    inner class Dependency {
        var groupId: String? = null
        var artifactId: String? = null
        var version: String? = null
        var isTipOfTree = false
    }

    inner class Check {
        var name: String? = null
        var passing = false
    }
}