// Copyright (C) 2018 The Android Open Source Project
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
	"android/soong/phony"
	"android/soong/rust"

	"fmt"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"sync"

	"github.com/google/blueprint/proptools"
)

const (
	aidlInterfaceSuffix       = "_interface"
	aidlMetadataSingletonName = "aidl_metadata_json"
	aidlApiDir                = "aidl_api"
	aidlApiSuffix             = "-api"
	langCpp                   = "cpp"
	langJava                  = "java"
	langNdk                   = "ndk"
	langNdkPlatform           = "ndk_platform"
	langRust                  = "rust"

	currentVersion = "current"
)

var (
	pctx = android.NewPackageContext("android/aidl")
)

func init() {
	pctx.Import("android/soong/android")
	pctx.HostBinToolVariable("aidlCmd", "aidl")
	pctx.SourcePathVariable("aidlToJniCmd", "system/tools/aidl/build/aidl_to_jni.py")
	pctx.SourcePathVariable("aidlRustGlueCmd", "system/tools/aidl/build/aidl_rust_glue.py")
	android.RegisterModuleType("aidl_interface", aidlInterfaceFactory)
	android.PreArchMutators(registerPreDepsMutators)
	android.PreArchBp2BuildMutators(registerPreDepsMutators)
	android.PostDepsMutators(func(ctx android.RegisterMutatorsContext) {
		ctx.BottomUp("checkUnstableModule", checkUnstableModuleMutator).Parallel()
		ctx.BottomUp("recordVersions", recordVersions).Parallel()
		ctx.BottomUp("checkDuplicatedVersions", checkDuplicatedVersions).Parallel()
	})
}

func registerPreDepsMutators(ctx android.RegisterMutatorsContext) {
	ctx.BottomUp("checkImports", checkImports).Parallel()
	ctx.TopDown("createAidlInterface", createAidlInterfaceMutator).Parallel()
}

func createAidlInterfaceMutator(mctx android.TopDownMutatorContext) {
	if g, ok := mctx.Module().(*aidlImplementationGenerator); ok {
		g.GenerateImplementation(mctx)
	}
}

func checkUnstableModuleMutator(mctx android.BottomUpMutatorContext) {
	// If it is an aidl interface, we don't need to check its dependencies.
	if isAidlModule(mctx.ModuleName(), mctx.Config()) {
		return
	}
	mctx.VisitDirectDepsIf(func(m android.Module) bool {
		return android.InList(m.Name(), *unstableModules(mctx.Config()))
	}, func(m android.Module) {
		if mctx.ModuleName() == m.Name() {
			return
		}
		// TODO(b/154066686): Replace it with a common method instead of listing up module types.
		// Test libraries are exempted.
		if android.InList(mctx.ModuleType(), []string{"cc_test_library", "android_test", "cc_benchmark", "cc_test"}) {
			return
		}

		mctx.ModuleErrorf(m.Name() + " is disallowed in release version because it is unstable, and its \"owner\" property is missing.")
	})
}

func isAidlModule(moduleName string, config android.Config) bool {
	for _, i := range *aidlInterfaces(config) {
		if android.InList(moduleName, i.internalModuleNames) {
			return true
		}
	}
	return false
}

