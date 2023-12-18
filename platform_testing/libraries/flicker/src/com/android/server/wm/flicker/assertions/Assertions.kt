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

package com.android.server.wm.flicker.assertions

/**
 * Checks assertion on a single trace entry.
 *
 * @param <T> trace entry type to perform the assertion on. </T>
 */
typealias Assertion<T> = (T) -> Unit

/**
 * Utility class to store assertions with an identifier to help generate more useful debug data
 * when dealing with multiple assertions.
 */
open class NamedAssertion<T> (
    private val assertion: Assertion<T>,
    open val name: String
) : Assertion<T> {
    override fun invoke(target: T): Unit = assertion.invoke(target)

    override fun toString(): String = "Assertion($name)"
}

/**
 * Utility class to store assertions composed of multiple individual assertions
 */
class CompoundAssertion<T>(assertion: Assertion<T>, name: String) :
    NamedAssertion<T>(assertion, name) {
    private val assertions = mutableListOf<NamedAssertion<T>>()

    init {
        add(assertion, name)
    }

    override val name: String
        get() = assertions.joinToString(" and ") { it.name }

    /**
     * Executes all [assertions] on [target]
     */
    override fun invoke(target: T) {
        val failure = assertions.mapNotNull {
            kotlin.runCatching { it.invoke(target) }.exceptionOrNull()
        }.firstOrNull()
        if (failure != null) {
            throw failure
        }
    }

    override fun toString(): String = name

    /**
     * Adds a new assertion to the list
     */
    fun add(assertion: Assertion<T>, name: String) {
        assertions.add(NamedAssertion(assertion, name))
    }
}
