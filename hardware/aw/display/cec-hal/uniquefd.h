/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef _sunxi_unique_fd_h_
#define _sunxi_unique_fd_h_

#include <unistd.h>

namespace sunxi {

class uniquefd {
public:
    uniquefd() = default;
    ~uniquefd() { reset(); }

    explicit uniquefd(int fd) { reset(fd); }
    uniquefd(uniquefd &&rhs) noexcept { reset(rhs.release()); }
    uniquefd &operator=(uniquefd &&rhs) noexcept {
        int fd = rhs.release();
        reset(fd);
        return *this;
    }

    int get() const { return fd_; }
    operator int() const { return get(); }

    bool operator!() const = delete;

    int release() __attribute__((warn_unused_result)) {
        int ret = fd_;
        fd_ = -1;
        return ret;
    }

private:
    void reset(int newfd  = -1) {
        if (fd_ != -1)
            ::close(fd_);
        fd_ = newfd;
    }

    int fd_ = -1;

    uniquefd(const uniquefd&);
    void operator=(const uniquefd&);
};

}  // namespace sunxi

#endif
