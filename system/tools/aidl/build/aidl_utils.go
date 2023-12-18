// Copyright (C) 2021 The Android Open Source Project
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

import (
	"android/soong/android"
	"strings"
)

// wrap(p, a, s) = [p + v + s for v in a]
func wrap(prefix string, strs []string, suffix string) []string {
	ret := make([]string, len(strs))
	for i, v := range strs {
		ret[i] = prefix + v + suffix
	}
	return ret
}

// wrapFunc(p, a, s, f) = [p + f(v) + s for v in a]
func wrapFunc(prefix string, strs []string, suffix string, f func(string) string) []string {
	ret := make([]string, len(strs))
	for i, v := range strs {
		ret[i] = prefix + f(v) + suffix
	}
	return ret
}

// concat(a...) = sum((i for i in a), [])
func concat(sstrs ...[]string) []string {
	var ret []string
	for _, v := range sstrs {
		ret = append(ret, v...)
	}
	return ret
}

// baseDir is the directory where the package name starts. e.g. For an AIDL fil
// mymodule/aidl_src/com/android/IFoo.aidl, baseDir is mymodule/aidl_src given that the package name is
// com.android. The build system however don't know the package name without actually reading the AIDL file.
// Therefore, we rely on the user to correctly set the base directory via following two methods:
// 1) via the 'path' property of filegroup or
// 2) via `local_include_dir' of the aidl_interface module.
func getBaseDir(ctx android.ModuleContext, src android.Path, aidlRoot android.Path) string {
	// By default, we try to get 1) by reading Rel() of the input path.
	baseDir := strings.TrimSuffix(src.String(), src.Rel())
	// However, if 2) is set and it's more specific (i.e. deeper) than 1), we use 2).
	if strings.HasPrefix(aidlRoot.String(), baseDir) {
		baseDir = aidlRoot.String()
	}
	return baseDir
}

func fixRustName(name string) string {
	return strings.Map(func(r rune) rune {
		switch r {
		case '-', '.':
			return '_'
		default:
			return r
		}
	}, name)
}
