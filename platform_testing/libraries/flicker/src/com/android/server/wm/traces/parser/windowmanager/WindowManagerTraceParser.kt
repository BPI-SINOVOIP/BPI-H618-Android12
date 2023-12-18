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

package com.android.server.wm.traces.parser.windowmanager

import android.app.nano.WindowConfigurationProto
import android.content.nano.ConfigurationProto
import android.graphics.nano.RectProto
import android.util.Log
import android.view.nano.ViewProtoEnums
import android.view.nano.WindowLayoutParamsProto
import com.android.server.wm.traces.common.windowmanager.windows.Configuration
import com.android.server.wm.traces.common.Rect
import com.android.server.wm.traces.common.windowmanager.windows.WindowConfiguration
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.common.windowmanager.WindowManagerState
import com.android.server.wm.traces.common.windowmanager.windows.Activity
import com.android.server.wm.traces.common.windowmanager.windows.ActivityTask
import com.android.server.wm.traces.common.windowmanager.windows.ConfigurationContainer
import com.android.server.wm.traces.common.windowmanager.windows.DisplayArea
import com.android.server.wm.traces.common.windowmanager.windows.DisplayContent
import com.android.server.wm.traces.common.windowmanager.windows.KeyguardControllerState
import com.android.server.wm.traces.common.windowmanager.windows.RootWindowContainer
import com.android.server.wm.traces.common.windowmanager.windows.WindowContainer
import com.android.server.wm.traces.common.windowmanager.windows.WindowManagerPolicy
import com.android.server.wm.traces.common.windowmanager.windows.WindowState
import com.android.server.wm.traces.common.windowmanager.windows.WindowToken
import com.android.server.wm.nano.ActivityRecordProto
import com.android.server.wm.nano.AppTransitionProto
import com.android.server.wm.nano.ConfigurationContainerProto
import com.android.server.wm.nano.DisplayAreaProto
import com.android.server.wm.nano.DisplayContentProto
import com.android.server.wm.nano.KeyguardControllerProto
import com.android.server.wm.nano.WindowManagerPolicyProto
import com.android.server.wm.nano.RootWindowContainerProto
import com.android.server.wm.nano.TaskProto
import com.android.server.wm.nano.WindowContainerChildProto
import com.android.server.wm.nano.WindowContainerProto
import com.android.server.wm.nano.WindowManagerServiceDumpProto
import com.android.server.wm.nano.WindowManagerTraceFileProto
import com.android.server.wm.nano.WindowStateProto
import com.android.server.wm.nano.WindowTokenProto
import com.android.server.wm.traces.common.Bounds
import com.android.server.wm.traces.common.windowmanager.windows.WindowLayoutParams
import com.android.server.wm.traces.parser.LOG_TAG
import com.google.protobuf.nano.InvalidProtocolBufferNanoException
import java.nio.file.Path
import kotlin.system.measureTimeMillis

object WindowManagerTraceParser {
    private const val TRANSIT_ACTIVITY_OPEN = "TRANSIT_ACTIVITY_OPEN"
    private const val TRANSIT_ACTIVITY_CLOSE = "TRANSIT_ACTIVITY_CLOSE"
    private const val TRANSIT_TASK_OPEN = "TRANSIT_TASK_OPEN"
    private const val TRANSIT_TASK_CLOSE = "TRANSIT_TASK_CLOSE"
    private const val TRANSIT_WALLPAPER_OPEN = "TRANSIT_WALLPAPER_OPEN"
    private const val TRANSIT_WALLPAPER_CLOSE = "TRANSIT_WALLPAPER_CLOSE"
    private const val TRANSIT_WALLPAPER_INTRA_OPEN = "TRANSIT_WALLPAPER_INTRA_OPEN"
    private const val TRANSIT_WALLPAPER_INTRA_CLOSE = "TRANSIT_WALLPAPER_INTRA_CLOSE"
    private const val TRANSIT_KEYGUARD_GOING_AWAY = "TRANSIT_KEYGUARD_GOING_AWAY"
    private const val TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER =
        "TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER"
    private const val TRANSIT_KEYGUARD_OCCLUDE = "TRANSIT_KEYGUARD_OCCLUDE"
    private const val TRANSIT_KEYGUARD_UNOCCLUDE = "TRANSIT_KEYGUARD_UNOCCLUDE"
    private const val TRANSIT_TRANSLUCENT_ACTIVITY_OPEN = "TRANSIT_TRANSLUCENT_ACTIVITY_OPEN"
    private const val TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE = "TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE"

