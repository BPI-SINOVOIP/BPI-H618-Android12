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

import org.gradle.api.Project
import org.gradle.api.tasks.bundling.Zip
import java.io.File

const val CREATE_ARCHIVE_TASK = "createArchive"

fun configurePublishingArchive(
    project: Project,
    publicationName: String,
    repositoryName: String,
    buildId: String,
    distributionDirectory: File
) {
    project.tasks.register(CREATE_ARCHIVE_TASK, Zip::class.java) {
        it.description = "Create a zip of the library in a maven format"
        it.group = "publishing"

        it.from("${distributionDirectory.canonicalPath}/repo")
        it.archiveFileName.set(project.provider<String> {
            "per-project-zips/${project.group}-${project.name}-all-$buildId-${project.version}.zip"
        })
        it.destinationDirectory.set(distributionDirectory)
        it.dependsOn("publish${publicationName}PublicationTo${repositoryName}Repository")
    }
}