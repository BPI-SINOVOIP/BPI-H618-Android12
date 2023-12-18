package com.softwinner.einklaunchrecent.model

import android.content.ComponentName
import android.graphics.Bitmap
import com.android.systemui.shared.recents.model.Task.TaskKey

class TaskData(var taskId: Int,
               var sourceComponent: ComponentName?,
               var icon: Bitmap?,
               var snapshot: Bitmap?,
               var lastActiveTime: Long,
               var displayId: Int,
               var taskKey: TaskKey?)