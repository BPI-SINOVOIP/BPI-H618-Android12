/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.permission3.cts.accessmicrophoneapp2

import android.app.Activity
import android.app.AppOpsManager
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Handler

private const val USE_DURATION_MS = 10000L
private const val SAMPLE_RATE_HZ = 44100

class AccessMicActivity2 : Activity() {
    private var recorder: AudioRecord? = null
    private var appOpsManager: AppOpsManager? = null

    override fun onStart() {
        super.onStart()
        useMic()
    }

    override fun finish() {
        recorder?.stop()
        recorder = null
        appOpsManager = null
        super.finish()
    }

    override fun onStop() {
        super.onStop()
        finish()
    }

    private fun useMic() {
        val minSize =
                AudioRecord.getMinBufferSize(SAMPLE_RATE_HZ, AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT)
        recorder = AudioRecord(MediaRecorder.AudioSource.MIC, SAMPLE_RATE_HZ,
                AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, minSize)
        recorder?.startRecording()
        Handler().postDelayed({ finish() }, USE_DURATION_MS)
    }
}