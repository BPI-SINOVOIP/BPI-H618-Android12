package com.softwinner.einklaunchrecent.service

import android.app.Activity
import android.app.Service
import android.content.Context
import android.content.Intent
import android.graphics.Rect
import android.graphics.Region
import android.hardware.display.DisplayManager
import android.os.Bundle
import android.os.IBinder
import android.os.Process
import android.util.Log
import android.view.Choreographer
import androidx.annotation.BinderThread
import androidx.annotation.Nullable
import androidx.annotation.UiThread
import androidx.annotation.WorkerThread
import com.android.systemui.shared.recents.IOverviewProxy
import com.android.systemui.shared.recents.ISystemUiProxy
import com.android.systemui.shared.system.ActivityManagerWrapper.CLOSE_SYSTEM_WINDOWS_REASON_RECENTS
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SHELL_ONE_HANDED
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SHELL_PIP
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SHELL_SHELL_TRANSITIONS
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SHELL_SPLIT_SCREEN
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SHELL_STARTING_WINDOW
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SMARTSPACE_TRANSITION_CONTROLLER
import com.android.systemui.shared.system.QuickStepContract.KEY_EXTRA_SYSUI_PROXY
import com.android.systemui.shared.system.QuickStepContract.SYSUI_STATE_TRACING_ENABLED
import com.android.systemui.shared.tracing.ProtoTraceable
import com.softwinner.einklaunchrecent.MainActivity
import com.softwinner.einklaunchrecent.util.utils
import java.util.*

class TouchService: Service() {
    private val TAG = utils.TAG
    companion object {
        var sConnected = false
        var sIsInitialized = false
        var mShouldStart = true
        var mContext: Context? = null
        var CLOSE_MSG = "close_activity"
        val mLock = Object()
    }

    @Nullable
    override fun onBind(intent: Intent?): IBinder? {
        Log.d(TAG, "Touch service connected: user=" + Process.myUserHandle().identifier)
        return ELRBinder()
    }

    class ELRBinder: IOverviewProxy.Stub() {
        private val TAG = utils.TAG
        @BinderThread
        override fun onInitialize(bundle: Bundle) {
            if (utils.DEBUG) Log.w(TAG, "onInitialize")
            //TODO
            sIsInitialized = true
        }

        @BinderThread
        override fun onOverviewToggle() {
            if (utils.DEBUG) Log.w(TAG, "onOverviewToggle")
            startActivity()

            //TODO
        }

        @BinderThread
        override fun onOverviewShown(triggeredFromAltTab: Boolean) {
            if (utils.DEBUG) Log.w(TAG, "onOverviewToggle $triggeredFromAltTab")
            //TODO
        }

        @BinderThread
        override fun onOverviewHidden(
            triggeredFromAltTab: Boolean,
            triggeredFromHomeKey: Boolean
        ) {
            if (utils.DEBUG) Log.w(TAG, "onOverviewHidden $triggeredFromAltTab $triggeredFromHomeKey")
            //TODO
        }

        @BinderThread
        override fun onTip(actionType: Int, viewType: Int) {
            if (utils.DEBUG) Log.w(TAG, "onTip $actionType $viewType")
            //TODO
        }

        @BinderThread
        override fun onAssistantAvailable(available: Boolean) {
            if (utils.DEBUG) Log.w(TAG, "onAssistantAvailable $available")
            //TODO
        }

        @BinderThread
        override fun onAssistantVisibilityChanged(visibility: Float) {
            if (utils.DEBUG) Log.w(TAG, "onAssistantVisibilityChanged $visibility")
            //TODO
        }

        @BinderThread
        override fun onBackAction(
            completed: Boolean, downX: Int, downY: Int, isButton: Boolean,
            gestureSwipeLeft: Boolean
        ) {
            if (utils.DEBUG) Log.w(TAG, "onBackAction $completed $downX $downY $isButton $gestureSwipeLeft")
            //TODO
        }

        @BinderThread
        override fun onSystemUiStateChanged(stateFlags: Int) {
            if (utils.DEBUG) Log.w(TAG, "onSystemUiStateChanged $stateFlags")
            //TODO
        }

        @BinderThread
        override fun onActiveNavBarRegionChanges(region: Region?) {
            if (utils.DEBUG) Log.w(TAG, "onActiveNavBarRegionChanges $region")
            //TODO
        }

        override fun onSplitScreenSecondaryBoundsChanged(bounds: Rect?, insets: Rect?) {
            if (utils.DEBUG) Log.w(TAG, "onActiveNavBarRegionChanges $bounds $insets")
            //TODO
        }

        override fun onImeWindowStatusChanged(
            displayId: Int, token: IBinder?, vis: Int,
            backDisposition: Int, showImeSwitcher: Boolean
        ) {
            if (utils.DEBUG) Log.w(TAG, "onActiveNavBarRegionChanges $displayId $vis $backDisposition $showImeSwitcher")
            //TODO
        }

        private fun startActivity() {
            if (mContext != null) {
                mContext?.startActivity(Intent(mContext, MainActivity::class.java).also {
                    if (utils.DEBUG) Log.w(TAG, "onOverviewToggle mShouldStart $mShouldStart")
                    it.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                })
            } else {
                if (utils.DEBUG) Log.w(TAG, "onOverviewToggle start activity failed")
            }
        }
    }

    private val KEY_BACK_NOTIFICATION_COUNT = "backNotificationCount"
    private val NOTIFY_ACTION_BACK = "com.android.quickstep.action.BACK_GESTURE"
    private val HAS_ENABLED_QUICKSTEP_ONCE = "launcher.has_enabled_quickstep_once"
    private val MAX_BACK_NOTIFICATION_COUNT = 3
    private val SYSTEM_ACTION_ID_ALL_APPS = 14
    private var mBackGestureNotificationCounter = -1
    fun isConnected(): Boolean {
        return sConnected
    }

    fun isInitialized(): Boolean {
        return sIsInitialized
    }
    private var mMainChoreographer: Choreographer? = null
    private var mDisplayManager: DisplayManager? = null

    override fun onCreate() {
        super.onCreate()
        mContext = this
        // Initialize anything here that is needed in direct boot mode.
        // Everything else should be initialized in onUserUnlocked() below.
        mMainChoreographer = Choreographer.getInstance()
        mDisplayManager = getSystemService(DisplayManager::class.java)
        sConnected = true
    }

    @UiThread
    private fun onAssistantVisibilityChanged() {}

    override fun onDestroy() {
        Log.d(TAG, "Touch service destroyed: user=" + Process.myUserHandle().identifier)
        sIsInitialized = false
        sConnected = false
        super.onDestroy()
    }

    protected fun shouldNotifyBackGesture(): Boolean {
        return false
    }

    @WorkerThread
    protected fun tryNotifyBackGesture() {

    }
}

