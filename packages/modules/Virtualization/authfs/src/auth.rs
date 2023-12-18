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

use std::io;

// TODO(b/170494765): Implement an authenticator to verify a PKCS#7 signature. We only need to
// verify the signature, not the full certificate chain.

pub trait Authenticator {
    fn verify(&self, signature: &[u8], signed_data: &[u8]) -> io::Result<bool>;
}

pub struct FakeAuthenticator {
    should_allow: bool,
}

#[allow(dead_code)]
impl FakeAuthenticator {
    pub fn always_succeed() -> Self {
        FakeAuthenticator { should_allow: true }
    }

    pub fn always_fail() -> Self {
        FakeAuthenticator { should_allow: false }
    }
}

impl Authenticator for FakeAuthenticator {
    fn verify(&self, _signature_pem: &[u8], _signed_data: &[u8]) -> io::Result<bool> {
        Ok(self.should_allow)
    }
}
