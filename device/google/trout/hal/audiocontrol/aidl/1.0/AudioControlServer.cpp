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

#include "AudioControlServer.h"

#include <deque>
#include <string>
#include <thread>

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <grpc++/grpc++.h>

#include <aidl/android/hardware/automotive/audiocontrol/AudioFocusChange.h>
#include <aidl/android/hardware/automotive/audiocontrol/BnAudioControl.h>
#include <aidl/android/hardware/automotive/audiocontrol/DuckingInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/IFocusListener.h>

#include <android_audio_policy_configuration_V7_0.h>

#include "AudioFocusControl.grpc.pb.h"
#include "AudioFocusControl.pb.h"
#include "libandroid_audio_controller/utils.h"

using std::literals::chrono_literals::operator""s;

namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

using xsd::AudioUsage;

namespace aidl::android::hardware::automotive::audiocontrol {

class AudioControlServerImpl : public AudioControlServer,
                               audio_focus_control_proto::AudioFocusControlServer::Service {
  public:
    explicit AudioControlServerImpl(const std::string& addr);

    ~AudioControlServerImpl();

    close_handle_func_t RegisterFocusListener(std::shared_ptr<IFocusListener> focusListener) override {
        std::lock_guard<std::mutex> lock(mFocusListenerMutex);
        mFocusListener = focusListener;

        return [this, focusListener]() {
            std::lock_guard<std::mutex> lock(mFocusListenerMutex);
            if (mFocusListener == focusListener) {
                mFocusListener = nullptr;
            }
        };
    }

    grpc::Status AudioRequests(::grpc::ServerContext* context,
                               const audio_focus_control_proto::AudioFocusControlMessage* message,
                               ::google::protobuf::Empty*) override;

    void Start() override;

    void Join() override;

  private:
    void RequestWorker();

    void CheckSessionHeartbeats(std::chrono::steady_clock::time_point current_timestamp);

    void HandleHeartbeat(aafc_session_id_t session,
                         std::chrono::steady_clock::time_point timestamp);

    void HandleAcquiring(audio_focus_control_proto::AudioFocusRequest&& acquire_request,
                         std::chrono::steady_clock::time_point timestamp);

    void HandleReleasing(aafc_session_id_t release_session);

    void RequestAudioFocus(aafc_audio_usage_t usage, aafc_zone_id_t zone,
                           AudioFocusChange focus_change);

    void AbandonAudioFocus(aafc_audio_usage_t usage, aafc_zone_id_t zone);

    using grpc_request_t = audio_focus_control_proto::AudioFocusControlMessage;
    using focus_listener_request_key_t = std::pair<aafc_audio_usage_t, aafc_zone_id_t>;

    struct AudioFocusSession {
        audio_focus_control_proto::AudioFocusRequest mRequest;
        std::chrono::steady_clock::time_point mLastHeartbeat;

        focus_listener_request_key_t GetRequestKey() const;
        AudioFocusChange GetFocusChange() const;
    };

    using session_pool_t = std::map<aafc_session_id_t, AudioFocusSession>;

    // data members

    std::string mServiceAddr;
    std::unique_ptr<::grpc::Server> mGrpcServer;
    std::shared_ptr<IFocusListener> mFocusListener{nullptr};

    // grpc request queue
    std::deque<grpc_request_t> mRequestQueue;

    // On the focus listener side, the usage/zone pair is used as the key,
    // and acquiring focus multiple times on the same usage and zone will
    // be treated as once, so we have to maintain the "sessions" and ref count
    // by ourselves here.
    //
    // Active audio focus sessions from grpc clients
    session_pool_t mSessionPool;

    // ref counts of usage/zone pair
    std::map<focus_listener_request_key_t, unsigned> mAudioFocusCount;

    std::atomic<bool> mShutdownFlag{false};
    std::thread mRequestWorker;

    mutable std::mutex mFocusListenerMutex;
    mutable std::mutex mRequestQueueMutex;

    std::condition_variable mRequestQueueCV;
};

static std::shared_ptr<::grpc::ServerCredentials> getServerCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureServerCredentials();
}

AudioControlServerImpl::AudioControlServerImpl(const std::string& addr) : mServiceAddr(addr) {}

AudioControlServerImpl::~AudioControlServerImpl() {
    mShutdownFlag.store(true);
    if (mRequestWorker.joinable()) {
        mRequestWorker.join();
    }
}

void AudioControlServerImpl::Start() {
    if (mGrpcServer) {
        LOG(WARNING) << __func__ << ": GRPC Server is running.";
        return;
    }

    ::grpc::ServerBuilder builder;
    builder.RegisterService(this);
    builder.AddListeningPort(mServiceAddr, getServerCredentials());

    mGrpcServer = builder.BuildAndStart();

    if (!mGrpcServer) {
        LOG(ERROR) << __func__ << ": failed to create the GRPC server, "
                   << "please make sure the configuration and permissions are correct.";
        return;
    }

    mRequestWorker = std::thread(std::bind(&AudioControlServerImpl::RequestWorker, this));
}