func recordVersions(mctx android.BottomUpMutatorContext) {
	switch mctx.Module().(type) {
	case *java.Library:
	case *cc.Module:
	case *rust.Module:
	case *aidlGenRule:
	default:
		return
	}

	isAidlModule := isAidlModule(mctx.ModuleName(), mctx.Config())

	// First, gather all the AIDL interfaces modules that are directly or indirectly
	// depended on by this module
	myAidlDeps := make(map[DepInfo]bool)
	mctx.VisitDirectDeps(func(dep android.Module) {
		switch dep.(type) {
		case *java.Library:
		case *cc.Module:
		case *rust.Module:
		case *aidlGenRule:
			// Dependencies to the source module is tracked only when it's from a client
			// module, i.e. not from an AIDL-generated stub library
			if isAidlModule {
				return
			}
		default:
			return
		}
		depName := mctx.OtherModuleName(dep)
		isSource := strings.HasSuffix(depName, "-source")
		depName = strings.TrimSuffix(depName, "-source")
		// If this module depends on one of the aidl interface module, record it
		for _, i := range *aidlInterfaces(mctx.Config()) {
			if android.InList(depName, i.internalModuleNames) {
				ifaceName := i.ModuleBase.Name()
				verLang := depName[len(ifaceName):]
				myAidlDeps[DepInfo{ifaceName, verLang, isSource}] = true
				break
			}
		}
		// If dep is in aidlDeps, that means dep has direct or indirect dependencies to AIDL interfaces
		// That becomes this module's aidlDeps as well
		aidlDepsMutex.RLock()
		if depsOfDep, ok := aidlDeps(mctx.Config())[dep]; ok {
			for _, d := range depsOfDep {
				myAidlDeps[d] = true
			}
		}
		aidlDepsMutex.RUnlock()
	})

	if len(myAidlDeps) == 0 {
		// This should be usual case.
		return
	}

	// Then, record the aidl deps of this module to the global map so that it can be used by
	// next runs of this mutator for the modules that depend on this module.
	var list []DepInfo
	for d := range myAidlDeps {
		list = append(list, d)
	}
	aidlDepsMutex.Lock()
	aidlDeps(mctx.Config())[mctx.Module()] = list
	aidlDepsMutex.Unlock()
}

func checkDuplicatedVersions(mctx android.BottomUpMutatorContext) {
	switch mctx.Module().(type) {
	case *java.Library:
	case *cc.Module:
	case *rust.Module:
	case *aidlGenRule:
	default:
		return
	}

	aidlDepsMutex.RLock()
	myAidlDeps := aidlDeps(mctx.Config())[mctx.Module()]
	aidlDepsMutex.RUnlock()
	if myAidlDeps == nil || len(myAidlDeps) == 0 {
		return // This should be the usual case
	}

	// Lastly, report an error if there is any duplicated versions of the same interface * lang
	for _, lang := range []string{langJava, langCpp, langNdk, langNdkPlatform} {
		// interfaceName -> verLang -> list of module names
		versionsOf := make(map[string]map[string][]string)
		for _, dep := range myAidlDeps {
			if !strings.HasSuffix(dep.verLang, lang) {
				continue
			}
			versions := versionsOf[dep.ifaceName]
			if versions == nil {
				versions = make(map[string][]string)
				versionsOf[dep.ifaceName] = versions
			}
			versions[dep.verLang] = append(versions[dep.verLang], dep.moduleName())
		}

		for _, versions := range versionsOf {
			if len(versions) >= 2 {
				var violators []string
				for _, modules := range versions {
					violators = append(violators, modules...)
				}
				violators = android.SortedUniqueStrings(violators)
				mctx.ModuleErrorf("depends on multiple versions of the same aidl_interface: %s", strings.Join(violators, ", "))
				mctx.WalkDeps(func(child android.Module, parent android.Module) bool {
					if android.InList(child.Name(), violators) {
						mctx.ModuleErrorf("Dependency path: %s", mctx.GetPathString(true))
						return false
					}
					return true
				})
			}
		}
	}
}

func getPaths(ctx android.ModuleContext, rawSrcs []string, root string) (srcs android.Paths, imports []string) {
	// TODO(b/189288369): move this to android.PathsForModuleSrcSubDir(ctx, srcs, subdir)
	for _, src := range rawSrcs {
		if m, _ := android.SrcIsModuleWithTag(src); m != "" {
			srcs = append(srcs, android.PathsForModuleSrc(ctx, []string{src})...)
		} else {
			srcs = append(srcs, android.PathsWithModuleSrcSubDir(ctx, android.PathsForModuleSrc(ctx, []string{src}), root)...)
		}
	}

	if len(srcs) == 0 {
		ctx.PropertyErrorf("srcs", "No sources provided.")
	}

	// gather base directories from input .aidl files
	for _, src := range srcs {
		if src.Ext() != ".aidl" {
			// Silently ignore non-aidl files as some filegroups have both java and aidl files together
			continue
		}
		baseDir := strings.TrimSuffix(src.String(), src.Rel())
		baseDir = strings.TrimSuffix(baseDir, "/")
		if baseDir != "" && !android.InList(baseDir, imports) {
			imports = append(imports, baseDir)
		}
	}

	return srcs, imports
}

func isRelativePath(path string) bool {
	if path == "" {
		return true
	}
	return filepath.Clean(path) == path && path != ".." &&
		!strings.HasPrefix(path, "../") && !strings.HasPrefix(path, "/")
}

