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
	"android/soong/cc"
	"android/soong/java"
	"android/soong/rust"

	"path/filepath"
	"strings"

	"github.com/google/blueprint/proptools"
)

func addLibrary(mctx android.LoadHookContext, i *aidlInterface, version string, lang string) string {
	if lang == langJava {
		return addJavaLibrary(mctx, i, version)
	} else if lang == langRust {
		return addRustLibrary(mctx, i, version)
	}
	return addCppLibrary(mctx, i, version, lang)
}

func addCppLibrary(mctx android.LoadHookContext, i *aidlInterface, version string, lang string) string {
	cppSourceGen := i.versionedName(version) + "-" + lang + "-source"
	cppModuleGen := i.versionedName(version) + "-" + lang

	srcs, aidlRoot := i.srcsForVersion(mctx, version)
	if len(srcs) == 0 {
		// This can happen when the version is about to be frozen; the version
		// directory is created but API dump hasn't been copied there.
		// Don't create a library for the yet-to-be-frozen version.
		return ""
	}

	var overrideVndkProperties cc.VndkProperties

	if !i.isModuleForVndk(version) {
		// We only want the VNDK to include the latest interface. For interfaces in
		// development, they will be frozen, so we put their latest version in the
		// VNDK. For interfaces which are already frozen, we put their latest version
		// in the VNDK, and when that version is frozen, the version in the VNDK can
		// be updated. Otherwise, we remove this library from the VNDK, to avoid adding
		// multiple versions of the same library to the VNDK.
		overrideVndkProperties.Vndk.Enabled = proptools.BoolPtr(false)
		overrideVndkProperties.Vndk.Support_system_process = proptools.BoolPtr(false)
	}

	var commonProperties *CommonNativeBackendProperties
	if lang == langCpp {
		commonProperties = &i.properties.Backend.Cpp.CommonNativeBackendProperties
	} else if lang == langNdk || lang == langNdkPlatform {
		commonProperties = &i.properties.Backend.Ndk.CommonNativeBackendProperties
	}

	genLog := proptools.Bool(commonProperties.Gen_log)
	genTrace := proptools.Bool(i.properties.Gen_trace)

	mctx.CreateModule(aidlGenFactory, &nameProperties{
		Name: proptools.StringPtr(cppSourceGen),
	}, &aidlGenProperties{
		Srcs:                  srcs,
		AidlRoot:              aidlRoot,
		IsToT:                 version == i.nextVersion(),
		ImportsWithoutVersion: i.properties.ImportsWithoutVersion,
		Stability:             i.properties.Stability,
		Lang:                  lang,
		BaseName:              i.ModuleBase.Name(),
		GenLog:                genLog,
		Version:               i.versionForAidlGenRule(version),
		GenTrace:              genTrace,
		Unstable:              i.properties.Unstable,
		Visibility:            srcsVisibility(mctx, lang),
		Flags:                 i.flagsForAidlGenRule(version),
	})

	importExportDependencies := []string{}
	var sharedLibDependency []string
	var headerLibs []string
	var sdkVersion *string
	var minSdkVersion *string
	var stl *string
	var cpp_std *string
	var hostSupported *bool
	var addCflags []string

	if lang == langCpp {
		importExportDependencies = append(importExportDependencies, "libbinder", "libutils")
		if genTrace {
			sharedLibDependency = append(sharedLibDependency, "libcutils")
		}
		hostSupported = i.properties.Host_supported
		minSdkVersion = i.properties.Backend.Cpp.Min_sdk_version
	} else if lang == langNdk {
		importExportDependencies = append(importExportDependencies, "libbinder_ndk")
		if genTrace {
			sharedLibDependency = append(sharedLibDependency, "libandroid")
		}
		sdkVersion = proptools.StringPtr("current")
		stl = proptools.StringPtr("c++_shared")
		minSdkVersion = i.properties.Backend.Ndk.Min_sdk_version
	} else if lang == langNdkPlatform {
		importExportDependencies = append(importExportDependencies, "libbinder_ndk")
		if genTrace {
			headerLibs = append(headerLibs, "libandroid_aidltrace")
			sharedLibDependency = append(sharedLibDependency, "libcutils")
		}
		hostSupported = i.properties.Host_supported
		addCflags = append(addCflags, "-DBINDER_STABILITY_SUPPORT")
		minSdkVersion = i.properties.Backend.Ndk.Min_sdk_version
	} else {
		panic("Unrecognized language: " + lang)
	}

	vendorAvailable := i.properties.Vendor_available
	odmAvailable := i.properties.Odm_available
	productAvailable := i.properties.Product_available
	if lang == langCpp {
		// Vendor and product modules cannot use the libbinder (cpp) backend of AIDL in a
		// way that is stable. So, in order to prevent accidental usage of these library by
		// vendor and product forcibly disabling this version of the library.
		//
		// It may be the case in the future that we will want to enable this (if some generic
		// helper should be used by both libbinder vendor things using /dev/vndbinder as well
		// as those things using /dev/binder + libbinder_ndk to talk to stable interfaces).
		if "vintf" == proptools.String(i.properties.Stability) {
			overrideVndkProperties.Vndk.Private = proptools.BoolPtr(true)
		}
		// As libbinder is not available for the product processes, we must not create
		// product variant for the aidl_interface
		productAvailable = nil
	}

	if lang == langNdk {
		// TODO(b/121157555): when the NDK variant is its own variant, these wouldn't interact,
		// but we can't create a vendor or product version of an NDK variant
		//
		// nil (unspecified) is used instead of false so that this can't conflict with
		// 'vendor: true', for instance.
		vendorAvailable = nil
		odmAvailable = nil
		productAvailable = nil
		overrideVndkProperties.Vndk.Enabled = proptools.BoolPtr(false)
		overrideVndkProperties.Vndk.Support_system_process = proptools.BoolPtr(false)
	}

	mctx.CreateModule(aidlImplementationGeneratorFactory, &nameProperties{
		Name: proptools.StringPtr(cppModuleGen + "-generator"),
	}, &aidlImplementationGeneratorProperties{
		Lang:              lang,
		AidlInterfaceName: i.ModuleBase.Name(),
		Version:           version,
		ModuleProperties: []interface{}{
			&ccProperties{
				Name:                      proptools.StringPtr(cppModuleGen),
				Vendor_available:          vendorAvailable,
				Odm_available:             odmAvailable,
				Product_available:         productAvailable,
				Host_supported:            hostSupported,
				Defaults:                  []string{"aidl-cpp-module-defaults"},
				Double_loadable:           i.properties.Double_loadable,
				Generated_sources:         []string{cppSourceGen},
				Generated_headers:         []string{cppSourceGen},
				Export_generated_headers:  []string{cppSourceGen},
				Shared_libs:               append(importExportDependencies, sharedLibDependency...),
				Header_libs:               headerLibs,
				Export_shared_lib_headers: importExportDependencies,
				Sdk_version:               sdkVersion,
				Stl:                       stl,
				Cpp_std:                   cpp_std,
				Cflags:                    append(addCflags, "-Wextra", "-Wall", "-Werror", "-Wextra-semi"),
				Apex_available:            commonProperties.Apex_available,
				Min_sdk_version:           minSdkVersion,
				UseApexNameMacro:          true,
				Target: ccTargetProperties{
					// Currently necessary for host builds
					// TODO(b/31559095): bionic on host should define this
					// TODO(b/146436251): default isn't applied because the module is created
					// in PreArchMutators, when import behavior becomes explicit, the logic can
					// be moved back to LoadHook
					Host: hostProperties{Cflags: []string{
						"-D__INTRODUCED_IN(n)=",
						"-D__assert(a,b,c)=",
						// We want all the APIs to be available on the host.
						"-D__ANDROID_API__=10000"}},
					Darwin: perTargetProperties{Enabled: proptools.BoolPtr(false)}},
				Tidy: proptools.BoolPtr(true),
				// Do the tidy check only for the generated headers
				Tidy_flags:            []string{"--header-filter=" + android.PathForOutput(mctx).String() + ".*"},
				Tidy_checks_as_errors: []string{"*"},
			}, &i.properties.VndkProperties,
			&commonProperties.VndkProperties,
			&overrideVndkProperties,
		},
	})

	return cppModuleGen
}

