//
// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//! ProfCollect trace provider trait and helper functions.

use anyhow::{anyhow, Result};
use chrono::Utc;
use std::path::Path;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use crate::simpleperf_etm_trace_provider::SimpleperfEtmTraceProvider;

pub trait TraceProvider {
    fn get_name(&self) -> &'static str;
    fn trace(&self, trace_dir: &Path, tag: &str, sampling_period: &Duration);
    fn process(&self, trace_dir: &Path, profile_dir: &Path) -> Result<()>;
}

pub fn get_trace_provider() -> Result<Arc<Mutex<dyn TraceProvider + Send>>> {
    if SimpleperfEtmTraceProvider::supported() {
        log::info!("simpleperf_etm trace provider registered.");
        return Ok(Arc::new(Mutex::new(SimpleperfEtmTraceProvider {})));
    }

    Err(anyhow!("No trace provider found for this device."))
}

pub fn construct_file_name(tag: &str) -> String {
    format!("{}_{}", Utc::now().format("%Y%m%d-%H%M%S"), tag)
}