type CommonBackendProperties struct {
	// Whether to generate code in the corresponding backend.
	// Default: true
	Enabled        *bool
	Apex_available []string

	// The minimum version of the sdk that the compiled artifacts will run against
	// For native modules, the property needs to be set when a module is a part of mainline modules(APEX).
	// Forwarded to generated java/native module.
	Min_sdk_version *string

	// Determines whether the generated source files are available or not. When set to true,
	// the source files can be added to `srcs` property via `:<ifacename>-<backend>-source`,
	// e.g., ":myaidl-java-source"
	Srcs_available *bool
}

type CommonNativeBackendProperties struct {
	CommonBackendProperties
	// Whether to generate additional code for gathering information
	// about the transactions.
	// Default: false
	Gen_log *bool

	// VNDK properties for correspdoning backend.
	cc.VndkProperties
}

type DumpApiProperties struct {
	// Dumps without license header (assuming it is the first comment in .aidl file). Default: false
	No_license *bool
}

type aidlInterfaceProperties struct {
	// Vndk properties for C++/NDK libraries only (preferred to use backend-specific settings)
	cc.VndkProperties

	// How to interpret VNDK options. We only want one library in the VNDK (not multiple
	// versions, since this would be a waste of space/unclear, and ultimately we want all
	// code in a given release to be updated to use a specific version). By default, this
	// puts either the latest stable version of the library or, if there is no stable
	// version, the unstable version of the library in the VNDK. When using this field,
	// explicitly set it to one of the values in the 'versions' field to put that version
	// in the VNDK or set it to the next version (1 higher than this) to mean the version
	// that will be frozen in the next update.
	Vndk_use_version *string

	// Whether the library can be installed on the vendor image.
	Vendor_available *bool

	// Whether the library can be installed on the odm image.
	Odm_available *bool

	// Whether the library can be installed on the product image.
	Product_available *bool

	// Whether the library can be loaded multiple times into the same process
	Double_loadable *bool

	// Whether the library can be used on host
	Host_supported *bool

	// Whether tracing should be added to the interface.
	Gen_trace *bool

	// Top level directories for includes.
	// TODO(b/128940869): remove it if aidl_interface can depend on framework.aidl
	Include_dirs []string
	// Relative path for includes. By default assumes AIDL path is relative to current directory.
	Local_include_dir string

	// List of .aidl files which compose this interface.
	Srcs []string `android:"path"`

	// List of aidl_interface modules that this uses. If one of your AIDL interfaces uses an
	// interface or parcelable from another aidl_interface, you should put its name here.
	// It could be an aidl_interface solely or with version(such as -V1)
	Imports []string

	// List of aidl_interface modules that this uses. It trims version suffix in 'Imports' field.
	ImportsWithoutVersion []string `blueprint:"mutated"`

	// Used by gen dependency to fill out aidl include path
	Full_import_paths []string `blueprint:"mutated"`

	// Stability promise. Currently only supports "vintf".
	// If this is unset, this corresponds to an interface with stability within
	// this compilation context (so an interface loaded here can only be used
	// with things compiled together, e.g. on the system.img).
	// If this is set to "vintf", this corresponds to a stability promise: the
	// interface must be kept stable as long as it is used.
	Stability *string

	// Previous API versions that are now frozen. The version that is last in
	// the list is considered as the most recent version.
	Versions []string

	Backend struct {
		// Backend of the compiler generating code for Java clients.
		// When enabled, this creates a target called "<name>-java".
		Java struct {
			CommonBackendProperties
			// Set to the version of the sdk to compile against
			// Default: system_current
			Sdk_version *string
			// Whether to compile against platform APIs instead of
			// an SDK.
			Platform_apis *bool
		}
		// Backend of the compiler generating code for C++ clients using
		// libbinder (unstable C++ interface)
		// When enabled, this creates a target called "<name>-cpp".
		Cpp struct {
			CommonNativeBackendProperties
		}
		// Backend of the compiler generating code for C++ clients using
		// libbinder_ndk (stable C interface to system's libbinder)
		// When enabled, this creates a target called "<name>-ndk"
		// (for apps) and "<name>-ndk_platform" (for platform usage).
		Ndk struct {
			CommonNativeBackendProperties

			// Currently, all ndk-supported interfaces generate two variants:
			// - ndk - for apps to use, against an NDK
			// - ndk_platform - for the platform to use
			//
			// This adds an option to disable the 'ndk' variant in cases where APIs
			// only available in the platform version work.
			Apps_enabled *bool
		}
		// Backend of the compiler generating code for Rust clients.
		// When enabled, this creates a target called "<name>-rust".
		Rust struct {
			CommonBackendProperties
		}
	}

	// Marks that this interface does not need to be stable. When set to true, the build system
	// doesn't create the API dump and require it to be updated. Default is false.
	Unstable *bool

	// Optional flags to be passed to the AIDL compiler. e.g. "-Weverything"
	Flags []string

	// --dumpapi options
	Dumpapi DumpApiProperties
}

