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

	"fmt"
	"io"

	"github.com/google/blueprint"
)

var (
	aidlDumpMappingsRule = pctx.StaticRule("aidlDumpMappingsRule", blueprint.RuleParams{
		Command: `rm -rf "${outDir}" && mkdir -p "${outDir}" && ` +
			`${aidlCmd} --apimapping ${outDir}/intermediate.txt ${in} ${imports} && ` +
			`${aidlToJniCmd} ${outDir}/intermediate.txt ${out}`,
		CommandDeps: []string{"${aidlCmd}"},
	}, "imports", "outDir")
)

func init() {
	android.RegisterModuleType("aidl_mapping", aidlMappingFactory)
}

type aidlMappingProperties struct {
	// Source file of this prebuilt.
	Srcs   []string `android:"path"`
	Output string
}

type aidlMapping struct {
	android.ModuleBase
	properties     aidlMappingProperties
	outputFilePath android.WritablePath
}

func (s *aidlMapping) DepsMutator(ctx android.BottomUpMutatorContext) {
}

func (s *aidlMapping) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	srcs, imports := getPaths(ctx, s.properties.Srcs, "")

	s.outputFilePath = android.PathForModuleOut(ctx, s.properties.Output)
	outDir := android.PathForModuleGen(ctx)
	ctx.Build(pctx, android.BuildParams{
		Rule:   aidlDumpMappingsRule,
		Inputs: srcs,
		Output: s.outputFilePath,
		Args: map[string]string{
			"imports": android.JoinWithPrefix(imports, " -I"),
			"outDir":  outDir.String(),
		},
	})
}

func InitAidlMappingModule(s *aidlMapping) {
	s.AddProperties(&s.properties)
}

func aidlMappingFactory() android.Module {
	module := &aidlMapping{}
	InitAidlMappingModule(module)
	android.InitAndroidModule(module)
	return module
}

func (m *aidlMapping) AndroidMk() android.AndroidMkData {
	return android.AndroidMkData{
		Custom: func(w io.Writer, name, prefix, moduleDir string, data android.AndroidMkData) {
			android.WriteAndroidMkData(w, data)
			targetName := m.Name()
			fmt.Fprintln(w, ".PHONY:", targetName)
			fmt.Fprintln(w, targetName+":", m.outputFilePath.String())
		},
	}
}
