// Copyright 2018 Google Inc. All rights reserved.
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

package xsdc

import (
	"android/soong/android"
	"android/soong/java"
	"path/filepath"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"
)

func init() {
	pctx.Import("android/soong/java/config")
	android.RegisterModuleType("xsd_config", xsdConfigFactory)

	android.PreArchMutators(func(ctx android.RegisterMutatorsContext) {
		ctx.TopDown("xsd_config", xsdConfigMutator).Parallel()
	})
}

var (
	pctx = android.NewPackageContext("android/xsdc")

	xsdc         = pctx.HostBinToolVariable("xsdcCmd", "xsdc")
	xsdcJavaRule = pctx.StaticRule("xsdcJavaRule", blueprint.RuleParams{
		Command: `rm -rf "${out}.temp" && mkdir -p "${out}.temp" && ` +
			`${xsdcCmd} $in -p $pkgName -o ${out}.temp -j $args && ` +
			`${config.SoongZipCmd} -jar -o ${out} -C ${out}.temp -D ${out}.temp && ` +
			`rm -rf ${out}.temp`,
		CommandDeps: []string{"${xsdcCmd}", "${config.SoongZipCmd}"},
		Description: "xsdc Java ${in} => ${out}",
	}, "pkgName", "args")

	xsdcCppRule = pctx.StaticRule("xsdcCppRule", blueprint.RuleParams{
		Command: `rm -rf "${outDir}" && ` +
			`${xsdcCmd} $in -p $pkgName -o ${outDir} -c $args`,
		CommandDeps: []string{"${xsdcCmd}", "${config.SoongZipCmd}"},
		Description: "xsdc C++ ${in} => ${out}",
	}, "pkgName", "outDir", "args")

	xsdConfigRule = pctx.StaticRule("xsdConfigRule", blueprint.RuleParams{
		Command:     "cp -f ${in} ${output}",
		Description: "copy the xsd file: ${in} => ${output}",
	}, "output")
)

type xsdConfigProperties struct {
	Srcs         []string
	Package_name *string
	Api_dir      *string
	Gen_writer   *bool
	Nullability  *bool

	// Whether has{element or atrribute} methods are set to public.
	// It is not applied to C++, because these methods are always
	// generated to public for C++.
	Gen_has *bool
	// Only generate code for enum converters. Applies to C++ only.
	// This is useful for memory footprint reduction since it avoids
	// depending on libxml2.
	Enums_only *bool
	// Only generate complementary code for XML parser. Applies to C++ only.
	// The code being generated depends on the enum converters module.
	Parser_only *bool
	// Whether getter name of boolean element or attribute is getX or isX.
	// Default value is false. If the property is true, getter name is isX.
	Boolean_getter *bool
}

type xsdConfig struct {
	android.ModuleBase

	properties xsdConfigProperties

	genOutputDir android.Path
	genOutputs_j android.WritablePath
	genOutputs_c android.WritablePaths
	genOutputs_h android.WritablePaths

	docsPath android.Path

	xsdConfigPath android.OptionalPath
	genOutputs    android.Paths
}

var _ android.SourceFileProducer = (*xsdConfig)(nil)

type ApiToCheck struct {
	Api_file         *string
	Removed_api_file *string
	Args             *string
}

type CheckApi struct {
	Last_released ApiToCheck
	Current       ApiToCheck
}
type DroidstubsProperties struct {
	Name                 *string
	Installable          *bool
	Srcs                 []string
	Sdk_version          *string
	Args                 *string
	Api_filename         *string
	Removed_api_filename *string
	Check_api            CheckApi
}

func (module *xsdConfig) GeneratedSourceFiles() android.Paths {
	return module.genOutputs_c.Paths()
}

func (module *xsdConfig) Srcs() android.Paths {
	return append(module.genOutputs, module.genOutputs_j)
}

func (module *xsdConfig) GeneratedDeps() android.Paths {
	return module.genOutputs_h.Paths()
}

func (module *xsdConfig) GeneratedHeaderDirs() android.Paths {
	return android.Paths{module.genOutputDir}
}

func (module *xsdConfig) DepsMutator(ctx android.BottomUpMutatorContext) {
	android.ExtractSourcesDeps(ctx, module.properties.Srcs)
}

func (module *xsdConfig) generateXsdConfig(ctx android.ModuleContext) {
	if !module.xsdConfigPath.Valid() {
		return
	}

	output := android.PathForModuleGen(ctx, module.Name()+".xsd")
	module.genOutputs = append(module.genOutputs, output)

	ctx.ModuleBuild(pctx, android.ModuleBuildParams{
		Rule:   xsdConfigRule,
		Input:  module.xsdConfigPath.Path(),
		Output: output,
		Args: map[string]string{
			"output": output.String(),
		},
	})
}