type aidlInterface struct {
	android.ModuleBase

	properties aidlInterfaceProperties

	computedTypes []string

	// list of module names that are created for this interface
	internalModuleNames []string
}

func (i *aidlInterface) shouldGenerateJavaBackend() bool {
	// explicitly true if not specified to give early warning to devs
	return i.properties.Backend.Java.Enabled == nil || *i.properties.Backend.Java.Enabled
}

func (i *aidlInterface) shouldGenerateCppBackend() bool {
	// explicitly true if not specified to give early warning to devs
	return i.properties.Backend.Cpp.Enabled == nil || *i.properties.Backend.Cpp.Enabled
}

func (i *aidlInterface) shouldGenerateNdkBackend() bool {
	// explicitly true if not specified to give early warning to devs
	return i.properties.Backend.Ndk.Enabled == nil || *i.properties.Backend.Ndk.Enabled
}

func (i *aidlInterface) shouldGenerateAppNdkBackend() bool {
	if !i.shouldGenerateNdkBackend() {
		return false
	}
	// explicitly true if not specified to give early warning to devs
	return i.properties.Backend.Ndk.Apps_enabled == nil || *i.properties.Backend.Ndk.Apps_enabled
}

func (i *aidlInterface) shouldGenerateRustBackend() bool {
	return i.properties.Backend.Rust.Enabled != nil && *i.properties.Backend.Rust.Enabled
}

func (i *aidlInterface) gatherInterface(mctx android.LoadHookContext) {
	aidlInterfaces := aidlInterfaces(mctx.Config())
	aidlInterfaceMutex.Lock()
	defer aidlInterfaceMutex.Unlock()
	*aidlInterfaces = append(*aidlInterfaces, i)
}

func addUnstableModule(mctx android.LoadHookContext, moduleName string) {
	unstableModules := unstableModules(mctx.Config())
	unstableModuleMutex.Lock()
	defer unstableModuleMutex.Unlock()
	*unstableModules = append(*unstableModules, moduleName)
}

func checkImports(mctx android.BottomUpMutatorContext) {
	if i, ok := mctx.Module().(*aidlInterface); ok {
		for _, anImportWithVersion := range i.properties.Imports {
			anImport, version := parseModuleWithVersion(anImportWithVersion)
			other := lookupInterface(anImport, mctx.Config())

			if other == nil {
				if mctx.Config().AllowMissingDependencies() {
					continue
				}
				mctx.PropertyErrorf("imports", "Import does not exist: "+anImport)
			}
			if version != "" {
				candidateVersions := concat(other.properties.Versions, []string{other.nextVersion()})
				if !android.InList(version, candidateVersions) {
					mctx.PropertyErrorf("imports", "%q depends on %q version %q(%q), which doesn't exist. The version must be one of %q", i.ModuleBase.Name(), anImport, version, anImportWithVersion, candidateVersions)
				}
			}
			if i.shouldGenerateJavaBackend() && !other.shouldGenerateJavaBackend() {
				mctx.PropertyErrorf("backend.java.enabled",
					"Java backend not enabled in the imported AIDL interface %q", anImport)
			}

			if i.shouldGenerateCppBackend() && !other.shouldGenerateCppBackend() {
				mctx.PropertyErrorf("backend.cpp.enabled",
					"C++ backend not enabled in the imported AIDL interface %q", anImport)
			}

			if i.shouldGenerateNdkBackend() && !other.shouldGenerateNdkBackend() {
				mctx.PropertyErrorf("backend.ndk.enabled",
					"NDK backend not enabled in the imported AIDL interface %q", anImport)
			}

			if i.shouldGenerateRustBackend() && !other.shouldGenerateRustBackend() {
				mctx.PropertyErrorf("backend.rust.enabled",
					"Rust backend not enabled in the imported AIDL interface %q", anImport)
			}
		}
	}
}

