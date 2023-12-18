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

import (
	"path/filepath"
	"strings"

	"android/soong/android"
	"android/soong/apex"
	"android/soong/etc"
	"android/soong/genrule"

	"github.com/google/blueprint/proptools"
)

type gkiApexProperties struct {
	// Path relative to $(PRODUCT_OUT) that points to the boot image. This is
	// passed to the generated makefile_goal.
	// Exactly one of [factory, product_out_path] must be set.
	Product_out_path *string

	// Declared KMI version of the boot image. Example: "5.4-android12-0"
	Kmi_version *string

	// The certificate to sign the OTA payload.
	// The name of a certificate in the default certificate directory, blank to
	// use the default product certificate,
	// or an android_app_certificate module name in the form ":module".
	Ota_payload_certificate *string

	// Whether test APEXes are generated. Test APEXes are named with
	// ${name}_test_high and ${name}_test_low, respectively.
	Gen_test *bool

	// Whether this APEX is installable to one of the partitions. Default:
	// see apex.installable.
	Installable *bool

	// Whether modules should be enabled according to board variables.
	ModulesEnabled bool `blueprint:"mutated"`
	// APEX package name that will be declared in the APEX manifest.
	// e.g. com.android.gki.kmi_5_4_android12_0
	ApexName *string `blueprint:"mutated"`
}

type gkiApex struct {
	android.ModuleBase
	properties gkiApexProperties
}

func init() {
	android.RegisterModuleType("gki_apex", gkiApexFactory)
}

// Declare a GKI APEX. Generate a set of modules to define an apex with name
// "com.android.gki" + sanitized(kmi_version).
func gkiApexFactory() android.Module {
	g := &gkiApex{}
	g.AddProperties(&g.properties)
	android.InitAndroidModule(g)
	android.AddLoadHook(g, func(ctx android.LoadHookContext) { gkiApexMutator(ctx, g) })
	return g
}

func gkiApexMutator(mctx android.LoadHookContext, g *gkiApex) {
	g.validateAndSetMutableProperties(mctx)
	g.createModulesRealApexes(mctx)
}

func (g *gkiApex) validateAndSetMutableProperties(mctx android.LoadHookContext) {
	// Parse kmi_version property to find APEX name.
	apexName, err := kmiVersionToApexName(proptools.String(g.properties.Kmi_version))
	if err != nil {
		mctx.PropertyErrorf("kmi_version", err.Error())
		return
	}

	// Set mutable properties.
	g.properties.ModulesEnabled = g.bootImgHasRules(mctx) && g.boardDefinesKmiVersion(mctx)
	g.properties.ApexName = proptools.StringPtr(apexName)
}

func testApexBundleFactory() android.Module {
	return apex.ApexBundleFactory(true /* testApex */, false /* art */)
}

// Create modules for a real APEX package that contains an OTA payload.
func (g *gkiApex) createModulesRealApexes(mctx android.LoadHookContext) {
	// Import $(PRODUCT_OUT)/boot.img to Soong
	bootImage := g.moduleName() + "_bootimage"
	mctx.CreateModule(android.MakefileGoalFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(bootImage),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &makefileGoalProperties{
		Product_out_path: g.properties.Product_out_path,
	})
	// boot.img -> kernel_release.txt
	mctx.CreateModule(genrule.GenRuleFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(g.kernelReleaseFileName()),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &genRuleProperties{
		Defaults: []string{"extract_kernel_release_defaults"},
		Srcs:     []string{":" + bootImage},
	})
	// boot.img -> payload.bin and payload_properties.txt
	otaPayloadGen := g.moduleName() + "_ota_payload_gen"
	mctx.CreateModule(rawImageOtaFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(otaPayloadGen),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &rawImageOtaProperties{
		Certificate: g.properties.Ota_payload_certificate,
		Image_goals: []string{"boot:" + bootImage},
	})
	// copy payload.bin to <apex>/etc/ota
	mctx.CreateModule(etc.PrebuiltEtcFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(g.otaPayloadName()),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &prebuiltEtcProperties{
		Src:                   proptools.StringPtr(":" + otaPayloadGen + "{" + payloadTag + "}"),
		Filename_from_src:     proptools.BoolPtr(true),
		Relative_install_path: proptools.StringPtr("ota"),
		Installable:           proptools.BoolPtr(false),
	})
	// copy payload_properties.txt to <apex>/etc/ota
	mctx.CreateModule(etc.PrebuiltEtcFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(g.otaPropertiesName()),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &prebuiltEtcProperties{
		Src:                   proptools.StringPtr(":" + otaPayloadGen + "{" + payloadPropertiesTag + "}"),
		Filename_from_src:     proptools.BoolPtr(true),
		Relative_install_path: proptools.StringPtr("ota"),
		Installable:           proptools.BoolPtr(false),
	})
	// Create the APEX module with name g.moduleName(). Use factory APEX version.
	g.createModulesRealApex(mctx, g.moduleName(), false, "")

	// Create test APEX modules if gen_test. Test packages are not installable.
	// Use hard-coded APEX version.
	if proptools.Bool(g.properties.Gen_test) {
		g.createModulesRealApex(mctx, g.moduleName()+"_test_high", true, "1000000000")
		g.createModulesRealApex(mctx, g.moduleName()+"_test_low", true, "1")
	}
}

