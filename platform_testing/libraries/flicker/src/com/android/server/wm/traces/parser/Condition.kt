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

package com.android.server.wm.traces.parser

import android.os.SystemClock
import android.util.Log

/**
 * The utility class to wait a condition with customized options.
 * The default retry policy is 5 times with interval 1 second.
 *
 * @param <T> The type of the object to validate.
 *
 * <p>Sample:</p>
 * <pre>
 * // Simple case.
 * if (Condition.waitFor("true value", () -> true)) {
 *     println("Success");
 * }
 * // Wait for customized result with customized validation.
 * String result = Condition.waitForResult(new Condition<String>("string comparison")
 *         .setResultSupplier(() -> "Result string")
 *         .setResultValidator(str -> str.equals("Expected string"))
 *         .setRetryIntervalMs(500)
 *         .setRetryLimit(3)
 *         .setOnFailure(str -> println("Failed on " + str)));
 * </pre>
 */
class Condition<T>
/**
 * Constructs with a simple boolean condition.
 *
 * When satisfier = null, it is expected that the condition will be configured with
 * [.setResultSupplier] and [.setResultValidator].
 *
 * @param message The message to show what is waiting for.
 * @param satisfier If it returns true, that means the condition is satisfied.
 */
@JvmOverloads constructor(
    private var message: String = "",
    /**
     * It decides whether this condition is satisfied.
     */
    private var satisfier: (() -> Boolean)? = null,
    private var retryLimit: Int = DEFAULT_RETRY_LIMIT,
    private var retryIntervalMs: Long = DEFAULT_RETRY_INTERVAL_MS
) {
    private var returnLastResult: Boolean = false

    /**
     * It is used when the condition is not a simple boolean expression, such as the caller may
     * want to get the validated product as the return value.
     */
    private var resultSupplier: (() -> T?)? = null

    /**
     * It validates the result from [.mResultSupplier].
     */
    private var resultValidator: ((T?) -> Boolean)? = null
    private var onFailure: ((T) -> Any)? = null
    private var onRetry: Runnable? = null
    private var lastResult: T? = null
    private var validatedResult: T? = null

    /**
     * Set the supplier which provides the result object to validate.
     */
    fun setResultSupplier(supplier: () -> T?): Condition<T> =
        apply { resultSupplier = supplier }

    /**
     * Set the validator which tests the object provided by the supplier.
     */
    fun setResultValidator(validator: (T?) -> Boolean): Condition<T> =
        apply { resultValidator = validator }

    /**
     * If true, when using [.waitForResult], the method will return the last result
     * provided by [.mResultSupplier] even it is not valid (by [.mResultValidator]).
     */
    fun setReturnLastResult(returnLastResult: Boolean): Condition<T> =
        apply { this.returnLastResult = returnLastResult }

    /**
     * Executes the action when the condition does not satisfy within the time limit. The passed
     * object to the consumer will be the last result from the supplier.
     */
    fun setOnFailure(onFailure: (T) -> Any): Condition<T> = apply { this.onFailure = onFailure }

    fun setOnRetry(onRetry: Runnable): Condition<T> = apply { this.onRetry = onRetry }

    fun setRetryIntervalMs(millis: Long): Condition<T> = apply { retryIntervalMs = millis }

    fun setRetryLimit(limit: Int): Condition<T> = apply { retryLimit = limit }

    /**
     * Build the condition by [.mResultSupplier] and [.mResultValidator].
     */
    private fun prepareSatisfier(): () -> Boolean {
        val supplier = resultSupplier
        val validator = resultValidator
        require(!(supplier == null || validator == null)) { "No specified condition" }

        return {
            val result = supplier.invoke()
            lastResult = result
            if (validator.invoke(result)) {
                validatedResult = result
                true
            } else {
                false
            }
        }.also {
            satisfier = it
        }
    }

    companion object {
        // TODO(b/112837428): Implement a incremental retry policy to reduce the unnecessary
        // constant time, currently keep the default as 5*1s because most of the original code
        // uses it, and some tests might be sensitive to the waiting interval.
        private const val DEFAULT_RETRY_LIMIT = 5
        private const val DEFAULT_RETRY_INTERVAL_MS = 1000L

        /**
         * @see .waitFor
         * @see .Condition
         */
        @JvmStatic
        @JvmOverloads
        fun <T> waitFor(
            message: String,
            retryLimit: Int = DEFAULT_RETRY_LIMIT,
            retryIntervalMs: Long = DEFAULT_RETRY_INTERVAL_MS,
            satisfier: () -> Boolean
        ): Boolean {
            val condition = Condition<T>(message, satisfier, retryLimit, retryIntervalMs)
            return waitFor(condition)
        }

        /**
         * @return `false` if the condition does not satisfy within the time limit.
         */
        @JvmStatic
        fun <T> waitFor(condition: Condition<T>): Boolean {
            val satisfier = condition.satisfier ?: condition.prepareSatisfier()
            val startTime = SystemClock.elapsedRealtime()
            Log.v(LOG_TAG, "***Waiting for ${condition.message}")
            for (i in 1..condition.retryLimit) {
                if (satisfier.invoke()) {
                    Log.v(LOG_TAG, "***Waiting for ${condition.message} ... Success!")
                    return true
                } else {
                    SystemClock.sleep(condition.retryIntervalMs)
                    Log.v(LOG_TAG, "***Waiting for ${condition.message} ... retry=$i" +
                        " elapsed=${SystemClock.elapsedRealtime() - startTime} ms")
                    val onRetry = condition.onRetry
                    if (onRetry != null && i < condition.retryLimit) {
                        onRetry.run()
                    }
                }
            }
            if (satisfier.invoke()) {
                Log.v(LOG_TAG, "***Waiting for ${condition.message} ... Success!")
                return true
            }
            val onFailure = condition.onFailure
            if (onFailure == null) {
                Log.e(LOG_TAG, "***Waiting for ${condition.message} ... Failed!")
            } else {
                val result = condition.lastResult
                require(result != null) { "Missing last result for failure notification" }
                onFailure.invoke(result)
            }
            return false
        }

        /**
         * @see .waitForResult
         */
        @JvmStatic
        fun <T> waitForResult(message: String, setup: (Condition<T>) -> Any): T? {
            val condition = Condition<T>(message)
            setup.invoke(condition)
            return waitForResult(condition)
        }

        /**
         * @return `null` if the condition does not satisfy within the time limit or the result
         * supplier returns `null`.
         */
        @JvmStatic
        fun <T> waitForResult(condition: Condition<T>): T? {
            condition.validatedResult = null
            condition.lastResult = condition.validatedResult
            condition.prepareSatisfier()
            waitFor(condition)
            return when {
                condition.validatedResult != null -> condition.validatedResult
                condition.returnLastResult -> condition.lastResult
                else -> null
            }
        }
    }
}