func (i *aidlInterface) checkGenTrace(mctx android.LoadHookContext) {
	if !proptools.Bool(i.properties.Gen_trace) {
		return
	}
	if i.shouldGenerateJavaBackend() && !proptools.Bool(i.properties.Backend.Java.Platform_apis) {
		mctx.PropertyErrorf("gen_trace", "must be false when Java backend is enabled and platform_apis is false")
	}
}

func (i *aidlInterface) checkStability(mctx android.LoadHookContext) {
	if i.properties.Stability == nil {
		return
	}

	if proptools.Bool(i.properties.Unstable) {
		mctx.PropertyErrorf("stability", "must be empty when \"unstable\" is true")
	}

	// TODO(b/136027762): should we allow more types of stability (e.g. for APEX) or
	// should we switch this flag to be something like "vintf { enabled: true }"
	isVintf := "vintf" == proptools.String(i.properties.Stability)
	if !isVintf {
		mctx.PropertyErrorf("stability", "must be empty or \"vintf\"")
	}
}
func (i *aidlInterface) checkVersions(mctx android.LoadHookContext) {
	versions := make(map[string]bool)
	intVersions := make([]int, 0, len(i.properties.Versions))
	for _, ver := range i.properties.Versions {
		if _, dup := versions[ver]; dup {
			mctx.PropertyErrorf("versions", "duplicate found", ver)
			continue
		}
		versions[ver] = true
		n, err := strconv.Atoi(ver)
		if err != nil {
			mctx.PropertyErrorf("versions", "%q is not an integer", ver)
			continue
		}
		if n <= 0 {
			mctx.PropertyErrorf("versions", "should be > 0, but is %v", ver)
			continue
		}
		intVersions = append(intVersions, n)

	}
	if !mctx.Failed() && !sort.IntsAreSorted(intVersions) {
		mctx.PropertyErrorf("versions", "should be sorted, but is %v", i.properties.Versions)
	}
}
func (i *aidlInterface) checkVndkUseVersion(mctx android.LoadHookContext) {
	if i.properties.Vndk_use_version == nil {
		return
	}
	if !i.hasVersion() {
		mctx.PropertyErrorf("vndk_use_version", "This does not make sense when no 'versions' are specified.")

	}
	if *i.properties.Vndk_use_version == i.nextVersion() {
		return
	}
	for _, ver := range i.properties.Versions {
		if *i.properties.Vndk_use_version == ver {
			return
		}
	}
	mctx.PropertyErrorf("vndk_use_version", "Specified version %q does not exist", *i.properties.Vndk_use_version)
}

func (i *aidlInterface) nextVersion() string {
	if proptools.Bool(i.properties.Unstable) {
		return ""
	}
	return nextVersion(i.properties.Versions)
}

func nextVersion(versions []string) string {
	if len(versions) == 0 {
		return "1"
	}
	ver := versions[len(versions)-1]
	i, err := strconv.Atoi(ver)
	if err != nil {
		panic(err)
	}
	return strconv.Itoa(i + 1)
}

func (i *aidlInterface) latestVersion() string {
	if !i.hasVersion() {
		return "0"
	}
	return i.properties.Versions[len(i.properties.Versions)-1]
}

func (i *aidlInterface) hasVersion() bool {
	return len(i.properties.Versions) > 0
}

func hasVersionSuffix(moduleName string) bool {
	hasVersionSuffix, _ := regexp.MatchString("-V\\d+$", moduleName)
	return hasVersionSuffix
}

func parseModuleWithVersion(moduleName string) (string, string) {
	if hasVersionSuffix(moduleName) {
		versionIdx := strings.LastIndex(moduleName, "-V")
		if versionIdx == -1 {
			panic("-V must exist in this context")
		}
		return moduleName[:versionIdx], moduleName[versionIdx+len("-V"):]
	}
	return moduleName, ""
}

