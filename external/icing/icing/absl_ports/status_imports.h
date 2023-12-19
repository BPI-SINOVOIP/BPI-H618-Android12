// Copyright (C) 2019 Google LLC
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

#ifndef ICING_ABSL_PORTS_STATUS_IMPORTS_H_
#define ICING_ABSL_PORTS_STATUS_IMPORTS_H_

#include "icing/text_classifier/lib3/utils/base/status.h"

namespace icing {
namespace lib {
namespace absl_ports {

// TODO(b/144458732) Delete this file once visibility on TC3 Status has been
// granted to the sample app.
using Status = libtextclassifier3::Status;

}  // namespace absl_ports
}  // namespace lib
}  // namespace icing

#endif  // ICING_ABSL_PORTS_STATUS_IMPORTS_H_
