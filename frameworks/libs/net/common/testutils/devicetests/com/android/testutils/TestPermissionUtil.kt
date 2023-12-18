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

@file:JvmName("TestPermissionUtil")

package com.android.testutils

import androidx.test.platform.app.InstrumentationRegistry
import com.android.testutils.ExceptionUtils.ThrowingRunnable
import com.android.testutils.ExceptionUtils.ThrowingSupplier

/**
 * Run the specified [task] with the specified [permissions] obtained through shell
 * permission identity.
 *
 * Passing in an empty list of permissions can grant all shell permissions, but this is
 * discouraged as it also causes the process to temporarily lose non-shell permissions.
 */
fun <T> runAsShell(vararg permissions: String, task: () -> T): T {
    val autom = InstrumentationRegistry.getInstrumentation().uiAutomation
    autom.adoptShellPermissionIdentity(*permissions)
    try {
        return task()
    } finally {
        autom.dropShellPermissionIdentity()
    }
}

/**
 * Convenience overload of [runAsShell] that uses a [ThrowingSupplier] for Java callers, when
 * only one/two/three permissions are needed.
 */
@JvmOverloads
fun <T> runAsShell(
    perm1: String,
    perm2: String = "",
    perm3: String = "",
    supplier: ThrowingSupplier<T>
): T = runAsShell(*getNonEmptyVarargs(perm1, perm2, perm3)) { supplier.get() }

/**
 * Convenience overload of [runAsShell] that uses a [ThrowingRunnable] for Java callers, when
 * only one/two/three permissions are needed.
 */
@JvmOverloads
fun runAsShell(
    perm1: String,
    perm2: String = "",
    perm3: String = "",
    runnable: ThrowingRunnable
): Unit = runAsShell(*getNonEmptyVarargs(perm1, perm2, perm3)) { runnable.run() }

/**
 * Get an array containing the first consecutive non-empty arguments out of three arguments.
 *
 * The first argument is assumed to be non-empty.
 */
private fun getNonEmptyVarargs(arg1: String, arg2: String, arg3: String): Array<String> {
    return when {
        arg2 == "" -> arrayOf(arg1)
        arg3 == "" -> arrayOf(arg1, arg2)
        else -> arrayOf(arg1, arg2, arg3)
    }
}