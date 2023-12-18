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
	"testing"
)

func expectValid(t *testing.T, kmi string, expectedApexName string) {
	t.Helper()
	got, e := kmiVersionToApexName(kmi)
	if e != nil {
		t.Errorf("Expected no error when parsing %q, got %q", kmi, e)
	}
	if got != expectedApexName {
		t.Errorf("Expected kmiVersionToApexName(%q) == %q, got %q", kmi, expectedApexName, got)
	}
}

func expectInvalid(t *testing.T, kmi string) {
	t.Helper()
	got, e := kmiVersionToApexName(kmi)
	if e == nil {
		t.Errorf("Expected error when parsing %q, got no error with result %q", kmi, got)
	}
}

func TestParse(t *testing.T) {
	expectInvalid(t, "")
	expectInvalid(t, "foobar")
	expectInvalid(t, "1")
	expectValid(t, "5.4-android12-0", "com.android.gki.kmi_5_4_android12_0")
	expectValid(t, "5.4-android12-42", "com.android.gki.kmi_5_4_android12_42")
}