func addJavaLibrary(mctx android.LoadHookContext, i *aidlInterface, version string) string {
	javaSourceGen := i.versionedName(version) + "-java-source"
	javaModuleGen := i.versionedName(version) + "-java"
	srcs, aidlRoot := i.srcsForVersion(mctx, version)
	if len(srcs) == 0 {
		// This can happen when the version is about to be frozen; the version
		// directory is created but API dump hasn't been copied there.
		// Don't create a library for the yet-to-be-frozen version.
		return ""
	}

	sdkVersion := i.properties.Backend.Java.Sdk_version
	if !proptools.Bool(i.properties.Backend.Java.Platform_apis) && sdkVersion == nil {
		// platform apis requires no default
		sdkVersion = proptools.StringPtr("system_current")
	}

	mctx.CreateModule(aidlGenFactory, &nameProperties{
		Name: proptools.StringPtr(javaSourceGen),
	}, &aidlGenProperties{
		Srcs:                  srcs,
		AidlRoot:              aidlRoot,
		IsToT:                 version == i.nextVersion(),
		ImportsWithoutVersion: i.properties.ImportsWithoutVersion,
		Stability:             i.properties.Stability,
		Lang:                  langJava,
		BaseName:              i.ModuleBase.Name(),
		Version:               i.versionForAidlGenRule(version),
		GenTrace:              proptools.Bool(i.properties.Gen_trace),
		Unstable:              i.properties.Unstable,
		Visibility:            srcsVisibility(mctx, langJava),
		Flags:                 i.flagsForAidlGenRule(version),
	})

	mctx.CreateModule(aidlImplementationGeneratorFactory, &nameProperties{
		Name: proptools.StringPtr(javaModuleGen + "-generator"),
	}, &aidlImplementationGeneratorProperties{
		Lang:              langJava,
		AidlInterfaceName: i.ModuleBase.Name(),
		Version:           version,
		ModuleProperties: []interface{}{&javaProperties{
			Name:            proptools.StringPtr(javaModuleGen),
			Installable:     proptools.BoolPtr(true),
			Defaults:        []string{"aidl-java-module-defaults"},
			Sdk_version:     sdkVersion,
			Platform_apis:   i.properties.Backend.Java.Platform_apis,
			Srcs:            []string{":" + javaSourceGen},
			Apex_available:  i.properties.Backend.Java.Apex_available,
			Min_sdk_version: i.properties.Backend.Java.Min_sdk_version,
		}},
	})

	return javaModuleGen
}

