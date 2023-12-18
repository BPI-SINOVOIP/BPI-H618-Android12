/*
 * Copyright 2020 The Android Open Source Project
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
#include <android/log.h>

#include "OboePlayer.h"
#include "WaveTableSource.h"

#include "AudioSource.h"

static const char * const TAG = "OboePlayer(native)";

using namespace oboe;

constexpr int32_t kBufferSizeInBursts = 2; // Use 2 bursts as the buffer size (double buffer)

OboePlayer::OboePlayer(JNIEnv *env, AudioSource* source, int subtype)
 : Player(source, subtype)
{
    env->GetJavaVM(&mJvm);

    jclass clsAudioTimestamp = env->FindClass("android/media/AudioTimestamp");

    mFidFramePosition = env->GetFieldID(clsAudioTimestamp, "framePosition", "J");
    mFidNanoTime = env->GetFieldID(clsAudioTimestamp, "nanoTime", "J");
}

DataCallbackResult OboePlayer::onAudioReady(AudioStream *oboeStream, void *audioData,
                                            int32_t numFrames) {
    StreamState streamState = oboeStream->getState();
    if (streamState != StreamState::Open && streamState != StreamState::Started) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "  streamState:%d", streamState);
    }
    if (streamState == StreamState::Disconnected) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "  streamState::Disconnected");
    }

    // memset(audioData, 0, numFrames * mChannelCount * sizeof(float));

    // Pull the data here!
    int numFramesRead = mAudioSource->pull((float*)audioData, numFrames, mChannelCount);
    // may need to handle 0-filling if numFramesRead < numFrames

    return numFramesRead != 0 ? DataCallbackResult::Continue : DataCallbackResult::Stop;
}

void OboePlayer::onErrorAfterClose(AudioStream *oboeStream, oboe::Result error) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "==== onErrorAfterClose() error:%d", error);

    startStream();
}

void OboePlayer::onErrorBeforeClose(AudioStream *, oboe::Result error) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "==== onErrorBeforeClose() error:%d", error);
}

StreamBase::Result OboePlayer::setupStream(int32_t channelCount, int32_t sampleRate, int32_t routeDeviceId) {

    oboe::Result result = oboe::Result::ErrorInternal;
    if (mAudioStream != nullptr) {
        return ERROR_INVALID_STATE;
    } else {
        std::lock_guard<std::mutex> lock(mStreamLock);

        mChannelCount = channelCount;
        mSampleRate = sampleRate;
        mRouteDeviceId = routeDeviceId;

        // Create an audio stream
        AudioStreamBuilder builder;
        builder.setChannelCount(mChannelCount);
        builder.setSampleRate(mSampleRate);
        builder.setCallback(this);
        builder.setPerformanceMode(PerformanceMode::LowLatency);
        builder.setSharingMode(SharingMode::Exclusive);
        builder.setSampleRateConversionQuality(SampleRateConversionQuality::None);
        builder.setDirection(Direction::Output);
        switch (mSubtype) {
        case SUB_TYPE_OBOE_AAUDIO:
            builder.setAudioApi(AudioApi::AAudio);
            break;

        case SUB_TYPE_OBOE_OPENSL_ES:
            builder.setAudioApi(AudioApi::OpenSLES);
            break;
        }

        if (mRouteDeviceId != ROUTING_DEVICE_NONE) {
            builder.setDeviceId(mRouteDeviceId);
        }

        result = builder.openStream(mAudioStream);
        if (result != oboe::Result::OK){
            __android_log_print(
                    ANDROID_LOG_ERROR,
                    TAG,
                    "openStream failed. Error: %s", convertToText(result));
        } else {
            // Reduce stream latency by setting the buffer size to a multiple of the burst size
            // Note: this will fail with ErrorUnimplemented if we are using a callback with OpenSL ES
            // See oboe::AudioStreamBuffered::setBufferSizeInFrames
            // This doesn't affect the success of opening the stream.
            int32_t desiredSize = mAudioStream->getFramesPerBurst() * kBufferSizeInBursts;
            mAudioStream->setBufferSizeInFrames(desiredSize);

            mAudioSource->init(desiredSize , mChannelCount);
        }
    }

    return OboeErrorToMegaAudioError(result);
}

StreamBase::Result OboePlayer::startStream() {
    StreamBase::Result result = Player::startStream();

    return result;
}

bool OboePlayer::getJavaTimestamp(jobject timestampObj) {
    oboe::FrameTimestamp nativeStamp;
    StreamBase::Result result = Player::getTimeStamp(&nativeStamp);
    if (result == OK) {
        JNIEnv* env;
        mJvm->AttachCurrentThread(&env, NULL);

        env->SetLongField(timestampObj, mFidFramePosition, nativeStamp.position);
        env->SetLongField(timestampObj, mFidNanoTime, nativeStamp.timestamp);
    }

    return result == OK;
}

//
// JNI functions
//
#include <jni.h>

#include <android/log.h>

extern "C" {
JNIEXPORT JNICALL jlong
Java_org_hyphonate_megaaudio_player_OboePlayer_allocNativePlayer(
    JNIEnv *env, jobject thiz, jlong native_audio_source, jint playerSubtype) {

    return (jlong)new OboePlayer(env, (AudioSource*)native_audio_source, playerSubtype);
}

JNIEXPORT jint JNICALL Java_org_hyphonate_megaaudio_player_OboePlayer_setupStreamN(
        JNIEnv *env, jobject thiz, jlong native_player,
        jint channel_count, jint sample_rate, jint routeDeviceId) {

    OboePlayer* player = (OboePlayer*)native_player;
    return player->setupStream(channel_count, sample_rate, routeDeviceId);
}

JNIEXPORT int JNICALL Java_org_hyphonate_megaaudio_player_OboePlayer_teardownStreamN(
        JNIEnv *env, jobject thiz, jlong native_player) {

    OboePlayer* player = (OboePlayer*)native_player;
    return player->teardownStream();
}

JNIEXPORT JNICALL jint Java_org_hyphonate_megaaudio_player_OboePlayer_startStreamN(
        JNIEnv *env, jobject thiz, jlong native_player, jint playerSubtype) {

    return ((OboePlayer*)(native_player))->startStream();
}

JNIEXPORT JNICALL jint
Java_org_hyphonate_megaaudio_player_OboePlayer_stopN(JNIEnv *env, jobject thiz, jlong native_player) {

   return ((OboePlayer*)(native_player))->stopStream();
}

JNIEXPORT jint JNICALL
Java_org_hyphonate_megaaudio_player_OboePlayer_getBufferFrameCountN(JNIEnv *env, jobject thiz,  jlong native_player) {
    return ((OboePlayer*)(native_player))->getNumBufferFrames();
}

JNIEXPORT jint JNICALL Java_org_hyphonate_megaaudio_player_OboePlayer_getRoutedDeviceIdN(JNIEnv *env, jobject thiz, jlong native_player) {
    return ((OboePlayer*)(native_player))->getRoutedDeviceId();
}

JNIEXPORT jboolean JNICALL Java_org_hyphonate_megaaudio_player_OboePlayer_getTimestampN(JNIEnv *env, jobject thiz, jlong native_player, jobject timestamp) {
    return ((OboePlayer*)native_player)->getJavaTimestamp(timestamp);
}

} // extern "C"
