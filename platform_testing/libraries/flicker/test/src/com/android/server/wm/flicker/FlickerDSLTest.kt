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

package com.android.server.wm.flicker

import android.app.Instrumentation
import androidx.test.platform.app.InstrumentationRegistry
import com.android.server.wm.flicker.assertions.AssertionData
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.dsl.AssertionTag
import com.android.server.wm.flicker.dsl.FlickerBuilder
import com.android.server.wm.flicker.traces.eventlog.EventLogSubject
import com.android.server.wm.flicker.traces.layers.LayerTraceEntrySubject
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerStateSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject
import com.google.common.truth.Truth
import org.junit.Assert
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters
import kotlin.reflect.KClass

/**
 * Contains [Flicker] and [FlickerBuilder] tests.
 *
 * To run this test: `atest FlickerLibTest:FlickerDSLTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class FlickerDSLTest {
    private val instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation()
    private val TAG = "tag"
    private var executed = false

    private fun runFlicker(assertion: AssertionData) {
        executed = false
        val builder = FlickerBuilder(instrumentation)
        builder.transitions {
            withTag(TAG) {
                device.pressHome()
            }
        }
        val flicker = builder.build()
        flicker.execute()
        flicker.checkAssertion(assertion)
        Truth.assertWithMessage("Assertion was not executed")
            .that(executed)
            .isTrue()
    }

    private fun validateAssertion(
        assertion: AssertionData,
        expectedSubjectClass: KClass<out FlickerSubject>,
        expectedTag: String
    ) {
        Truth.assertWithMessage("Unexpected subject type")
            .that(assertion.expectedSubjectClass)
            .isEqualTo(expectedSubjectClass)
        Truth.assertWithMessage("Unexpected tag")
            .that(assertion.tag)
            .isEqualTo(expectedTag)
    }

    @Test
    fun checkBuiltWMStartAssertion() {
        val assertion = FlickerTestParameter.buildWmStartAssertion { executed = true }
        validateAssertion(assertion, WindowManagerStateSubject::class, AssertionTag.START)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltWMEndAssertion() {
        val assertion = FlickerTestParameter.buildWmEndAssertion { executed = true }
        validateAssertion(assertion, WindowManagerStateSubject::class, AssertionTag.END)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltWMAssertion() {
        val assertion = FlickerTestParameter.buildWMAssertion { executed = true }
        validateAssertion(assertion, WindowManagerTraceSubject::class, AssertionTag.ALL)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltWMTagAssertion() {
        val assertion = FlickerTestParameter.buildWMTagAssertion(TAG) { executed = true }
        validateAssertion(assertion, WindowManagerStateSubject::class, TAG)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltLayersStartAssertion() {
        val assertion = FlickerTestParameter.buildLayersStartAssertion { executed = true }
        validateAssertion(assertion, LayerTraceEntrySubject::class, AssertionTag.START)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltLayersEndAssertion() {
        val assertion = FlickerTestParameter.buildLayersEndAssertion { executed = true }
        validateAssertion(assertion, LayerTraceEntrySubject::class, AssertionTag.END)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltLayersAssertion() {
        val assertion = FlickerTestParameter.buildLayersAssertion { executed = true }
        validateAssertion(assertion, LayersTraceSubject::class, AssertionTag.ALL)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltLayersTagAssertion() {
        val assertion = FlickerTestParameter.buildLayersTagAssertion(TAG) { executed = true }
        validateAssertion(assertion, LayerTraceEntrySubject::class, TAG)
        runFlicker(assertion)
    }

    @Test
    fun checkBuiltEventLogAssertion() {
        val assertion = FlickerTestParameter.buildEventLogAssertion { executed = true }
        validateAssertion(assertion, EventLogSubject::class, AssertionTag.ALL)
        runFlicker(assertion)
    }

    @Test
    fun supportDuplicatedTag() {
        var count = 0
        val assertion = FlickerTestParameter.buildWMTagAssertion(TAG) {
            count++
        }

        val builder = FlickerBuilder(instrumentation).apply {
            transitions {
                this.createTag(TAG)
                this.withTag(TAG) {
                    this.device.pressHome()
                }
            }
        }
        val flicker = builder.build().execute()

        flicker.checkAssertion(assertion)

        Truth.assertWithMessage("Should have asserted $TAG 2x")
            .that(count)
            .isEqualTo(2)
    }

    @Test
    fun preventInvalidTagNames() {
        try {
            val builder = FlickerBuilder(instrumentation).apply {
                transitions {
                    this.createTag("inv lid")
                }
            }
            builder.build().execute()
            Assert.fail("Should not have allowed invalid tag name")
        } catch (e: Exception) {
            Truth.assertWithMessage("Did not validate tag name")
                .that(e.cause?.message)
                .contains("The test tag inv lid can not contain spaces")
        }
    }

    @Test
    fun assertCreatedTags() {
        val builder = FlickerBuilder(instrumentation).apply {
            transitions {
                this.createTag(TAG)
                device.pressHome()
            }
        }
        val flicker = builder.build()
        val passAssertion = FlickerTestParameter.buildWMTagAssertion(TAG) {
            this.isNotEmpty()
        }
        val ignoredAssertion = FlickerTestParameter.buildWMTagAssertion("invalid") {
            fail("`Invalid` tag was not created, so it should not " +
                "have been asserted")
        }
        flicker.checkAssertion(passAssertion)
        flicker.checkAssertion(ignoredAssertion)
    }

    @Test
    fun detectEmptyResults() {
        try {
            FlickerBuilder(instrumentation).build().execute()
            Assert.fail("Should not have allowed empty transition")
        } catch (e: Exception) {
            Truth.assertWithMessage("Flicker did not warn of empty transitions")
                .that(e.message)
                .contains("A flicker test must include transitions to run")
        }
    }

    @Test
    fun detectCrashedTransition() {
        val exceptionMessage = "Crashed transition"
        val builder = FlickerBuilder(instrumentation)
        builder.transitions { error("Crashed transition") }
        val flicker = builder.build()
        try {
            flicker.execute()
            Assert.fail("Should have raised an exception with message $exceptionMessage")
        } catch (e: Throwable) {
            Truth.assertWithMessage("Incorrect exception message")
                .that(e.message)
                .contains("Unable to execute transition")
            Truth.assertWithMessage("Test exception does not contain original crash message")
                .that(e.cause?.message)
                .contains(exceptionMessage)
        }
    }
}
