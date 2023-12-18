package com.softwinner.einklaunchrecent.model

import android.app.Activity
import android.app.ActivityManager
import android.app.ActivityManager.TaskDescription
import android.app.ActivityTaskManager
import android.content.ComponentName
import android.content.Context
import android.util.Log
import com.android.systemui.shared.recents.model.Task
import com.android.systemui.shared.recents.model.Task.TaskKey
import com.android.systemui.shared.recents.model.ThumbnailData
import com.android.systemui.shared.system.ActivityManagerWrapper
import com.android.systemui.shared.system.KeyguardManagerCompat
import com.android.systemui.shared.system.TaskDescriptionCompat
import com.android.systemui.shared.system.TaskStackChangeListener
import com.softwinner.einklaunchrecent.util.utils

class TaskLoader(mContext: Context): TaskStackChangeListener() {
    private val TAG = utils.TAG

    var mIndex = 0
    private var mChangeId: Int = 1
    val mCacheTaskInfo = mutableListOf<TaskData>()

    private var mActivityManagerWrapper: ActivityManagerWrapper? = null
    private var mKeyguardManagerCompat: KeyguardManagerCompat? = null

    init {
        mActivityManagerWrapper = ActivityManagerWrapper.getInstance()
        mActivityManagerWrapper?.registerTaskStackListener(this)
        mKeyguardManagerCompat = KeyguardManagerCompat(mContext)
    }

    fun loadTask(): Boolean {
        return true
    }

    fun updateTasks(userId: Int):  List<ActivityManager.RecentTaskInfo> {
        return if (mActivityManagerWrapper == null || mKeyguardManagerCompat == null) emptyList()
        else  mActivityManagerWrapper!!.getRecentTasks(Integer.MAX_VALUE, userId)!!.also {
            mCacheTaskInfo.clear()
            if (utils.DEBUG) Log.w(TAG,"it ${it.size}")
            for (item in it) {
                Log.w(TAG, "item.baseIntent.component.packageName ${item.baseIntent.component.packageName}")
                if (item.baseIntent.component.packageName.contains("einklaunchrecent")) continue
                try {

                } catch (e: Exception) {
                    e.printStackTrace()
                }

                val thumbnails: ThumbnailData = ActivityManagerWrapper.getInstance().getTaskThumbnail(item.taskId, true /* reducedResolution */)
                val taskKey: TaskKey = TaskKey(item)
                val task = Task.from(taskKey, item, mKeyguardManagerCompat!!.isDeviceLocked(taskKey.userId))
                if (utils.DEBUG) Log.w(TAG, "item ${item.origActivity} " +
                        " ${item.configuration.windowConfiguration.getWindowingMode()}" +
                        " ${item.baseIntent.component}" +
                        " ${item.taskId}" +
                        " ${item.lastActiveTime}" +
                        " ${item.displayId}"
                )
                mCacheTaskInfo.add(
                    TaskData(
                        item.taskId,
                        item.origActivity,
                        TaskDescriptionCompat.getIcon(task.taskDescription, taskKey.userId),
                        thumbnails.thumbnail,
                        item.lastActiveTime,
                        item.displayId,
                        taskKey
                    )
                )
            }
        }
    }

    fun getCurrentTask(): TaskData? {
        return if (mIndex >= mCacheTaskInfo.size) null else mCacheTaskInfo[mIndex]
    }

    @Synchronized
    fun isTaskListValid(changeId: Int): Boolean {
        return mChangeId == changeId;
    }

    @Synchronized
    override fun onTaskStackChanged() {
        mChangeId++
    }

    override fun onTaskRemoved(taskId: Int) {
        for (i in mCacheTaskInfo.size - 1 downTo 0) {
            if (mCacheTaskInfo[i].taskId == taskId) {
                mCacheTaskInfo.removeAt(i)
                return
            }
        }
    }

    @Synchronized
    override fun onActivityPinned(packageName: String?, userId: Int, taskId: Int, stackId: Int) {
        mChangeId++
    }

    @Synchronized
    override fun onActivityUnpinned() {
        mChangeId++
    }

    fun deleteTasks() {
        if (mIndex >= mCacheTaskInfo.size) return
        mActivityManagerWrapper?.removeAllRecentTasks()
        mCacheTaskInfo.clear()
        mIndex = 0
    }

    fun deleteCurrentTask() {
        if (mIndex >= mCacheTaskInfo.size) return
        mActivityManagerWrapper?.removeTask(mCacheTaskInfo[mIndex].taskId)
        mCacheTaskInfo.removeAt(mIndex)
        if (mIndex == mCacheTaskInfo.size && mIndex > 0) {
            mIndex--
        }
    }

    fun startActivityFromRecent() {
        if (mIndex >= mCacheTaskInfo.size) return
        mActivityManagerWrapper?.startActivityFromRecentsAsync(
            mCacheTaskInfo[mIndex].taskKey,
            null,
            null,
            null)
    }

    fun onDestroy() {
        mActivityManagerWrapper?.unregisterTaskStackListener(this)
    }

    fun left() {
        if (mIndex < mCacheTaskInfo.size - 1) mIndex++
    }

    fun right() {
        if (mIndex > 0) mIndex--
    }
}