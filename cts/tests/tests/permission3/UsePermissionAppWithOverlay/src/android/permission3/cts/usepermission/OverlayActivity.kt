package android.permission3.cts.usepermission

import android.app.Activity
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.view.Gravity
import android.view.WindowManager

class OverlayActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.overlay_activity)
        val params = window.attributes
        params.flags = (WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS or
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL or
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        if (!intent.getBooleanExtra(EXTRA_FULL_OVERLAY, true)) {
            params.gravity = Gravity.LEFT or Gravity.TOP
            val left = intent.getIntExtra(DIALOG_LEFT, params.x)
            val top = intent.getIntExtra(DIALOG_TOP, params.y)
            val right = intent.getIntExtra(DIALOG_RIGHT, params.x + params.width)
            val bottom = intent.getIntExtra(MESSAGE_BOTTOM, top + 1)
            params.x = left
            params.y = top
            params.width = right - left
            params.height = bottom - top
        }

        registerReceiver(object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                if (intent?.action != RequestPermissionsActivity.ACTION_HIDE_OVERLAY) {
                    return
                }

                finish()
            }
        }, IntentFilter(RequestPermissionsActivity.ACTION_HIDE_OVERLAY))
    }

    companion object {
        const val EXTRA_FULL_OVERLAY = "android.permission3.cts.usepermission.extra.FULL_OVERLAY"

        const val DIALOG_LEFT = "android.permission3.cts.usepermission.extra.DIALOG_LEFT"
        const val DIALOG_TOP = "android.permission3.cts.usepermission.extra.DIALOG_TOP"
        const val DIALOG_RIGHT = "android.permission3.cts.usepermission.extra.DIALOG_RIGHT"
        const val MESSAGE_BOTTOM = "android.permission3.cts.usepermission.extra.MESSAGE_BOTTOM"
    }
}