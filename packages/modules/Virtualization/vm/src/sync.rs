// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Synchronisation utilities.

use std::sync::{Arc, Condvar, Mutex};

/// A flag which one thread can use to notify other threads when a condition becomes true. This is
/// something like a single-use binary semaphore.
#[derive(Clone, Debug)]
pub struct AtomicFlag {
    state: Arc<(Mutex<bool>, Condvar)>,
}

impl Default for AtomicFlag {
    #[allow(clippy::mutex_atomic)]
    fn default() -> Self {
        Self { state: Arc::new((Mutex::new(false), Condvar::new())) }
    }
}

#[allow(clippy::mutex_atomic)]
impl AtomicFlag {
    /// Wait until the flag is set.
    pub fn wait(&self) {
        let _flag = self.state.1.wait_while(self.state.0.lock().unwrap(), |flag| !*flag).unwrap();
    }

    /// Set the flag, and notify all waiting threads.
    pub fn raise(&self) {
        let mut flag = self.state.0.lock().unwrap();
        *flag = true;
        self.state.1.notify_all();
    }
}
