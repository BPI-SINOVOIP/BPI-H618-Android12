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

package com.android.tools.metalava.model

import com.android.tools.metalava.model.AnnotationTarget.INTERNAL
import org.junit.Assert.assertTrue
import org.junit.Test
import java.lang.reflect.Modifier

class AnnotationTargetTest {
    @Test
    fun `All annotation targets contain INTERNAL`() {
        // Kotlin reflection can't list top-level items yet, rely on Java reflection for now.
        Class.forName("${AnnotationTarget::class.qualifiedName}Kt").constants
            .also { assertTrue(it.isNotEmpty()) }
            .forEach { (name, value) ->
                val targets = value as Collection<*>
                assertTrue("$name: should contain $INTERNAL but doesn't: $targets",
                        targets.contains(INTERNAL))
            }
    }

    private val Class<*>.constants get() = methods
            .filter { Modifier.isStatic(it.modifiers) }
            .filter { it.name.startsWith("get") }
            .map { it.name to it.invoke(null) }
}