func (g *gkiApex) createModulesRealApex(mctx android.LoadHookContext,
	moduleName string,
	isTestApex bool,
	overrideApexVersion string) {
	// Check kmi_version property against kernel_release.txt, then
	// kernel_release.txt -> apex_manifest.json.
	apexManifest := moduleName + "_apex_manifest"
	mctx.CreateModule(genrule.GenRuleFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(apexManifest),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &genRuleProperties{
		Tools: []string{"build_gki_apex_manifest"},
		Out:   []string{"apex_manifest.json"},
		Srcs:  []string{":" + g.kernelReleaseFileName()},
		Cmd:   proptools.StringPtr(g.createApexManifestCmd(overrideApexVersion)),
	})

	// The APEX module.

	// For test APEXes, if module is not enabled because KMI version is not
	// compatible with the device, create a stub module that produces an empty
	// file. This is so that the module name can be used in tests.
	if isTestApex && !g.properties.ModulesEnabled {
		mctx.CreateModule(genrule.GenRuleFactory, &moduleCommonProperties{
			Name: proptools.StringPtr(moduleName),
		}, &genRuleProperties{
			Out: []string{moduleName + ".apex"},
			Cmd: proptools.StringPtr(`touch $(out)`),
		})
		return
	}

	// For test APEXes, if module is enabled, build an apex_test with installable: false.
	// For installed APEXes, build apex, respecting installable and enabled.
	apexFactory := apex.BundleFactory
	overrideInstallable := g.properties.Installable
	if isTestApex {
		apexFactory = testApexBundleFactory
		overrideInstallable = proptools.BoolPtr(false)
	}

	mctx.CreateModule(apexFactory, &moduleCommonProperties{
		Name:    proptools.StringPtr(moduleName),
		Enabled: proptools.BoolPtr(g.properties.ModulesEnabled),
	}, &apexProperties{
		Apex_name: g.properties.ApexName,
		Manifest:  proptools.StringPtr(":" + apexManifest),
		Defaults:  []string{"com.android.gki_defaults"},
		// A real GKI APEX cannot be preinstalled to the device.
		// It can only be provided as an update.
		Installable: overrideInstallable,
		Prebuilts: []string{
			g.otaPayloadName(),
			g.otaPropertiesName(),
		},
	})
}

// Original module name as specified by the "name" property.
// This is also the APEX module name, i.e. the file name of the APEX file.
// This is also the prefix of names of all generated modules that the phony module depends on.
// e.g. com.android.gki.kmi_5_4_android12_0_boot
func (g *gkiApex) moduleName() string {
	return g.BaseModuleName()
}

// The appeared name of this gkiApex object. Exposed to Soong to avoid conflicting with
// the generated APEX module with name moduleName().
// e.g. com.android.gki.kmi_5_4_android12_0_boot_all
func (g *gkiApex) Name() string {
	return g.moduleName() + "_all"
}

