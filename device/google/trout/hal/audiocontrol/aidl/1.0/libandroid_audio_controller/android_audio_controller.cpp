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

#include "android_audio_controller.h"

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <grpc++/grpc++.h>
#include "AudioFocusControl.grpc.pb.h"
#include "AudioFocusControl.pb.h"

namespace android::hardware::automotive::audiocontrol::V2_0::implementation {

using std::literals::chrono_literals::operator""s;

static pid_t getCurrentThreadID() {
#ifdef gettid
    return gettid();
#elif defined(SYS_gettid)
    return syscall(SYS_gettid);
#else
    return getpid();
#endif
}

class AudioFocusControllerImpl {
  public:
    static AudioFocusControllerImpl* GetInstance();

    int SetServerAddr(const std::string& addr);

    aafc_session_id_t AcquireFocus(aafc_audio_focus_request_t&& request);

    void ReleaseFocus(aafc_session_id_t session_id);

  private:
    AudioFocusControllerImpl();

    ~AudioFocusControllerImpl();

    AudioFocusControllerImpl(const AudioFocusControllerImpl&) = delete;

    AudioFocusControllerImpl& operator=(const AudioFocusControllerImpl&) = delete;

    AudioFocusControllerImpl(AudioFocusControllerImpl&&) = delete;

    struct AudioFocusRequest {
        aafc_session_id_t session_id;
        aafc_audio_focus_request_t request;
    };

    void RequestWorker();

    static aafc_session_id_t GetNewUniqueSessionID();

    // data members

    mutable std::mutex mMutex;
    std::condition_variable mRequestWorkerCV;

    std::thread mRequestWorkerThread;
    std::set<aafc_session_id_t> mActiveSessions;
    std::vector<AudioFocusRequest> mAudioFocusRequests;
    std::vector<aafc_session_id_t> mSessionsReleaseRequests;

    std::atomic<bool> mShutdownFlag{false};

    std::string mServiceAddr;
    std::shared_ptr<::grpc::Channel> mGrpcChannel;
    std::unique_ptr<audio_focus_control_proto::AudioFocusControlServer::Stub> mGrpcStub;
};

static std::shared_ptr<::grpc::ChannelCredentials> getChannelCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureChannelCredentials();
}

static void validateRequest(aafc_audio_focus_request_t* request) {
    if (!request) {
        std::cerr << "Validate null request is a no-op";
        return;
    }
    if (!request->is_transient && (request->allow_duck || request->is_exclusive)) {
        std::cerr << "If request is not transient, allow_duck and "
                     "exclusive options will be ignored."
                  << std::endl;
    } else if (request->allow_duck && request->is_exclusive) {
        std::cerr << "allow_duck and is_exclusive cannot be set together, "
                     "disabled ducking."
                  << std::endl;
        request->allow_duck = false;
    }
}

AudioFocusControllerImpl::AudioFocusControllerImpl()
    : mRequestWorkerThread(std::bind(&AudioFocusControllerImpl::RequestWorker, this)) {}

AudioFocusControllerImpl::~AudioFocusControllerImpl() {
    mShutdownFlag.store(true);
    if (mRequestWorkerThread.joinable()) {
        mRequestWorkerThread.join();
    }
}

int AudioFocusControllerImpl::SetServerAddr(const std::string& addr) {
    if (addr.empty()) {
        std::cerr << "Error: Server address cannot be empty." << std::endl;
        return -EINVAL;
    }

    // Although the server settings are guarded by mutex, it is still not safe to
    // run concurrently with acquiring/releasing focus, or with active sessions,
    // since the grpc operations in them are not guarded.
    std::lock_guard<std::mutex> lock(mMutex);

    mServiceAddr = addr;
    mGrpcChannel = ::grpc::CreateChannel(mServiceAddr, getChannelCredentials());
    mGrpcStub = audio_focus_control_proto::AudioFocusControlServer::NewStub(mGrpcChannel);

    return 0;
}

AudioFocusControllerImpl* AudioFocusControllerImpl::GetInstance() {
    static AudioFocusControllerImpl instance;
    return &instance;
}

aafc_session_id_t AudioFocusControllerImpl::GetNewUniqueSessionID() {
    static const auto tid = static_cast<uint64_t>(getCurrentThreadID());

    // 48 bits for timestamp (in nanoseconds), so a session ID
    // within a thread is guaranteed not to reappear in about 3 days,
    // which is much longer than any audio session should be.
    //
    // 16 bits for tid (65536 threads)
    constexpr auto use_timestamp_bits = 48;
    aafc_session_id_t session_id = AAFC_SESSION_ID_INVALID;

    do {
        uint64_t timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        session_id = (tid << use_timestamp_bits) | (timestamp & ((1ull << use_timestamp_bits) - 1));
    } while (session_id == AAFC_SESSION_ID_INVALID);

    return session_id;
}