func trimVersionSuffixInList(moduleNames []string) []string {
	return wrapFunc("", moduleNames, "", func(moduleName string) string {
		moduleNameWithoutVersion, _ := parseModuleWithVersion(moduleName)
		return moduleNameWithoutVersion
	})
}

func aidlInterfaceHook(mctx android.LoadHookContext, i *aidlInterface) {
	if hasVersionSuffix(i.ModuleBase.Name()) {
		mctx.PropertyErrorf("name", "aidl_interface should not have '-V<number> suffix")
	}
	i.properties.ImportsWithoutVersion = trimVersionSuffixInList(i.properties.Imports)
	if !isRelativePath(i.properties.Local_include_dir) {
		mctx.PropertyErrorf("local_include_dir", "must be relative path: "+i.properties.Local_include_dir)
	}
	var importPaths []string
	importPaths = append(importPaths, filepath.Join(mctx.ModuleDir(), i.properties.Local_include_dir))
	importPaths = append(importPaths, i.properties.Include_dirs...)

	i.properties.Full_import_paths = importPaths

	i.gatherInterface(mctx)
	i.checkStability(mctx)
	i.checkVersions(mctx)
	i.checkVndkUseVersion(mctx)
	i.checkGenTrace(mctx)

	if mctx.Failed() {
		return
	}

	var libs []string
	sdkIsFinal := !mctx.Config().DefaultAppTargetSdk(mctx).IsPreview()

	unstable := proptools.Bool(i.properties.Unstable)

	if unstable {
		if i.hasVersion() {
			mctx.PropertyErrorf("versions", "cannot have versions for an unstable interface")
			return
		}
		if i.properties.Stability != nil {
			mctx.ModuleErrorf("unstable:true and stability:%q cannot happen at the same time", i.properties.Stability)
			return
		}
	}

	// Two different types of 'unstable' here
	// - 'unstable: true' meaning the module is never stable
	// - current unfrozen ToT version
	//
	// OEM branches may remove 'i.Owner()' here to apply the check to all interfaces, in
	// addition to core platform interfaces. Otherwise, we rely on vts_treble_vintf_vendor_test.
	requireFrozenVersion := !unstable && sdkIsFinal && i.Owner() == ""

	// surface error early, main check is via checkUnstableModuleMutator
	if requireFrozenVersion && !i.hasVersion() {
		mctx.PropertyErrorf("versions", "must be set (need to be frozen) when \"unstable\" is false, PLATFORM_VERSION_CODENAME is REL, and \"owner\" property is missing.")
	}

	versions := i.properties.Versions
	nextVersion := i.nextVersion()
	shouldGenerateLangBackendMap := map[string]bool{
		langCpp:         i.shouldGenerateCppBackend(),
		langNdk:         i.shouldGenerateAppNdkBackend(),
		langNdkPlatform: i.shouldGenerateNdkBackend(),
		langJava:        i.shouldGenerateJavaBackend(),
		langRust:        i.shouldGenerateRustBackend()}
	for lang, shouldGenerate := range shouldGenerateLangBackendMap {
		if !shouldGenerate {
			continue
		}
		libs = append(libs, addLibrary(mctx, i, nextVersion, lang))
		if requireFrozenVersion {
			addUnstableModule(mctx, libs[len(libs)-1])
		}
		for _, version := range versions {
			libs = append(libs, addLibrary(mctx, i, version, lang))
		}
	}

	if unstable {
		apiDirRoot := filepath.Join(aidlApiDir, i.ModuleBase.Name())
		aidlDumps, _ := mctx.GlobWithDeps(filepath.Join(mctx.ModuleDir(), apiDirRoot, "**/*.aidl"), nil)
		if len(aidlDumps) != 0 {
			mctx.PropertyErrorf("unstable", "The interface is configured as unstable, "+
				"but API dumps exist under %q. Unstable interface cannot have dumps.", apiDirRoot)
		}
	} else {
		addApiModule(mctx, i)
	}

	if proptools.Bool(i.properties.VndkProperties.Vndk.Enabled) {
		if "vintf" != proptools.String(i.properties.Stability) {
			mctx.PropertyErrorf("stability", "must be \"vintf\" if the module is for VNDK.")
		}
	}

	// Reserve this module name for future use
	mctx.CreateModule(phony.PhonyFactory, &phonyProperties{
		Name: proptools.StringPtr(i.ModuleBase.Name()),
	})

	i.internalModuleNames = libs
}