    /**
     * Parses [WindowManagerTraceFileProto] from [data] and uses the proto to generates
     * a list of trace entries.
     *
     * @param data binary proto data
     * @param source Path to source of data for additional debug information
     * @param checksum File SHA512 checksum
     */
    @JvmOverloads
    @JvmStatic
    fun parseFromTrace(
        data: ByteArray?,
        source: Path? = null,
        checksum: String = ""
    ): WindowManagerTrace {
        val fileProto: WindowManagerTraceFileProto
        try {
            measureTimeMillis {
                fileProto = WindowManagerTraceFileProto.parseFrom(data)
            }.also {
                Log.v(LOG_TAG, "Parsing proto (WM Trace): ${it}ms")
            }
        } catch (e: InvalidProtocolBufferNanoException) {
            throw RuntimeException(e)
        }
        return parseFromTrace(fileProto, source, checksum)
    }

    /**
     * Uses the proto to generates a list of trace entries.
     *
     * @param proto Parsed proto data
     * @param source Path to source of data for additional debug information
     * @param checksum File SHA512 checksum
     */
    @JvmOverloads
    @JvmStatic
    fun parseFromTrace(
        proto: WindowManagerTraceFileProto,
        source: Path? = null,
        checksum: String = ""
    ): WindowManagerTrace {
        val entries = mutableListOf<WindowManagerState>()
        var traceParseTime = 0L
        for (entryProto in proto.entry) {
            val entryParseTime = measureTimeMillis {
                val entry = newTraceEntry(entryProto.windowManagerService,
                    entryProto.elapsedRealtimeNanos, entryProto.where)
                entries.add(entry)
            }
            traceParseTime += entryParseTime
        }

        Log.v(LOG_TAG, "Parsing duration (WM Trace): ${traceParseTime}ms " +
            "(avg ${traceParseTime / entries.size}ms per entry)")
        return WindowManagerTrace(entries, source?.toAbsolutePath()?.toString()
            ?: "", checksum)
    }

    /**
     * Parses [WindowManagerServiceDumpProto] from [proto] dump and uses the proto to generates
     * a list of trace entries.
     *
     * @param proto Parsed proto data
     */
    @JvmStatic
    fun parseFromDump(proto: WindowManagerServiceDumpProto): WindowManagerTrace {
        return WindowManagerTrace(
            listOf(newTraceEntry(proto, timestamp = 0, where = "")),
            source = "",
            sourceChecksum = "")
    }

    /**
     * Parses [WindowManagerServiceDumpProto] from [data] and uses the proto to generates
     * a list of trace entries.
     *
     * @param data binary proto data
     */
    @JvmStatic
    fun parseFromDump(data: ByteArray?): WindowManagerTrace {
        val fileProto = try {
            WindowManagerServiceDumpProto.parseFrom(data)
        } catch (e: InvalidProtocolBufferNanoException) {
            throw RuntimeException(e)
        }
        return WindowManagerTrace(parseFromDump(fileProto), source = "", sourceChecksum = "")
    }

    private fun newTraceEntry(
        proto: WindowManagerServiceDumpProto,
        timestamp: Long,
        where: String
    ): WindowManagerState {
        return WindowManagerState(
            where = where,
            policy = newWindowManagerPolicy(proto.policy),
            focusedApp = proto.focusedApp,
            focusedDisplayId = proto.focusedDisplayId,
            focusedWindow = proto.focusedWindow?.title ?: "",
            inputMethodWindowAppToken = if (proto.inputMethodWindow != null) {
                Integer.toHexString(proto.inputMethodWindow.hashCode)
            } else {
                ""
            },
            isHomeRecentsComponent = proto.rootWindowContainer.isHomeRecentsComponent,
            isDisplayFrozen = proto.displayFrozen,
            pendingActivities = proto.rootWindowContainer.pendingActivities
                .map { it.title }.toTypedArray(),
            root = newRootWindowContainer(proto.rootWindowContainer),
            keyguardControllerState = newKeyguardControllerState(
                proto.rootWindowContainer.keyguardController),
            timestamp = timestamp
        )
    }