func addRustLibrary(mctx android.LoadHookContext, i *aidlInterface, version string) string {
	rustSourceGen := i.versionedName(version) + "-rust-source"
	rustModuleGen := i.versionedName(version) + "-rust"
	srcs, aidlRoot := i.srcsForVersion(mctx, version)
	if len(srcs) == 0 {
		// This can happen when the version is about to be frozen; the version
		// directory is created but API dump hasn't been copied there.
		// Don't create a library for the yet-to-be-frozen version.
		return ""
	}

	mctx.CreateModule(aidlGenFactory, &nameProperties{
		Name: proptools.StringPtr(rustSourceGen),
	}, &aidlGenProperties{
		Srcs:                  srcs,
		AidlRoot:              aidlRoot,
		ImportsWithoutVersion: i.properties.ImportsWithoutVersion,
		IsToT:                 version == i.nextVersion(),
		Stability:             i.properties.Stability,
		Lang:                  langRust,
		BaseName:              i.ModuleBase.Name(),
		Version:               i.versionForAidlGenRule(version),
		Unstable:              i.properties.Unstable,
		Visibility:            srcsVisibility(mctx, langRust),
		Flags:                 i.flagsForAidlGenRule(version),
	})

	versionedRustName := fixRustName(i.versionedName(version))
	rustCrateName := fixRustName(i.ModuleBase.Name())

	mctx.CreateModule(aidlRustLibraryFactory, &rustProperties{
		Name:           proptools.StringPtr(rustModuleGen),
		Crate_name:     rustCrateName,
		Stem:           proptools.StringPtr("lib" + versionedRustName),
		Defaults:       []string{"aidl-rust-module-defaults"},
		Host_supported: i.properties.Host_supported,
		Apex_available: i.properties.Backend.Rust.Apex_available,
		Target:         rustTargetProperties{Darwin: perTargetProperties{Enabled: proptools.BoolPtr(false)}},
	}, &rust.SourceProviderProperties{
		Source_stem: proptools.StringPtr(versionedRustName),
	}, &aidlRustSourceProviderProperties{
		SourceGen:         rustSourceGen,
		Imports:           i.properties.Imports,
		Version:           version,
		AidlInterfaceName: i.ModuleBase.Name(),
	})

	return rustModuleGen
}

// This function returns module name with version. Assume that there is foo of which latest version is 2
// Version -> Module name
// "1"->foo-V1
// "2"->foo-V2
// "3"->foo-V3
// And assume that there is 'bar' which is an 'unstable' interface.
// ""->bar
func (i *aidlInterface) versionedName(version string) string {
	name := i.ModuleBase.Name()
	if version == "" {
		return name
	}
	return name + "-V" + version
}

