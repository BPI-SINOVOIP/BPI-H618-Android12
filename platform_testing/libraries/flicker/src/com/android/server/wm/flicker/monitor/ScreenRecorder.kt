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

package com.android.server.wm.flicker.monitor

import android.content.Context
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.MediaCodecInfo
import android.media.MediaRecorder
import android.media.Metadata.VIDEO_FRAME_RATE
import android.os.SystemClock
import android.util.DisplayMetrics
import android.util.Log
import android.view.Surface
import android.view.WindowManager
import com.android.server.wm.flicker.FLICKER_TAG
import com.android.server.wm.flicker.FlickerRunResult
import java.nio.file.Files
import java.nio.file.Path

/** Captures screen contents and saves it as a mp4 video file.  */
open class ScreenRecorder @JvmOverloads constructor(
    outputPath: Path,
    context: Context,
    private val maxDurationMs: Int = MAX_DURATION_MS,
    private val maxFileSizeBytes: Long = MAX_FILESIZE_BYTES,
    private val width: Int = 720,
    private val height: Int = 1280,
    traceFile: String = "transition.mp4"
) : TraceMonitor(outputPath, outputPath.resolve(traceFile)) {
    private val displayManager = context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
    private val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    private var mediaRecorder: MediaRecorder? = null
    private var inputSurface: Surface? = null
    private var virtualDisplay: VirtualDisplay? = null
    private fun prepare() {
        // Set up media recorder
        val recorder = MediaRecorder()

        // Set up audio source
        recorder.setVideoSource(MediaRecorder.VideoSource.SURFACE)
        recorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)

        // Set up video
        val metrics = DisplayMetrics()
        windowManager.defaultDisplay.getRealMetrics(metrics)
        val refreshRate = windowManager.defaultDisplay.refreshRate.toInt()
        val vidBitRate = (width * height * refreshRate / VIDEO_FRAME_RATE
            * VIDEO_FRAME_RATE_TO_RESOLUTION_RATIO)
        recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264)
        recorder.setVideoEncodingProfileLevel(
            MediaCodecInfo.CodecProfileLevel.AVCProfileHigh,
            MediaCodecInfo.CodecProfileLevel.AVCLevel3)
        recorder.setVideoSize(width, height)
        recorder.setVideoFrameRate(refreshRate)
        recorder.setVideoEncodingBitRate(vidBitRate)
        recorder.setMaxDuration(maxDurationMs)
        recorder.setMaxFileSize(maxFileSizeBytes)
        recorder.setOutputFile(sourceTraceFilePath.toFile())
        recorder.prepare()

        // Create surface
        inputSurface = recorder.surface
        virtualDisplay = displayManager.createVirtualDisplay(
            "Recording Display",
            width,
            height,
            metrics.densityDpi,
            inputSurface,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            null,
            null)

        this.mediaRecorder = recorder
    }

    override fun start() {
        if (mediaRecorder != null) {
            Log.i(FLICKER_TAG, "Screen recorder already running")
            return
        }
        Files.deleteIfExists(sourceTraceFilePath)
        require(!Files.exists(sourceTraceFilePath)) { "Could not delete old trace file" }
        Files.createDirectories(sourceTraceFilePath.parent)

        prepare()
        Log.d(FLICKER_TAG, "Starting screen recording to file $sourceTraceFilePath")
        mediaRecorder?.start()

        var remainingTime = WAIT_TIMEOUT_MS
        do {
            SystemClock.sleep(WAIT_INTERVAL_MS)
            remainingTime -= WAIT_INTERVAL_MS
        } while (!Files.exists(sourceTraceFilePath) && remainingTime > 0)

        require(Files.exists(sourceTraceFilePath)) {
            "Screen recorder didn't start" }
    }

    override fun stop() {
        if (mediaRecorder == null) {
            Log.i(FLICKER_TAG, "Screen recorder was not started")
            return
        }

        Log.d(FLICKER_TAG, "Stopping screen recording. Storing result in $sourceTraceFilePath")
        try {
            mediaRecorder?.stop()
            mediaRecorder?.release()
            inputSurface?.release()
            virtualDisplay?.release()
        } catch (e: Exception) {
            throw IllegalStateException("Unable to stop screen recording", e)
        } finally {
            mediaRecorder = null
        }
    }

    override val isEnabled: Boolean
        get() = mediaRecorder != null

    override fun setResult(flickerRunResultBuilder: FlickerRunResult.Builder, traceFile: Path) {
        flickerRunResultBuilder.screenRecording = traceFile
    }

    override fun toString(): String {
        return "ScreenRecorder($sourceTraceFilePath)"
    }

    companion object {
        private const val WAIT_TIMEOUT_MS = 5000L
        private const val WAIT_INTERVAL_MS = 500L
        private const val MAX_DURATION_MS = 60 * 60 * 1000
        private const val MAX_FILESIZE_BYTES = 100000000L
        /**
         * From [ScreenMediaRecorder#VIDEO_FRAME_RATE_TO_RESOLUTION_RATIO]
         */
        private const val VIDEO_FRAME_RATE_TO_RESOLUTION_RATIO = 6
    }
}