    private fun newWindowManagerPolicy(proto: WindowManagerPolicyProto): WindowManagerPolicy? {
        return WindowManagerPolicy(
            focusedAppToken = proto.focusedAppToken ?: "",
            forceStatusBar = proto.forceStatusBar,
            forceStatusBarFromKeyguard = proto.forceStatusBarFromKeyguard,
            keyguardDrawComplete = proto.keyguardDrawComplete,
            keyguardOccluded = proto.keyguardOccluded,
            keyguardOccludedChanged = proto.keyguardOccludedChanged,
            keyguardOccludedPending = proto.keyguardOccludedPending,
            lastSystemUiFlags = proto.lastSystemUiFlags,
            orientation = proto.orientation,
            rotation = proto.rotation,
            rotationMode = proto.rotationMode,
            screenOnFully = proto.screenOnFully,
            windowManagerDrawComplete = proto.windowManagerDrawComplete
        )
    }

    private fun newRootWindowContainer(proto: RootWindowContainerProto): RootWindowContainer {
        return RootWindowContainer(
            newWindowContainer(
                proto.windowContainer,
                proto.windowContainer.children
                    .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree = false) }
            ) ?: error("Window container should not be null")
        )
    }

    private fun newKeyguardControllerState(
        proto: KeyguardControllerProto?
    ): KeyguardControllerState {
        return KeyguardControllerState(
            isAodShowing = proto?.aodShowing ?: false,
            isKeyguardShowing = proto?.keyguardShowing ?: false,
            keyguardOccludedStates = proto?.keyguardOccludedStates
                ?.map { it.displayId to it.keyguardOccluded }
                ?.toMap()
                ?: emptyMap()
        )
    }

    private fun newWindowContainerChild(
        proto: WindowContainerChildProto,
        isActivityInTree: Boolean
    ): WindowContainer? {
        return newDisplayContent(proto.displayContent, isActivityInTree)
            ?: newDisplayArea(proto.displayArea, isActivityInTree)
            ?: newTask(proto.task, isActivityInTree) ?: newActivity(proto.activity)
            ?: newWindowToken(proto.windowToken, isActivityInTree)
            ?: newWindowState(proto.window, isActivityInTree)
            ?: newWindowContainer(proto.windowContainer, children = emptyList())
    }

    private fun newDisplayContent(
        proto: DisplayContentProto?,
        isActivityInTree: Boolean
    ): DisplayContent? {
        return if (proto == null) {
            null
        } else {
            DisplayContent(
                id = proto.id,
                focusedRootTaskId = proto.focusedRootTaskId,
                resumedActivity = proto.resumedActivity?.title ?: "",
                singleTaskInstance = proto.singleTaskInstance,
                _defaultPinnedStackBounds = proto.pinnedTaskController?.defaultBounds?.toRect(),
                _pinnedStackMovementBounds = proto.pinnedTaskController?.movementBounds?.toRect(),
                displayRect = Rect(0, 0, proto.displayInfo?.logicalWidth
                    ?: 0, proto.displayInfo?.logicalHeight ?: 0),
                appRect = Rect(0, 0, proto.displayInfo?.appWidth ?: 0,
                    proto.displayInfo?.appHeight
                        ?: 0),
                dpi = proto.dpi,
                flags = proto.displayInfo?.flags ?: 0,
                _stableBounds = proto.displayFrames?.stableBounds?.toRect(),
                surfaceSize = proto.surfaceSize,
                focusedApp = proto.focusedApp,
                lastTransition = appTransitionToString(
                    proto.appTransition?.lastUsedAppTransition ?: 0),
                appTransitionState = appStateToString(
                    proto.appTransition?.appTransitionState ?: 0),
                rotation = proto.displayRotation?.rotation ?: 0,
                lastOrientation = proto.displayRotation?.lastOrientation ?: 0,
                windowContainer = newWindowContainer(
                    proto.rootDisplayArea.windowContainer,
                    proto.rootDisplayArea.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree) },
                    nameOverride = proto.displayInfo?.name ?: ""
                ) ?: error("Window container should not be null")
            )
        }
    }

    private fun newDisplayArea(proto: DisplayAreaProto?, isActivityInTree: Boolean): DisplayArea? {
        return if (proto == null) {
            null
        } else {
            DisplayArea(
                isTaskDisplayArea = proto.isTaskDisplayArea,
                windowContainer = newWindowContainer(
                    proto.windowContainer,
                    proto.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree) }
                ) ?: error("Window container should not be null")
            )
        }
    }

    private fun newTask(proto: TaskProto?, isActivityInTree: Boolean): ActivityTask? {
        return if (proto == null) {
            null
        } else {
            ActivityTask(
                activityType = proto.activityType,
                isFullscreen = proto.fillsParent,
                bounds = proto.bounds.toRect(),
                taskId = proto.id,
                rootTaskId = proto.rootTaskId,
                displayId = proto.displayId,
                _lastNonFullscreenBounds = proto.lastNonFullscreenBounds?.toRect(),
                realActivity = proto.realActivity,
                origActivity = proto.origActivity,
                resizeMode = proto.resizeMode,
                _resumedActivity = proto.resumedActivity?.title ?: "",
                animatingBounds = proto.animatingBounds,
                surfaceWidth = proto.surfaceWidth,
                surfaceHeight = proto.surfaceHeight,
                createdByOrganizer = proto.createdByOrganizer,
                minWidth = proto.minWidth,
                minHeight = proto.minHeight,
                windowContainer = newWindowContainer(
                    proto.windowContainer,
                    proto.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree) }
                ) ?: error("Window container should not be null")
            )
        }
    }

    private fun newActivity(proto: ActivityRecordProto?): Activity? {
        return if (proto == null) {
            null
        } else {
            Activity(
                name = proto.name,
                state = proto.state,
                visible = proto.visible,
                frontOfTask = proto.frontOfTask,
                procId = proto.procId,
                isTranslucent = proto.translucent,
                windowContainer = newWindowContainer(
                    proto.windowToken.windowContainer,
                    proto.windowToken.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree = true) }
                ) ?: error("Window container should not be null")
            )
        }
    }

    private fun newWindowToken(proto: WindowTokenProto?, isActivityInTree: Boolean): WindowToken? {
        return if (proto == null) {
            null
        } else {
            WindowToken(
                newWindowContainer(
                    proto.windowContainer,
                    proto.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree) }
                ) ?: error("Window container should not be null")
            )
        }
    }

    private fun newWindowState(proto: WindowStateProto?, isActivityInTree: Boolean): WindowState? {
        return if (proto == null) {
            null
        } else {
            val identifierName = proto.windowContainer.identifier?.title ?: ""
            WindowState(
                attributes = newWindowLayerParams(proto.attributes),
                displayId = proto.displayId,
                stackId = proto.stackId,
                layer = proto.animator?.surface?.layer ?: 0,
                isSurfaceShown = proto.animator?.surface?.shown ?: false,
                windowType = when {
                    identifierName.startsWith(WindowState.STARTING_WINDOW_PREFIX) ->
                        WindowState.WINDOW_TYPE_STARTING
                    proto.animatingExit -> WindowState.WINDOW_TYPE_EXITING
                    identifierName.startsWith(WindowState.DEBUGGER_WINDOW_PREFIX) ->
                        WindowState.WINDOW_TYPE_STARTING
                    else -> 0
                },
                requestedSize = Bounds(proto.requestedWidth, proto.requestedHeight),
                surfacePosition = proto.surfacePosition?.toRect(),
                _frame = proto.windowFrames?.frame?.toRect(),
                _containingFrame = proto.windowFrames?.containingFrame?.toRect(),
                _parentFrame = proto.windowFrames?.parentFrame?.toRect(),
                _contentFrame = proto.windowFrames?.contentFrame?.toRect(),
                _contentInsets = proto.windowFrames?.contentInsets?.toRect(),
                _surfaceInsets = proto.surfaceInsets?.toRect(),
                _givenContentInsets = proto.givenContentInsets?.toRect(),
                _crop = proto.animator?.lastClipRect?.toRect(),
                windowContainer = newWindowContainer(
                    proto.windowContainer,
                    proto.windowContainer.children
                        .mapNotNull { p -> newWindowContainerChild(p, isActivityInTree) },
                    nameOverride = when {
                        // Existing code depends on the prefix being removed
                        identifierName.startsWith(WindowState.STARTING_WINDOW_PREFIX) ->
                            identifierName.substring(WindowState.STARTING_WINDOW_PREFIX.length)
                        identifierName.startsWith(WindowState.DEBUGGER_WINDOW_PREFIX) ->
                            identifierName.substring(WindowState.DEBUGGER_WINDOW_PREFIX.length)
                        else -> identifierName
                    }
                ) ?: error("Window container should not be null"),
                isAppWindow = isActivityInTree
            )
        }
    }

    private fun newWindowLayerParams(proto: WindowLayoutParamsProto?): WindowLayoutParams {
        return WindowLayoutParams(
            type = proto?.type ?: 0,
            x = proto?.x ?: 0,
            y = proto?.y ?: 0,
            width = proto?.width ?: 0,
            height = proto?.height ?: 0,
            horizontalMargin = proto?.horizontalMargin ?: 0f,
            verticalMargin = proto?.verticalMargin ?: 0f,
            gravity = proto?.gravity ?: 0,
            softInputMode = proto?.softInputMode ?: 0,
            format = proto?.format ?: 0,
            windowAnimations = proto?.windowAnimations ?: 0,
            alpha = proto?.alpha ?: 0f,
            screenBrightness = proto?.screenBrightness ?: 0f,
            buttonBrightness = proto?.buttonBrightness ?: 0f,
            rotationAnimation = proto?.rotationAnimation ?: 0,
            preferredRefreshRate = proto?.preferredRefreshRate ?: 0f,
            preferredDisplayModeId = proto?.preferredDisplayModeId ?: 0,
            hasSystemUiListeners = proto?.hasSystemUiListeners ?: false,
            inputFeatureFlags = proto?.inputFeatureFlags ?: 0,
            userActivityTimeout = proto?.userActivityTimeout ?: 0,
            colorMode = proto?.colorMode ?: 0,
            flags = proto?.flags ?: 0,
            privateFlags = proto?.privateFlags ?: 0,
            systemUiVisibilityFlags = proto?.systemUiVisibilityFlags ?: 0,
            subtreeSystemUiVisibilityFlags = proto?.subtreeSystemUiVisibilityFlags ?: 0,
            appearance = proto?.appearance ?: 0,
            behavior = proto?.behavior ?: 0,
            fitInsetsTypes = proto?.fitInsetsTypes ?: 0,
            fitInsetsSides = proto?.fitInsetsSides ?: 0,
            fitIgnoreVisibility = proto?.fitIgnoreVisibility ?: false
        )
    }

    private fun newConfigurationContainer(
        proto: ConfigurationContainerProto?
    ): ConfigurationContainer {
        return ConfigurationContainer(
            overrideConfiguration = newConfiguration(proto?.overrideConfiguration),
            fullConfiguration = newConfiguration(proto?.fullConfiguration),
            mergedOverrideConfiguration = newConfiguration(proto?.mergedOverrideConfiguration)
        )
    }

    private fun newConfiguration(proto: ConfigurationProto?): Configuration? {
        return if (proto == null) {
            null
        } else {
            Configuration(
                windowConfiguration = if (proto.windowConfiguration != null) {
                    newWindowConfiguration(proto.windowConfiguration)
                } else {
                    null
                },
                densityDpi = proto.densityDpi,
                orientation = proto.orientation,
                screenHeightDp = proto.screenHeightDp,
                screenWidthDp = proto.screenWidthDp,
                smallestScreenWidthDp = proto.smallestScreenWidthDp,
                screenLayout = proto.screenLayout,
                uiMode = proto.uiMode
            )
        }
    }

    private fun newWindowConfiguration(proto: WindowConfigurationProto): WindowConfiguration {
        return WindowConfiguration(
            _appBounds = proto.appBounds?.toRect(),
            _bounds = proto.bounds?.toRect(),
            _maxBounds = proto.maxBounds?.toRect(),
            windowingMode = proto.windowingMode,
            activityType = proto.activityType
        )
    }

    private fun newWindowContainer(
        proto: WindowContainerProto?,
        children: List<WindowContainer>,
        nameOverride: String? = null
    ): WindowContainer? {
        return if (proto == null) {
            null
        } else {
            WindowContainer(
                title = nameOverride ?: proto.identifier?.title ?: "",
                token = proto.identifier?.hashCode?.toString(16) ?: "",
                orientation = proto.orientation,
                _isVisible = proto.visible,
                configurationContainer = newConfigurationContainer(
                    proto.configurationContainer),
                children.toTypedArray()
            )
        }
    }

    private fun appTransitionToString(transition: Int): String {
        return when (transition) {
            ViewProtoEnums.TRANSIT_UNSET -> "TRANSIT_UNSET"
            ViewProtoEnums.TRANSIT_NONE -> "TRANSIT_NONE"
            ViewProtoEnums.TRANSIT_ACTIVITY_OPEN -> TRANSIT_ACTIVITY_OPEN
            ViewProtoEnums.TRANSIT_ACTIVITY_CLOSE -> TRANSIT_ACTIVITY_CLOSE
            ViewProtoEnums.TRANSIT_TASK_OPEN -> TRANSIT_TASK_OPEN
            ViewProtoEnums.TRANSIT_TASK_CLOSE -> TRANSIT_TASK_CLOSE
            ViewProtoEnums.TRANSIT_TASK_TO_FRONT -> "TRANSIT_TASK_TO_FRONT"
            ViewProtoEnums.TRANSIT_TASK_TO_BACK -> "TRANSIT_TASK_TO_BACK"
            ViewProtoEnums.TRANSIT_WALLPAPER_CLOSE -> TRANSIT_WALLPAPER_CLOSE
            ViewProtoEnums.TRANSIT_WALLPAPER_OPEN -> TRANSIT_WALLPAPER_OPEN
            ViewProtoEnums.TRANSIT_WALLPAPER_INTRA_OPEN -> TRANSIT_WALLPAPER_INTRA_OPEN
            ViewProtoEnums.TRANSIT_WALLPAPER_INTRA_CLOSE -> TRANSIT_WALLPAPER_INTRA_CLOSE
            ViewProtoEnums.TRANSIT_TASK_OPEN_BEHIND -> "TRANSIT_TASK_OPEN_BEHIND"
            ViewProtoEnums.TRANSIT_ACTIVITY_RELAUNCH -> "TRANSIT_ACTIVITY_RELAUNCH"
            ViewProtoEnums.TRANSIT_DOCK_TASK_FROM_RECENTS -> "TRANSIT_DOCK_TASK_FROM_RECENTS"
            ViewProtoEnums.TRANSIT_KEYGUARD_GOING_AWAY -> TRANSIT_KEYGUARD_GOING_AWAY
            ViewProtoEnums.TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER ->
                TRANSIT_KEYGUARD_GOING_AWAY_ON_WALLPAPER
            ViewProtoEnums.TRANSIT_KEYGUARD_OCCLUDE -> TRANSIT_KEYGUARD_OCCLUDE
            ViewProtoEnums.TRANSIT_KEYGUARD_UNOCCLUDE -> TRANSIT_KEYGUARD_UNOCCLUDE
            ViewProtoEnums.TRANSIT_TRANSLUCENT_ACTIVITY_OPEN ->
                TRANSIT_TRANSLUCENT_ACTIVITY_OPEN
            ViewProtoEnums.TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE ->
                TRANSIT_TRANSLUCENT_ACTIVITY_CLOSE
            ViewProtoEnums.TRANSIT_CRASHING_ACTIVITY_CLOSE -> "TRANSIT_CRASHING_ACTIVITY_CLOSE"
            else -> error("Invalid lastUsedAppTransition")
        }
    }

    private fun appStateToString(appState: Int): String {
        return when (appState) {
            AppTransitionProto.APP_STATE_IDLE -> "APP_STATE_IDLE"
            AppTransitionProto.APP_STATE_READY -> "APP_STATE_READY"
            AppTransitionProto.APP_STATE_RUNNING -> "APP_STATE_RUNNING"
            AppTransitionProto.APP_STATE_TIMEOUT -> "APP_STATE_TIMEOUT"
            else -> error("Invalid AppTransitionState")
        }
    }

    private fun RectProto.toRect() = Rect(this.left, this.top, this.right, this.bottom)
}