void AudioControlServerImpl::Join() {
    if (!mGrpcServer) {
        LOG(WARNING) << __func__ << ": GRPC Server is not running.";
        return;
    }
    mGrpcServer->Wait();
}

grpc::Status AudioControlServerImpl::AudioRequests(
        ::grpc::ServerContext* context,
        const audio_focus_control_proto::AudioFocusControlMessage* message,
        ::google::protobuf::Empty*) {
    {
        std::lock_guard<std::mutex> lock(mRequestQueueMutex);
        mRequestQueue.emplace_back(*message);
    }
    mRequestQueueCV.notify_all();
    return ::grpc::Status::OK;
}

void AudioControlServerImpl::RequestWorker() {
    constexpr auto kCheckHeartbeatFreq = 1s;
    auto nextHeartbeatCheckTime = std::chrono::steady_clock::now();
    while (!mShutdownFlag.load()) {
        std::optional<grpc_request_t> message;
        {
            std::unique_lock<std::mutex> lock(mRequestQueueMutex);
            if (mRequestQueue.empty()) {
                mRequestQueueCV.wait_until(lock, nextHeartbeatCheckTime,
                                           [this]() { return !mRequestQueue.empty(); });
            }
            if (!mRequestQueue.empty()) {
                message = std::move(*mRequestQueue.begin());
                mRequestQueue.pop_front();
            }
        }

        auto current_timestamp = std::chrono::steady_clock::now();
        if (message) {
            for (auto&& active_session : message->active_sessions()) {
                HandleHeartbeat(active_session, current_timestamp);
            }

            for (auto&& acquire_request : *message->mutable_acquire_requests()) {
                HandleAcquiring(std::move(acquire_request), current_timestamp);
            }

            for (auto&& release_session : message->release_requests()) {
                HandleReleasing(release_session);
            }
        }
        if (current_timestamp >= nextHeartbeatCheckTime) {
            nextHeartbeatCheckTime += kCheckHeartbeatFreq;
            CheckSessionHeartbeats(current_timestamp);
        }
    }
}

void AudioControlServerImpl::HandleHeartbeat(aafc_session_id_t session,
                                             std::chrono::steady_clock::time_point timestamp) {
    auto session_search = mSessionPool.find(session);
    if (session_search == mSessionPool.end()) {
        LOG(ERROR) << __func__ << ": unknown session ID: " << session;
        return;
    }
    auto& session_info = session_search->second;
    session_info.mLastHeartbeat = timestamp;
}

void AudioControlServerImpl::HandleAcquiring(
        audio_focus_control_proto::AudioFocusRequest&& acquire_request,
        std::chrono::steady_clock::time_point timestamp) {
    const auto session_id = acquire_request.session_id();
    const auto session_emplace = mSessionPool.emplace(
            session_id, AudioFocusSession{std::move(acquire_request), timestamp});
    if (session_emplace.second == false) {
        LOG(ERROR) << __func__ << ": duplicate session ID: " << session_id;
        return;
    }

    const auto& session_emplace_iter = session_emplace.first;
    const auto& session_info = session_emplace_iter->second;
    const auto request_key = session_info.GetRequestKey();
    const auto focus_change = session_info.GetFocusChange();
    const auto ref_count_search = mAudioFocusCount.find(request_key);
    const auto& [audio_usage, zone_id] = request_key;
    LOG(DEBUG) << __func__ << ": acquiring: " << toString(static_cast<AudioUsage>(audio_usage))
               << " " << zone_id << " " << toString(focus_change);

    const bool not_found = ref_count_search == mAudioFocusCount.end();
    const bool count_zero = !not_found && ref_count_search->second == 0;

    if (count_zero) {
        LOG(WARNING) << __func__ << ": unexcepted unremoved zero ref count, treating as missing.";
    }

    if (not_found || count_zero) {
        mAudioFocusCount[request_key] = 1;
        RequestAudioFocus(audio_usage, zone_id, focus_change);
    } else {
        ++ref_count_search->second;
    }
}

