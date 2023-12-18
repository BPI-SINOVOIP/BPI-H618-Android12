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

#include <gtest/gtest.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "MultiPoll.h"

using android::hardware::sensors::V2_0::subhal::implementation::MultiPoll;

// using namespace here is the sanctioned C++ way
// NOLINTNEXTLINE(build/namespaces)
using namespace std::chrono_literals;

class PipeHelper {
  public:
    PipeHelper() {
        int pipefd[2] = {0, 0};
        int err = ::pipe(pipefd);
        if (err == 0) {
            mReadFd = pipefd[0];
            mWriteFd = pipefd[1];
        } else {
            mReadFd = mWriteFd = -1;
        }
    }

    ~PipeHelper() {
        close(mReadFd);
        close(mWriteFd);
    }

    int readFd() const { return mReadFd; }
    int writeFd() const { return mWriteFd; }

    explicit operator bool() const { return (mReadFd >= 0) && (mWriteFd >= 0); }

    size_t read(char* buf, size_t n) { return ::read(mReadFd, reinterpret_cast<void*>(buf), n); }
    size_t write(const char* buf, size_t n) {
        return ::write(mWriteFd, reinterpret_cast<const void*>(buf), n);
    }

  private:
    int mReadFd;
    int mWriteFd;
};

TEST(MultiPollTest, EmptyList) {
    MultiPoll mp(100);
    bool called = false;
    MultiPoll::OnPollIn f = [&called](int) -> void { called = true; };
    mp.poll(f);
    EXPECT_FALSE(called);
}

TEST(MultiPollTest, DataAvailable) {
    MultiPoll mp(100);
    PipeHelper pe;
    ASSERT_TRUE(pe);

    mp.addDescriptor(pe.readFd());
    pe.write("hello", 5);
    bool called = false;
    int poll_fd;
    MultiPoll::OnPollIn f = [&called, &poll_fd](int fd) -> void {
        called = true;
        poll_fd = fd;
    };
    mp.poll(f);
    EXPECT_TRUE(called);
    EXPECT_EQ(poll_fd, pe.readFd());
}

TEST(MultiPollTest, DataComesUpLater) {
    MultiPoll mp(120000 /* 2 minutes */);
    PipeHelper pe;
    ASSERT_TRUE(pe);
    mp.addDescriptor(pe.readFd());

    bool called = false;
    int poll_fd;
    MultiPoll::OnPollIn f = [&called, &poll_fd](int fd) -> void {
        called = true;
        poll_fd = fd;
    };
    std::thread pollerThread([&mp, &f]() -> void { mp.poll(f); });

    std::this_thread::sleep_for(100ms);
    pe.write("hello", 5);

    pollerThread.join();
    EXPECT_TRUE(called);
    EXPECT_EQ(poll_fd, pe.readFd());
}

TEST(MultiPollTest, OneFdHasData) {
    MultiPoll mp(100);
    PipeHelper p1;
    PipeHelper p2;

    mp.addDescriptor(p1.readFd());
    mp.addDescriptor(p2.readFd());

    int called = 0;
    MultiPoll::OnPollIn f = [&called](int) -> void { ++called; };

    p1.write("hello", 5);
    mp.poll(f);
    EXPECT_EQ(1, called);
}

TEST(MultiPollTest, TwoFdHaveData) {
    MultiPoll mp(100);
    PipeHelper p1;
    PipeHelper p2;

    mp.addDescriptor(p1.readFd());
    mp.addDescriptor(p2.readFd());

    int called = 0;
    int prev_fd = -1;
    bool repeat_fd = false;
    MultiPoll::OnPollIn f = [&called, &prev_fd, &repeat_fd](int fd) -> void {
        ++called;
        if (prev_fd == fd) repeat_fd = true;
        prev_fd = fd;
    };

    p1.write("hello", 5);
    p2.write("hi", 2);
    mp.poll(f);
    EXPECT_EQ(2, called);
    EXPECT_FALSE(repeat_fd);
    EXPECT_TRUE(prev_fd == p1.readFd() || prev_fd == p2.readFd());
}

TEST(MultiPollTest, ZeroWait) {
    MultiPoll mp(0);
    PipeHelper pe;
    ASSERT_TRUE(pe);
    mp.addDescriptor(pe.readFd());

    bool called = false;
    int poll_fd;
    MultiPoll::OnPollIn f = [&called, &poll_fd](int fd) -> void {
        called = true;
        poll_fd = fd;
    };
    std::thread pollerThread([&mp, &f, &called]() -> void {
        while (!called) mp.poll(f);
    });

    std::this_thread::sleep_for(100ms);
    pe.write("hello", 5);

    pollerThread.join();
    EXPECT_TRUE(called);
    EXPECT_EQ(poll_fd, pe.readFd());
}

TEST(MultiPollTest, AddOneLater) {
    MultiPoll mp(100);
    PipeHelper p1;
    PipeHelper p2;

    mp.addDescriptor(p1.readFd());

    bool called = false;
    int poll_fd;
    MultiPoll::OnPollIn f = [&called, &poll_fd](int fd) -> void {
        called = true;
        poll_fd = fd;
    };

    std::thread pollerThread([&mp, &f, &called]() -> void {
        while (!called) mp.poll(f);
    });

    std::this_thread::sleep_for(250ms);
    mp.addDescriptor(p2.readFd());
    std::this_thread::sleep_for(100ms);
    p2.write("hello", 5);

    pollerThread.join();
    EXPECT_TRUE(called);
    EXPECT_EQ(p2.readFd(), poll_fd);
}
