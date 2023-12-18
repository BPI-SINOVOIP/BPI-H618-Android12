package com.softwinner.einklaunchrecent

import android.app.ActivityTaskManager
import android.content.Intent
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Process
import android.util.DisplayMetrics
import android.util.Log
import android.view.View
import android.widget.ImageView
import android.widget.RelativeLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.cardview.widget.CardView
import androidx.constraintlayout.widget.ConstraintLayout
import com.softwinner.einklaunchrecent.model.TaskData
import com.softwinner.einklaunchrecent.model.TaskLoader
import com.softwinner.einklaunchrecent.service.TouchService
import com.softwinner.einklaunchrecent.util.utils
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {

    private val TAG = utils.TAG

    private var mTaskLoader: TaskLoader? = null
    private lateinit var mLeft: ImageView
    private lateinit var mRight: ImageView
    private lateinit var mSnapshotIcon: ImageView
    private lateinit var mSnapshot: ImageView
    private lateinit var mDeleteCurrentTask: ImageView
    private lateinit var mDeleteAllTask: ImageView
    private lateinit var mNoTask: RelativeLayout
    private lateinit var mCardView: CardView
    private lateinit var mTaskView: ConstraintLayout

    private val LEFT = 0
    private val RIGHT = 1
    private val DELETE = 2
    private val DELETE_ALL = 3
    private val INIT = 4

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        parseIntent(intent)
        initData()
        initView()
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        parseIntent(intent)
    }

    private fun parseIntent(intent: Intent?) {
        if (!TouchService.mShouldStart) {
            finish()
        }
    }

    private fun initView() {
        mLeft = findViewById(R.id.left)
        mRight = findViewById(R.id.right)
        mSnapshotIcon = findViewById(R.id.snapshot_icon)
        mSnapshot = findViewById(R.id.snapshot)
        mDeleteCurrentTask = findViewById(R.id.delete_current_task)
        mDeleteAllTask = findViewById(R.id.delete_all_task)
        mNoTask = findViewById(R.id.no_task)
        mCardView = findViewById(R.id.task_area)
        mTaskView = findViewById(R.id.task_bg)

        mTaskLoader?.mCacheTaskInfo?.also {
            Log.w(TAG, "it ${it.size}")
            updateContent(INIT)
            mLeft.setOnClickListener {
                updateContent(LEFT)
            }

            mRight.setOnClickListener {
                updateContent(RIGHT)
            }

            mDeleteCurrentTask.setOnClickListener {
                mTaskLoader?.deleteCurrentTask()
                updateContent(DELETE)
            }

            mDeleteAllTask.setOnClickListener {
                mTaskLoader?.deleteTasks()
                updateContent(DELETE_ALL)
            }

            mSnapshot.setOnClickListener {
                openActivity()
            }
        }
    }

    private fun initData() {
        mTaskLoader = TaskLoader(this)
        mTaskLoader?.updateTasks(Process.myUserHandle().identifier)
    }

    override fun onResume() {
        synchronized(TouchService.mLock) {
            TouchService.mShouldStart = false
        }
        mTaskLoader?.updateTasks(Process.myUserHandle().identifier)
        super.onResume()
    }

    private fun updateContent(status: Int) {
        Log.w(TAG, "mTaskLoader ${mTaskLoader!!.mIndex}")
        when(status) {
            DELETE_ALL -> {
                updateContent(null)
                return
            }
            LEFT -> {
                mTaskLoader?.left()
            }
            RIGHT -> {
                mTaskLoader?.right()
            }
            INIT -> {
                // do nothing
            }
        }
        updateContent(mTaskLoader!!.getCurrentTask())
        updateView()
    }

    private fun updateView() {
        if (mTaskLoader == null) return
        if (mTaskLoader!!.mCacheTaskInfo.size == 0) {
            mNoTask.visibility = View.VISIBLE
            mTaskView.visibility = View.INVISIBLE
            return
        }

        mRight.visibility = if (mTaskLoader!!.mIndex > 0)  View.VISIBLE else  View.INVISIBLE
        mLeft.visibility =
            if (mTaskLoader!!.mIndex < mTaskLoader!!.mCacheTaskInfo.size - 1) View.VISIBLE
            else View.INVISIBLE
    }

    private fun updateContent(taskData: TaskData?) {
        mSnapshotIcon.setImageBitmap(taskData?.icon)
        mSnapshot.setImageBitmap(taskData?.snapshot)
    }

    private fun openActivity() {
        mTaskLoader?.startActivityFromRecent()
    }

    override fun onPause() {
        synchronized(TouchService.mLock) {
            TouchService.mShouldStart = true
        }
        mTaskLoader?.onDestroy()
        super.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
    }
}