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

package com.android.server.wm.traces.common.windowmanager.windows

import com.android.server.wm.traces.common.Rect

/**
 * Represents a display content in the window manager hierarchy
 *
 * This is a generic object that is reused by both Flicker and Winscope and cannot
 * access internal Java/Android functionality
 *
 */
open class DisplayContent(
    val id: Int,
    val focusedRootTaskId: Int,
    val resumedActivity: String,
    val singleTaskInstance: Boolean,
    _defaultPinnedStackBounds: Rect?,
    _pinnedStackMovementBounds: Rect?,
    val displayRect: Rect,
    val appRect: Rect,
    val dpi: Int,
    val flags: Int,
    _stableBounds: Rect?,
    val surfaceSize: Int,
    val focusedApp: String,
    val lastTransition: String,
    val appTransitionState: String,
    val rotation: Int,
    val lastOrientation: Int,
    windowContainer: WindowContainer
) : WindowContainer(windowContainer) {
    override val name: String = id.toString()
    override val isVisible: Boolean = false

    val defaultPinnedStackBounds: Rect = _defaultPinnedStackBounds ?: Rect.EMPTY
    val pinnedStackMovementBounds: Rect = _pinnedStackMovementBounds ?: Rect.EMPTY
    val stableBounds: Rect = _stableBounds ?: Rect.EMPTY

    val rootTasks: Array<ActivityTask>
        get() {
            val tasks = this.collectDescendants<ActivityTask> { it.isRootTask }.toMutableList()
            // TODO(b/149338177): figure out how CTS tests deal with organizer. For now,
            //                    don't treat them as regular stacks
            val rootOrganizedTasks = mutableListOf<ActivityTask>()
            val reversedTaskList = tasks.reversed()
            reversedTaskList.forEach { task ->
                // Skip tasks created by an organizer
                if (task.createdByOrganizer) {
                    tasks.remove(task)
                    rootOrganizedTasks.add(task)
                }
            }
            // Add root tasks controlled by an organizer
            rootOrganizedTasks.reversed().forEach { task ->
                tasks.addAll(task.children.reversed().map { it as ActivityTask })
            }

            return tasks.toTypedArray()
        }

    fun containsActivity(activityName: String): Boolean =
        rootTasks.any { it.containsActivity(activityName) }

    fun getTaskDisplayArea(activityName: String): DisplayArea? {
        val taskDisplayAreas = this.collectDescendants<DisplayArea> { it.isTaskDisplayArea }
            .filter { it.containsActivity(activityName) }

        if (taskDisplayAreas.size > 1) {
            throw IllegalArgumentException(
                "There must be exactly one activity among all TaskDisplayAreas.")
        }

        return taskDisplayAreas.firstOrNull()
    }

    override fun toString(): String {
        return "${this::class.simpleName} #$id: name=$title mDisplayRect=$displayRect " +
            "mAppRect=$appRect mFlags=$flags"
    }
}