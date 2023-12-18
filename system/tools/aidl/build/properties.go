// Copyright (C) 2017 The Android Open Source Project
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

package aidl

type nameProperties struct {
	Name *string
}

type hostProperties struct {
	Cflags []string
}
type perTargetProperties struct {
	Enabled *bool
}

type ccTargetProperties struct {
	Host   hostProperties
	Darwin perTargetProperties
}

type rustTargetProperties struct {
	Darwin perTargetProperties
}

type ccProperties struct {
	Name                      *string
	Owner                     *string
	Defaults                  []string
	Double_loadable           *bool
	Vendor_available          *bool
	Odm_available             *bool
	Product_available         *bool
	Host_supported            *bool
	Generated_sources         []string
	Generated_headers         []string
	Shared_libs               []string
	Export_shared_lib_headers []string
	Export_generated_headers  []string
	Header_libs               []string
	Sdk_version               *string
	Stl                       *string
	Cpp_std                   *string
	Cflags                    []string
	Stem                      *string
	Apex_available            []string
	Min_sdk_version           *string
	UseApexNameMacro          bool
	Target                    ccTargetProperties
	Tidy                      *bool
	Tidy_flags                []string
	Tidy_checks_as_errors     []string
}

type javaProperties struct {
	Name            *string
	Owner           *string
	Defaults        []string
	Installable     *bool
	Sdk_version     *string
	Platform_apis   *bool
	Srcs            []string
	Static_libs     []string
	Apex_available  []string
	Min_sdk_version *string
}

type rustProperties struct {
	Name           *string
	Crate_name     string
	Owner          *string
	Defaults       []string
	Host_supported *bool
	Srcs           []string
	Rustlibs       []string
	Stem           *string
	Target         rustTargetProperties
	Apex_available []string
}

type phonyProperties struct {
	Name     *string
	Required []string
}
