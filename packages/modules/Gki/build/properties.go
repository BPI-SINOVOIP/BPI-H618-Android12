// Copyright (C) 2020 The Android Open Source Project
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

package gki

type moduleCommonProperties struct {
	Name             *string
	Enabled          *bool
	Required         []string
	Product_specific *bool
}

type makefileGoalProperties struct {
	Product_out_path *string
}

type genRuleProperties struct {
	Cmd      *string
	Defaults []string
	Out      []string
	Srcs     []string
	Tools    []string
}

type prebuiltEtcProperties struct {
	Src                   *string
	Filename_from_src     *bool
	Relative_install_path *string
	Installable           *bool
}

type apexProperties struct {
	Apex_name   *string
	Manifest    *string
	Defaults    []string
	Installable *bool
	Prebuilts   []string
	Overrides   []string
}