func (module *xsdConfig) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	if len(module.properties.Srcs) != 1 {
		ctx.PropertyErrorf("srcs", "xsd_config must be one src")
	}

	ctx.VisitDirectDeps(func(to android.Module) {
		if doc, ok := to.(java.ApiFilePath); ok {
			module.docsPath = doc.ApiFilePath()
		}
	})

	srcFiles := ctx.ExpandSources(module.properties.Srcs, nil)
	xsdFile := srcFiles[0]

	pkgName := *module.properties.Package_name
	filenameStem := strings.Replace(pkgName, ".", "_", -1)

	args := ""
	if proptools.Bool(module.properties.Gen_writer) {
		args = "-w"
	}

	if proptools.Bool(module.properties.Nullability) {
		args = args + " -n "
	}

	if proptools.Bool(module.properties.Gen_has) {
		args = args + " -g "
	}

	if proptools.Bool(module.properties.Enums_only) {
		args = args + " -e "
	}

	if proptools.Bool(module.properties.Parser_only) {
		args = args + " -x "
	}

	if proptools.Bool(module.properties.Boolean_getter) {
		args = args + " -b "
	}

	module.genOutputs_j = android.PathForModuleGen(ctx, "java", filenameStem+"_xsdcgen.srcjar")

	ctx.Build(pctx, android.BuildParams{
		Rule:        xsdcJavaRule,
		Description: "xsdc " + xsdFile.String(),
		Input:       xsdFile,
		Implicit:    module.docsPath,
		Output:      module.genOutputs_j,
		Args: map[string]string{
			"pkgName": pkgName,
			"args":    args,
		},
	})

	if proptools.Bool(module.properties.Enums_only) {
		module.genOutputs_c = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", filenameStem+"_enums.cpp")}
		module.genOutputs_h = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", "include/"+filenameStem+"_enums.h")}
	} else if proptools.Bool(module.properties.Parser_only) {
		module.genOutputs_c = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", filenameStem+".cpp")}
		module.genOutputs_h = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", "include/"+filenameStem+".h")}
	} else {
		module.genOutputs_c = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", filenameStem+".cpp"),
			android.PathForModuleGen(ctx, "cpp", filenameStem+"_enums.cpp")}
		module.genOutputs_h = android.WritablePaths{
			android.PathForModuleGen(ctx, "cpp", "include/"+filenameStem+".h"),
			android.PathForModuleGen(ctx, "cpp", "include/"+filenameStem+"_enums.h")}
	}
	module.genOutputDir = android.PathForModuleGen(ctx, "cpp", "include")

	ctx.Build(pctx, android.BuildParams{
		Rule:            xsdcCppRule,
		Description:     "xsdc " + xsdFile.String(),
		Input:           xsdFile,
		Implicit:        module.docsPath,
		Outputs:         module.genOutputs_c,
		ImplicitOutputs: module.genOutputs_h,
		Args: map[string]string{
			"pkgName": pkgName,
			"outDir":  android.PathForModuleGen(ctx, "cpp").String(),
			"args":    args,
		},
	})
	module.xsdConfigPath = android.ExistentPathForSource(ctx, xsdFile.String())
	module.generateXsdConfig(ctx)
}

func xsdConfigMutator(mctx android.TopDownMutatorContext) {
	if module, ok := mctx.Module().(*xsdConfig); ok {
		name := module.BaseModuleName()

		args := " --stub-packages " + *module.properties.Package_name +
			" --hide MissingPermission --hide BroadcastBehavior" +
			" --hide HiddenSuperclass --hide DeprecationMismatch --hide UnavailableSymbol" +
			" --hide SdkConstant --hide HiddenTypeParameter --hide Todo --hide Typo"

		api_dir := proptools.StringDefault(module.properties.Api_dir, "api")

		currentApiFileName := filepath.Join(api_dir, "current.txt")
		removedApiFileName := filepath.Join(api_dir, "removed.txt")

		check_api := CheckApi{}

		check_api.Current.Api_file = proptools.StringPtr(currentApiFileName)
		check_api.Current.Removed_api_file = proptools.StringPtr(removedApiFileName)

		check_api.Last_released.Api_file = proptools.StringPtr(
			filepath.Join(api_dir, "last_current.txt"))
		check_api.Last_released.Removed_api_file = proptools.StringPtr(
			filepath.Join(api_dir, "last_removed.txt"))

		mctx.CreateModule(java.DroidstubsFactory, &DroidstubsProperties{
			Name:                 proptools.StringPtr(name + ".docs"),
			Srcs:                 []string{":" + name},
			Args:                 proptools.StringPtr(args),
			Api_filename:         proptools.StringPtr(currentApiFileName),
			Removed_api_filename: proptools.StringPtr(removedApiFileName),
			Check_api:            check_api,
			Installable:          proptools.BoolPtr(false),
			Sdk_version:          proptools.StringPtr("core_platform"),
		})
	}
}

func xsdConfigFactory() android.Module {
	module := &xsdConfig{}
	module.AddProperties(&module.properties)
	android.InitAndroidModule(module)

	return module
}
