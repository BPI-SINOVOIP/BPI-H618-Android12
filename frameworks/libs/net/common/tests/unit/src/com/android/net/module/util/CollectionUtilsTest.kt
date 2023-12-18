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

package com.android.net.module.util

import androidx.test.filters.SmallTest
import androidx.test.runner.AndroidJUnit4
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@SmallTest
class CollectionUtilsTest {
    @Test
    fun testAny() {
        assertTrue(CollectionUtils.any(listOf("A", "B", "C", "D", "E")) { it == "E" })
        assertFalse(CollectionUtils.any(listOf("A", "B", "C", "D", "E")) { it == "F" })
        assertTrue(CollectionUtils.any(listOf("AA", "BBB")) { it.length >= 3 })
        assertFalse(CollectionUtils.any(listOf("A", "BB", "CCC")) { it.length >= 4 })
        assertFalse(CollectionUtils.any(listOf("A", "BB", "CCC")) { it.length < 0 })
        assertFalse(CollectionUtils.any(listOf<String>()) { true })
        assertFalse(CollectionUtils.any(listOf<String>()) { false })
        assertTrue(CollectionUtils.any(listOf("A")) { true })
        assertFalse(CollectionUtils.any(listOf("A")) { false })
    }

    @Test
    fun testIndexOf() {
        assertEquals(4, CollectionUtils.indexOf(listOf("A", "B", "C", "D", "E")) { it == "E" })
        assertEquals(0, CollectionUtils.indexOf(listOf("A", "B", "C", "D", "E")) { it == "A" })
        assertEquals(1, CollectionUtils.indexOf(listOf("AA", "BBB", "CCCC")) { it.length >= 3 })
        assertEquals(1, CollectionUtils.indexOf(listOf("AA", null, "CCCC")) { it == null })
        assertEquals(1, CollectionUtils.indexOf(listOf(null, "CCCC")) { it != null })
    }

    @Test
    fun testAll() {
        assertFalse(CollectionUtils.all(listOf("A", "B", "C", "D", "E")) { it != "E" })
        assertTrue(CollectionUtils.all(listOf("A", "B", "C", "D", "E")) { it != "F" })
        assertFalse(CollectionUtils.all(listOf("A", "BB", "CCC")) { it.length > 2 })
        assertTrue(CollectionUtils.all(listOf("A", "BB", "CCC")) { it.length >= 1 })
        assertTrue(CollectionUtils.all(listOf("A", "BB", "CCC")) { it.length < 4 })
        assertTrue(CollectionUtils.all(listOf<String>()) { true })
        assertTrue(CollectionUtils.all(listOf<String>()) { false })
        assertTrue(CollectionUtils.all(listOf(1)) { true })
        assertFalse(CollectionUtils.all(listOf(1)) { false })
    }
}
