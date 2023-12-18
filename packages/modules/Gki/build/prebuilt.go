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
	"fmt"
	"io"

	"android/soong/android"
	"android/soong/apex"
	"github.com/google/blueprint/proptools"
)

var (
	prebuiltApexTag = dependencyTag{name: "prebuilt_apex"}
)

type prebuiltGkiApexProperties struct {
	apex.PrebuiltProperties

	// Declared KMI version of the boot image. Example: "5.4-android12-0"
	Kmi_version *string
}

type prebuiltGkiApex struct {
	android.ModuleBase
	properties prebuiltGkiApexProperties

	extractedBootImage android.WritablePath
}

func init() {
	android.RegisterModuleType("prebuilt_gki_apex", prebuiltGkiApexFactory)
}

// Declare a prebuilt GKI APEX. When installed, the boot image is extracted from
// the module.
func prebuiltGkiApexFactory() android.Module {
	g := &prebuiltGkiApex{}
	g.AddProperties(&g.properties)
	android.InitAndroidModule(g)
	android.AddLoadHook(g, func(ctx android.LoadHookContext) { prebuiltGkiApexMutator(ctx, g) })
	return g
}
func prebuiltGkiApexMutator(mctx android.LoadHookContext, g *prebuiltGkiApex) {
	// Whether modules should be enabled according to board variables.
	enabled := boardDefinesKmiVersion(mctx, proptools.String(g.properties.Kmi_version))
	if !enabled {
		g.Disable()
	}

	// The prebuilt_apex module.
	mctx.CreateModule(apex.PrebuiltFactory, &moduleCommonProperties{
		Name:             proptools.StringPtr(g.BaseModuleName()),
		Enabled:          proptools.BoolPtr(enabled),
		Product_specific: proptools.BoolPtr(true),
	}, &g.properties.PrebuiltProperties)
}

// The appeared name of this prebuiltGkiApex object. Exposed to Soong to avoid conflicting with
// the generated prebuilt_apex module with name BaseModuleName().
func (g *prebuiltGkiApex) Name() string {
	return g.BaseModuleName() + "_boot_img"
}

func (g *prebuiltGkiApex) DepsMutator(ctx android.BottomUpMutatorContext) {
	ctx.AddDependency(ctx.Module(), prebuiltApexTag, g.BaseModuleName())
}

func (g *prebuiltGkiApex) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	var apexFile android.OptionalPath
	ctx.VisitDirectDepsWithTag(prebuiltApexTag, func(m android.Module) {
		if prebuiltApex, ok := m.(*apex.Prebuilt); ok {
			srcFiles, err := prebuiltApex.OutputFiles("")
			if err != nil {
				ctx.ModuleErrorf("Cannot get output files from %q: %s", ctx.OtherModuleName(m), err)
			} else if len(srcFiles) != 1 {
				ctx.ModuleErrorf("%q generated %d files", ctx.OtherModuleName(m), len(srcFiles))
			} else {
				apexFile = android.OptionalPathForPath(srcFiles[0])
			}
		} else {
			ctx.ModuleErrorf("%q is not a prebuilt_apex", ctx.OtherModuleName(m))
		}
	})
	if !apexFile.Valid() {
		ctx.ModuleErrorf("Can't determine the prebuilt APEX file")
		return
	}

	genDir := android.PathForModuleOut(ctx, "extracted")
	g.extractedBootImage = genDir.Join(ctx, "boot.img")

	rule := android.NewRuleBuilder(pctx, ctx)
	rule.Command().
		ImplicitOutput(g.extractedBootImage).
		BuiltTool("extract_img_from_apex").
		Flag("--tool").BuiltTool("debugfs").
		Flag("--tool").BuiltTool("delta_generator").
		Input(apexFile.Path()).
		Text(genDir.String())
	rule.Build("extractImgFromApex", "Extract boot image from prebuilt GKI APEX")

	ctx.Phony(g.BaseModuleName(), g.extractedBootImage)
}

func (g *prebuiltGkiApex) AndroidMk() android.AndroidMkData {
	return android.AndroidMkData{
		Custom: func(w io.Writer, name, prefix, moduleDir string, data android.AndroidMkData) {
			fmt.Fprintf(w, "ALL_MODULES.%s.EXTRACTED_BOOT_IMAGE := %s\n", name, g.extractedBootImage)
		},
	}
}
