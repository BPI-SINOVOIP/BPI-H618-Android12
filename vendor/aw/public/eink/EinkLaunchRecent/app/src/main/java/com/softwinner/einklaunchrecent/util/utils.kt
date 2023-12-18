package com.softwinner.einklaunchrecent.util

import android.content.Context
import android.os.Looper
import android.util.Log
import android.view.Gravity
import android.widget.Toast

object utils {
    val DEBUG = true
    val TAG = "EINK_RECENT"
    val showToast = false

    fun megReport(context: Context, msg: String) {
        Log.w(TAG, msg)
        handleLog(context, msg)
    }

    private fun handleLog(context: Context, msg: String) {
        if (showToast) Toast.makeText(context, msg, Toast.LENGTH_SHORT).also {
                it.setGravity(Gravity.BOTTOM,0,0)
            }.show()
    }

}