func (i *aidlInterface) commonBackendProperties(lang string) CommonBackendProperties {
	switch lang {
	case langCpp:
		return i.properties.Backend.Cpp.CommonBackendProperties
	case langJava:
		return i.properties.Backend.Java.CommonBackendProperties
	case langNdk, langNdkPlatform:
		return i.properties.Backend.Ndk.CommonBackendProperties
	case langRust:
		return i.properties.Backend.Rust.CommonBackendProperties
	default:
		panic(fmt.Errorf("unsupported language backend %q\n", lang))
	}
}

// srcsVisibility gives the value for the `visibility` property of the source gen module for the
// language backend `lang`. By default, the source gen module is not visible to the clients of
// aidl_interface (because it's an impl detail), but when `backend.<backend>.srcs_available` is set
// to true, the source gen module follows the visibility of the aidl_interface module.
func srcsVisibility(mctx android.LoadHookContext, lang string) []string {
	if a, ok := mctx.Module().(*aidlInterface); !ok {
		panic(fmt.Errorf("%q is not aidl_interface", mctx.Module().String()))
	} else {
		if proptools.Bool(a.commonBackendProperties(lang).Srcs_available) {
			// Returning nil so that the visibility of the source module defaults to the
			// the package-level default visibility. This way, the source module gets
			// the same visibility as the library modules.
			return nil
		}
	}
	return []string{
		"//" + mctx.ModuleDir(),
		// system/tools/aidl/build is always added because aidl_metadata_json in the
		// directory has dependencies to all aidl_interface modules.
		"//system/tools/aidl/build",
	}
}

func (i *aidlInterface) Name() string {
	return i.ModuleBase.Name() + aidlInterfaceSuffix
}
func (i *aidlInterface) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	aidlRoot := android.PathForModuleSrc(ctx, i.properties.Local_include_dir)
	for _, src := range android.PathsForModuleSrc(ctx, i.properties.Srcs) {
		baseDir := getBaseDir(ctx, src, aidlRoot)
		relPath, _ := filepath.Rel(baseDir, src.String())
		computedType := strings.TrimSuffix(strings.ReplaceAll(relPath, "/", "."), ".aidl")
		i.computedTypes = append(i.computedTypes, computedType)
	}
}
func (i *aidlInterface) DepsMutator(ctx android.BottomUpMutatorContext) {
	ctx.AddReverseDependency(ctx.Module(), nil, aidlMetadataSingletonName)
}

var (
	aidlInterfacesKey   = android.NewOnceKey("aidlInterfaces")
	unstableModulesKey  = android.NewOnceKey("unstableModules")
	aidlDepsKey         = android.NewOnceKey("aidlDeps")
	aidlInterfaceMutex  sync.Mutex
	unstableModuleMutex sync.Mutex
	aidlDepsMutex       sync.RWMutex
)

func aidlInterfaces(config android.Config) *[]*aidlInterface {
	return config.Once(aidlInterfacesKey, func() interface{} {
		return &[]*aidlInterface{}
	}).(*[]*aidlInterface)
}

func unstableModules(config android.Config) *[]string {
	return config.Once(unstableModulesKey, func() interface{} {
		return &[]string{}
	}).(*[]string)
}

type DepInfo struct {
	ifaceName string
	verLang   string
	isSource  bool
}

func (d DepInfo) moduleName() string {
	name := d.ifaceName + d.verLang
	if d.isSource {
		name += "-source"
	}
	return name
}

func aidlDeps(config android.Config) map[android.Module][]DepInfo {
	return config.Once(aidlDepsKey, func() interface{} {
		return make(map[android.Module][]DepInfo)
	}).(map[android.Module][]DepInfo)
}

func aidlInterfaceFactory() android.Module {
	i := &aidlInterface{}
	i.AddProperties(&i.properties)
	android.InitAndroidModule(i)
	android.AddLoadHook(i, func(ctx android.LoadHookContext) { aidlInterfaceHook(ctx, i) })
	return i
}

func lookupInterface(name string, config android.Config) *aidlInterface {
	for _, i := range *aidlInterfaces(config) {
		if i.ModuleBase.Name() == name {
			return i
		}
	}
	return nil
}
