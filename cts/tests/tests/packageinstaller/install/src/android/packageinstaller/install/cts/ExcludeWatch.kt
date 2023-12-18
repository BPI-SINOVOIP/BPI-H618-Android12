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

package android.packageinstaller.install.cts

import android.content.pm.PackageManager
import org.junit.Assume.assumeFalse
import org.junit.rules.TestRule
import org.junit.runner.Description
import org.junit.runners.model.Statement

class ExcludeWatch(
    val motivation: String,
    val packageManager: PackageManager
) : TestRule {

    override fun apply(stmt: Statement?, desc: Description?): Statement {
        return FeatureStatement()
    }

    inner class FeatureStatement : Statement() {
        override fun evaluate() {
            assumeFalse(
                motivation,
                packageManager.hasSystemFeature(PackageManager.FEATURE_WATCH)
            )
        }
    }
}