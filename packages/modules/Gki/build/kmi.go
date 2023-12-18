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

// Minimum support of KMI version in Go. Keep in sync with libkver.

package gki

import (
	"fmt"
	"regexp"
)

var digits = "([0-9]+)"
var reKmi = regexp.MustCompile("^([0-9]+)[.]([0-9]+)-(android[0-9]+)-([0-9]+|unstable)$")

// Input is a valid KMI version, e.g. 5.4-android12-0.
// Return a sanitized string to be used as a suffix of APEX package name
// com.android.gki.kmi_5_4_android12_0
// Keep in sync with libkver.
func kmiVersionToApexName(s string) (string, error) {
	matches := reKmi.FindAllStringSubmatch(s, 4)

	if matches == nil {
		return "", fmt.Errorf("Poorly formed KMI version: %q must match regex %q", s, reKmi)
	}

	version := matches[0][1]
	patchLevel := matches[0][2]
	androidRelease := matches[0][3]
	kmiGeneration := matches[0][4]

	return fmt.Sprintf("com.android.gki.kmi_%s_%s_%s_%s",
		version, patchLevel, androidRelease, kmiGeneration), nil
}