aafc_session_id_t AudioFocusControllerImpl::AcquireFocus(aafc_audio_focus_request_t&& request) {
    validateRequest(&request);

    auto session_id = AAFC_SESSION_ID_INVALID;

    {
        const std::lock_guard<std::mutex> lock(mMutex);

        if (mServiceAddr.empty()) {
            std::cerr << "Uninitialized Controller." << std::endl;
            return session_id;
        }

        constexpr int max_attempt_times = 5;
        for (int i = 0; i < max_attempt_times; ++i) {
            auto session_id_candicate = GetNewUniqueSessionID();
            auto session_id_insert = mActiveSessions.insert(session_id_candicate);
            if (session_id_insert.second) {
                session_id = session_id_candicate;
                break;
            }
        }

        if (session_id == AAFC_SESSION_ID_INVALID) {
            return session_id;
        }

        mAudioFocusRequests.emplace_back((AudioFocusRequest){
                .session_id = session_id,
                .request = std::move(request),
        });
    }

    mRequestWorkerCV.notify_all();

    return session_id;
}

void AudioFocusControllerImpl::ReleaseFocus(aafc_session_id_t session_id) {
    if (!session_id) {
        return;
    }

    {
        const std::lock_guard<std::mutex> lock(mMutex);
        auto session_id_itr = mActiveSessions.find(session_id);
        if (session_id_itr == mActiveSessions.end()) {
            std::cerr << "Unknown session ID: " << session_id << std::endl;
            return;
        }
        mActiveSessions.erase(session_id_itr);
        mSessionsReleaseRequests.emplace_back(session_id);
    }

    mRequestWorkerCV.notify_all();
}

void AudioFocusControllerImpl::RequestWorker() {
    auto nextHeartbeatTime = std::chrono::steady_clock::now();
    auto heartBeatPeriod = 1s;

    const auto ok_to_proceed = [this, &nextHeartbeatTime]() {
        auto current_timestamp = std::chrono::steady_clock::now();
        return !mAudioFocusRequests.empty() || !mSessionsReleaseRequests.empty() ||
               (current_timestamp > nextHeartbeatTime && !mActiveSessions.empty());
    };
    while (!mShutdownFlag.load()) {
        audio_focus_control_proto::AudioFocusControlMessage audio_requests;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            if (!mRequestWorkerCV.wait_until(lock, nextHeartbeatTime, ok_to_proceed)) {
                nextHeartbeatTime = std::chrono::steady_clock::now() + heartBeatPeriod;
                continue;
            }

            auto current_timestamp = std::chrono::steady_clock::now();
            if (current_timestamp > nextHeartbeatTime) {
                nextHeartbeatTime = current_timestamp + 1s;
                for (auto session_id : mActiveSessions) {
                    audio_requests.add_active_sessions(session_id);
                }
            }

            for (auto& request : mAudioFocusRequests) {
                auto& acquire_request = *audio_requests.add_acquire_requests();
                acquire_request.set_session_id(request.session_id);
                acquire_request.set_audio_usage(request.request.audio_usage);
                acquire_request.set_zone_id(request.request.zone_id);
                acquire_request.set_allow_duck(request.request.allow_duck);
                acquire_request.set_is_transient(request.request.is_transient);
                acquire_request.set_is_exclusive(request.request.is_exclusive);
            }
            for (auto session_id : mSessionsReleaseRequests) {
                audio_requests.add_release_requests(session_id);
            }
            mAudioFocusRequests.clear();
            mSessionsReleaseRequests.clear();
        }

        constexpr int max_attempt_times = 3;
        constexpr auto wait_time_between_attempts = 1s;
        ::grpc::Status grpc_status;
        for (int current_attempt = 1; current_attempt <= max_attempt_times; ++current_attempt) {
            ::grpc::ClientContext context;
            ::google::protobuf::Empty empty_retval;
            grpc_status = mGrpcStub->AudioRequests(&context, audio_requests, &empty_retval);
            if (grpc_status.ok()) {
                break;
            }
            std::cerr << "(Attempt " << current_attempt << "/" << max_attempt_times
                      << ") Failed to send audio requests: " << grpc_status.error_message()
                      << std::endl;
            std::this_thread::sleep_for(wait_time_between_attempts);
        }
        if (!grpc_status.ok()) {
            std::cerr << "Failed to send audio requests. Please check the server address setting "
                         "and make sure the server is running."
                      << std::endl;
        }
    }
}

}  // namespace android::hardware::automotive::audiocontrol::V2_0::implementation

using android::hardware::automotive::audiocontrol::V2_0::implementation::AudioFocusControllerImpl;

int aafc_init_audio_focus_controller(const char* audio_control_server_addr) {
    return AudioFocusControllerImpl::GetInstance()->SetServerAddr(audio_control_server_addr);
}

aafc_session_id_t aafc_acquire_audio_focus(aafc_audio_focus_request_t request) {
    return AudioFocusControllerImpl::GetInstance()->AcquireFocus(std::move(request));
}

void aafc_release_audio_focus(aafc_session_id_t session_id) {
    return AudioFocusControllerImpl::GetInstance()->ReleaseFocus(session_id);
}