func (i *aidlInterface) srcsForVersion(mctx android.LoadHookContext, version string) (srcs []string, aidlRoot string) {
	if version == i.nextVersion() {
		return i.properties.Srcs, i.properties.Local_include_dir
	} else {
		aidlRoot = filepath.Join(aidlApiDir, i.ModuleBase.Name(), version)
		full_paths, err := mctx.GlobWithDeps(filepath.Join(mctx.ModuleDir(), aidlRoot, "**/*.aidl"), nil)
		if err != nil {
			panic(err)
		}
		for _, path := range full_paths {
			// Here, we need path local to the module
			srcs = append(srcs, strings.TrimPrefix(path, mctx.ModuleDir()+"/"))
		}
		return srcs, aidlRoot
	}
}

func (i *aidlInterface) versionForAidlGenRule(version string) string {
	if !i.hasVersion() {
		return ""
	}
	return version
}

func (i *aidlInterface) flagsForAidlGenRule(version string) (flags []string) {
	flags = append(flags, i.properties.Flags...)
	// For ToT, turn on "-Weverything" (enable all warnings)
	if version == i.nextVersion() {
		flags = append(flags, "-Weverything")
	}
	return
}

func (i *aidlInterface) isModuleForVndk(version string) bool {
	if i.properties.Vndk_use_version != nil {
		if !i.hasVersion() {
			panic("does not make sense, vndk_use_version specififed")
		}
		// Will be exactly one of the version numbers
		return version == *i.properties.Vndk_use_version
	}

	// For an interface with no versions, this is the ToT interface.
	if !i.hasVersion() {
		return version == i.nextVersion()
	}

	return version == i.latestVersion()
}

// importing aidl_interface's version  | imported aidl_interface | imported aidl_interface's version
// --------------------------------------------------------------------------------------------------
// whatever                            | unstable                | unstable version
// ToT version(including unstable)     | whatever                | ToT version(unstable if unstable)
// otherwise                           | whatever                | the latest stable version
// In the case that import specifies the version which it wants to use, use that version.
func (i *aidlInterface) getImportWithVersion(version string, anImport string, config android.Config) string {
	if hasVersionSuffix(anImport) {
		return anImport
	}
	other := lookupInterface(anImport, config)
	if proptools.Bool(other.properties.Unstable) {
		return anImport
	}
	if version == i.nextVersion() || !other.hasVersion() {
		return other.versionedName(other.nextVersion())
	}
	return other.versionedName(other.latestVersion())
}

func aidlImplementationGeneratorFactory() android.Module {
	g := &aidlImplementationGenerator{}
	g.AddProperties(&g.properties)
	android.InitAndroidModule(g)
	return g
}

type aidlImplementationGenerator struct {
	android.ModuleBase
	properties aidlImplementationGeneratorProperties
}

type aidlImplementationGeneratorProperties struct {
	Lang              string
	AidlInterfaceName string
	Version           string
	ModuleProperties  []interface{}
}

func (g *aidlImplementationGenerator) DepsMutator(ctx android.BottomUpMutatorContext) {
}

func (g *aidlImplementationGenerator) GenerateAndroidBuildActions(ctx android.ModuleContext) {
}

func (g *aidlImplementationGenerator) GenerateImplementation(ctx android.TopDownMutatorContext) {
	i := lookupInterface(g.properties.AidlInterfaceName, ctx.Config())
	version := g.properties.Version
	lang := g.properties.Lang
	if g.properties.Lang == langJava {
		imports := make([]string, len(i.properties.Imports))
		for idx, anImport := range i.properties.Imports {
			imports[idx] = i.getImportWithVersion(version, anImport, ctx.Config()) + "-" + langJava
		}
		if p, ok := g.properties.ModuleProperties[0].(*javaProperties); ok {
			p.Static_libs = imports
		}
		ctx.CreateModule(java.LibraryFactory, g.properties.ModuleProperties...)
	} else {
		imports := make([]string, len(i.properties.Imports))
		for idx, anImport := range i.properties.Imports {
			imports[idx] = i.getImportWithVersion(version, anImport, ctx.Config()) + "-" + lang
		}
		if p, ok := g.properties.ModuleProperties[0].(*ccProperties); ok {
			p.Shared_libs = append(p.Shared_libs, imports...)
			p.Export_shared_lib_headers = append(p.Export_shared_lib_headers, imports...)
		}
		ctx.CreateModule(cc.LibraryFactory, g.properties.ModuleProperties...)
	}
}