// Names for intermediate modules.
func (g *gkiApex) kernelReleaseFileName() string {
	return g.moduleName() + "_bootimage_kernel_release_file"
}

func (g *gkiApex) otaPayloadName() string {
	return g.moduleName() + "_ota_payload"
}

func (g *gkiApex) otaPropertiesName() string {
	return g.moduleName() + "_ota_payload_properties"
}

// If the boot image pointed at product_out_path has no rule to be generated, do not generate any
// build rules for this gki_apex module. For example, if this gki_apex module is:
//     { name: "foo", product_out_path: "boot-bar.img" }
// But there is no rule to generate boot-bar.img, then
// - `m foo` fails with `unknown target 'foo'`
// - checkbuild is still successful. The module foo doesn't even exist, so there
//   is no dependency on boot-bar.img
//
// There is a rule to generate "boot-foo.img" if "kernel-foo" is in BOARD_KERNEL_BINARIES.
// As a special case, there is a rule to generate "boot.img" if BOARD_KERNEL_BINARIES is empty,
// or "kernel" is in BOARD_KERNEL_BINARIES.
func (g *gkiApex) bootImgHasRules(mctx android.EarlyModuleContext) bool {
	kernelNames := mctx.DeviceConfig().BoardKernelBinaries()
	if len(kernelNames) == 0 {
		return proptools.String(g.properties.Product_out_path) == "boot.img"
	}
	for _, kernelName := range kernelNames {
		validBootImagePath := strings.Replace(kernelName, "kernel", "boot", -1) + ".img"
		if proptools.String(g.properties.Product_out_path) == validBootImagePath {
			return true
		}
	}
	return false
}

// Only generate if this module's kmi_version property is in BOARD_KERNEL_MODULE_INTERFACE_VERSIONS.
// Otherwise, this board does not support GKI APEXes, so no modules are generated at all.
// This function also avoids building invalid modules in checkbuild. For example, if these
// gki_apex modules are defined:
//   gki_apex { name: "boot-kmi-1", kmi_version: "1", product_out_path: "boot.img" }
//   gki_apex { name: "boot-kmi-2", kmi_version: "2", product_out_path: "boot.img" }
// But a given device's $PRODUCT_OUT/boot.img can only support at most one KMI version.
// Disable some modules accordingly to make sure checkbuild still works.
func boardDefinesKmiVersion(mctx android.EarlyModuleContext, kmiVersion string) bool {
	kmiVersions := mctx.DeviceConfig().BoardKernelModuleInterfaceVersions()
	return android.InList(kmiVersion, kmiVersions)
}

func (g *gkiApex) boardDefinesKmiVersion(mctx android.EarlyModuleContext) bool {
	return boardDefinesKmiVersion(mctx, proptools.String(g.properties.Kmi_version))
}

// Transform kernel release file in $(in) to KMI version + sublevel.
// e.g. 5.4.42-android12-0 => name: "com.android.gki.kmi_5_4_android12_0", version: "300000000"
// Finally, write APEX manifest JSON to $(out).
func (g *gkiApex) createApexManifestCmd(apexVersion string) string {
	ret := `$(location build_gki_apex_manifest) ` +
		`--kmi_version "` + proptools.String(g.properties.Kmi_version) + `" ` +
		`--apex_manifest $(out) --kernel_release_file $(in)`
	// Override version field if set.
	if apexVersion != "" {
		ret += ` --apex_version ` + apexVersion
	}
	return ret
}

func (g *gkiApex) DepsMutator(ctx android.BottomUpMutatorContext) {
}

func (g *gkiApex) GenerateAndroidBuildActions(ctx android.ModuleContext) {
}

// OTA payload binary is signed with default_system_dev_certificate, which is equivalent to
// DefaultAppCertificate().
func getDefaultCertificate(ctx android.EarlyModuleContext) string {
	pem, _ := ctx.Config().DefaultAppCertificate(ctx)
	return strings.TrimSuffix(pem.String(), filepath.Ext(pem.String()))
}