void AudioControlServerImpl::HandleReleasing(aafc_session_id_t release_session) {
    const auto session_search = mSessionPool.find(release_session);
    if (session_search == mSessionPool.end()) {
        LOG(ERROR) << __func__ << ": unknown session ID: " << release_session;
        return;
    }
    const auto& session_info = session_search->second;
    const auto request_key = session_info.GetRequestKey();
    const auto& [audio_usage, zone_id] = request_key;
    mSessionPool.erase(session_search);
    LOG(DEBUG) << __func__ << ": releasing: " << toString(static_cast<AudioUsage>(audio_usage))
               << " " << zone_id;

    const auto ref_count_search = mAudioFocusCount.find(request_key);
    if (ref_count_search == mAudioFocusCount.end()) {
        LOG(ERROR) << __func__ << ": unknown request, audio usage: "
                   << toString(static_cast<AudioUsage>(audio_usage)) << ", zone: " << zone_id;
        return;
    }
    auto& request_ref_count = ref_count_search->second;
    if (--request_ref_count == 0) {
        AbandonAudioFocus(audio_usage, zone_id);
        mAudioFocusCount.erase(ref_count_search);
    }
}

void AudioControlServerImpl::RequestAudioFocus(aafc_audio_usage_t usage, aafc_zone_id_t zone,
                                               AudioFocusChange focus_change) {
    std::lock_guard<std::mutex> lock(mFocusListenerMutex);
    auto listener = mFocusListener;
    const auto audio_usage = static_cast<AudioUsage>(usage);
    LOG(DEBUG) << __func__
               << ": requesting focus, usage: " << toString(audio_usage)
               << ", zone: " << zone
               << ", focus change: " << toString(static_cast<AudioFocusChange>(focus_change));
    if (!listener) {
        LOG(ERROR) << __func__ << ": audio focus listener has not been registered.";
        return;
    }
    listener->requestAudioFocus(toString(audio_usage), zone, focus_change);
}

void AudioControlServerImpl::AbandonAudioFocus(aafc_audio_usage_t usage, aafc_zone_id_t zone) {
    std::lock_guard<std::mutex> lock(mFocusListenerMutex);
    auto listener = mFocusListener;
    const auto audio_usage = static_cast<AudioUsage>(usage);
    LOG(DEBUG) << __func__
               << ": abandoning focus, usage: " << toString(audio_usage)
               << ", zone: " << zone;
    if (!listener) {
        LOG(ERROR) << __func__ << ": audio focus listener has not been registered.";
        return;
    }
    listener->abandonAudioFocus(toString(audio_usage), zone);
}

void AudioControlServerImpl::CheckSessionHeartbeats(
        std::chrono::steady_clock::time_point current_timestamp) {
    constexpr auto kSessionHeartbeatTimeout = 5s;
    const auto timestamp_to_sec = [](auto&& timestamp) {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
                       timestamp.time_since_epoch())
                .count();
    };

    constexpr size_t max_timeout_session_num = 256;
    std::array<aafc_session_id_t, max_timeout_session_num> timeout_sessions;
    size_t num_of_timeout_sessions = 0;

    for (auto&& current_session : mSessionPool) {
        const auto& current_session_id = current_session.first;
        const auto& current_session_info = current_session.second;
        if (current_session_info.mLastHeartbeat + kSessionHeartbeatTimeout < current_timestamp) {
            if (num_of_timeout_sessions >= max_timeout_session_num) {
                LOG(ERROR) << __func__ << ": timeout session number exceeds the limit: "
                           << max_timeout_session_num;
                break;
            }
            LOG(WARNING) << __func__ << ": timeout on session " << current_session_id
                         << ", last heartbeat at "
                         << timestamp_to_sec(current_session_info.mLastHeartbeat)
                         << ", current timestamp is " << timestamp_to_sec(current_timestamp)
                         << ", timeout limit " << kSessionHeartbeatTimeout.count() << "s";
            timeout_sessions[num_of_timeout_sessions++] = current_session_id;
        }
    }

    for (int i = 0; i < num_of_timeout_sessions; ++i) {
        HandleReleasing(timeout_sessions[i]);
    }
}

AudioControlServerImpl::focus_listener_request_key_t
AudioControlServerImpl::AudioFocusSession::GetRequestKey() const {
    return {mRequest.audio_usage(), mRequest.zone_id()};
}

AudioFocusChange AudioControlServerImpl::AudioFocusSession::GetFocusChange() const {
    constexpr auto cast_to_bitfield = [](auto&& focus_change) {
        return static_cast<AudioFocusChange>(focus_change);
    };
    if (!mRequest.is_transient()) {
        return cast_to_bitfield(AudioFocusChange::GAIN);
    }
    if (mRequest.is_exclusive()) {
        return cast_to_bitfield(AudioFocusChange::GAIN_TRANSIENT_EXCLUSIVE);
    }
    if (mRequest.allow_duck()) {
        return cast_to_bitfield(AudioFocusChange::GAIN_TRANSIENT_MAY_DUCK);
    }
    return cast_to_bitfield(AudioFocusChange::GAIN_TRANSIENT);
}

std::unique_ptr<AudioControlServer> MakeAudioControlServer(const std::string& addr) {
    return std::make_unique<AudioControlServerImpl>(addr);
}

}  // namespace aidl::android::hardware::automotive::audiocontrol
