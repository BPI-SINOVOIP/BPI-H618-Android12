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

package com.android.permissioncontroller.permission.ui.handheld

import android.app.Application
import android.content.Context
import android.os.UserHandle
import android.widget.ImageButton
import androidx.preference.PreferenceViewHolder
import com.android.permissioncontroller.R
import com.android.permissioncontroller.permission.ui.RemovablePref

/**
 * A preference which represents an app that is unused. Has the app icon and label, as well as a
 * button to uninstall/disable the app, and a button to open the app.
 *
 * @param app The current application
 * @param packageName The name of the package whose icon this preference will retrieve
 * @param user The user whose package icon will be retrieved
 * @param context The current context
 */
class UnusedAppPreference(
    app: Application,
    packageName: String,
    user: UserHandle,
    context: Context
) : SmartIconLoadPackagePermissionPreference(app, packageName, user, context), RemovablePref {
    private var removeButton: ImageButton? = null
    private var removeRunnable: Runnable? = null

    init {
        widgetLayoutResource = R.xml.uninstall_button_preference_widget
    }

    override fun onBindViewHolder(holder: PreferenceViewHolder) {
        super.onBindViewHolder(holder)

        removeButton = holder.findViewById(R.id.uninstall_button) as ImageButton
        bindRemoveButtonRunnable(removeRunnable)
    }

    override fun setRemoveClickRunnable(runnable: Runnable) {
        removeRunnable = runnable
        bindRemoveButtonRunnable(removeRunnable)
    }

    private fun bindRemoveButtonRunnable(runnable: Runnable?) {
        removeButton?.setOnClickListener {
            runnable?.run()
        }
    }
}