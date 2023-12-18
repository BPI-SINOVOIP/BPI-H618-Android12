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

// A special genrule that creates OTA payload and payload_properties from a raw
// image. This rule is created so that the two outputs, payload and
// payload_properties, can be distinguished with tags.

package gki

import (
	"fmt"
	"sort"
	"strings"

	"android/soong/android"
	"android/soong/java"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"
)

type dependencyTag struct {
	blueprint.BaseDependencyTag
	name string
}

// {"foo": "fooVal", "bar": "barVal"} -> ["${foo}", "${bar}"]
func keysToVars(deps map[string]string) []string {
	var ret []string
	for dep := range deps {
		ret = append(ret, fmt.Sprintf("${%s}", dep))
	}
	sort.Strings(ret)
	return ret
}

var (
	certificateTag = dependencyTag{name: "certificate"}
	rawImageTag    = dependencyTag{name: "raw_image"}

	pctx = android.NewPackageContext("android/gki")

	otaFromRawImageDeps = map[string]string{
		"ota_from_raw_image": "ota_from_raw_image",

		// Needed by ota_from_target_files
		"brillo_update_payload": "brillo_update_payload",

		// Needed by brillo_update_payload
		"delta_generator": "delta_generator",
		// b/171581299: shflags isn't built to the path where HostBinToolVariable
		// points to without explicitly declaring it, even if it is stated as
		// required by brillo_update_payload.
		"shflags": "lib/shflags/shflags",

		// Needed by GetBootImageTimestamp
		"lz4":            "lz4",
		"toybox":         "toybox",
		"unpack_bootimg": "unpack_bootimg",
	}

	otaFromRawImageVarDeps = keysToVars(otaFromRawImageDeps)

	otaFromRawImageRule = pctx.AndroidStaticRule("ota_from_raw_image", blueprint.RuleParams{
		Command: `${ota_from_raw_image} --tools ` + strings.Join(otaFromRawImageVarDeps, " ") +
			` ${kwargs} --out ${outDir} -- ${inputArg}`,
		CommandDeps: otaFromRawImageVarDeps,
		Description: "ota_from_raw_image ${outDir}",
	}, "kwargs", "outDir", "inputArg")

	// Tags to OutputFiles
	payloadTag           = "payload"
	payloadPropertiesTag = "properties"
)

func init() {
	for dep := range otaFromRawImageDeps {
		pctx.HostBinToolVariable(dep, otaFromRawImageDeps[dep])
	}
	// Intentionally not register this module so that it can only be constructed by gki_apex.
}

type rawImageOtaProperties struct {
	// The name of a certificate in the default certificate directory, blank to use the default product certificate,
	// or an android_app_certificate module name in the form ":module".
	Certificate *string

	// A set of images and their related modules. Must be in this form
	// IMAGE_NAME:MODULE, where IMAGE_NAME is an image name like "boot", and
	// MODULE is the name of a makefile_goal.
	Image_goals []string
}

type rawImageOta struct {
	android.ModuleBase
	properties rawImageOtaProperties

	pem android.Path
	key android.Path

	outPayload    android.WritablePath
	outProperties android.WritablePath
}

// Declare a rule that generates a signed OTA payload from a raw image. This
// includes payload.bin and payload_properties.txt.
func rawImageOtaFactory() android.Module {
	r := &rawImageOta{}
	r.AddProperties(&r.properties)
	android.InitAndroidModule(r)
	return r
}

func (r *rawImageOta) OutputFiles(tag string) (android.Paths, error) {
	switch tag {
	case "":
		return android.Paths{r.outPayload, r.outProperties}, nil
	case payloadTag:
		return android.Paths{r.outPayload}, nil
	case payloadPropertiesTag:
		return android.Paths{r.outProperties}, nil
	default:
		return nil, fmt.Errorf("unsupported module reference tag %q", tag)
	}
}

var _ android.OutputFileProducer = (*rawImageOta)(nil)

func (r *rawImageOta) getCertString(ctx android.BaseModuleContext) string {
	moduleName := ctx.ModuleName()
	certificate, overridden := ctx.DeviceConfig().OverrideCertificateFor(moduleName)
	if overridden {
		return ":" + certificate
	}
	return proptools.String(r.properties.Certificate)
}

// Returns module->image_name mapping, e.g. "bootimage_soong"->"boot"
func (r *rawImageOta) goalToImage(ctx android.EarlyModuleContext) map[string]string {
	ret := map[string]string{}
	for _, imageGoal := range r.properties.Image_goals {
		lst := strings.Split(imageGoal, ":")
		if len(lst) != 2 {
			ctx.PropertyErrorf("image_goals", "Must be in the form IMAGE_NAME:MODULE")
			return map[string]string{}
		}
		ret[lst[1]] = lst[0]
	}
	return ret
}

func (r *rawImageOta) DepsMutator(ctx android.BottomUpMutatorContext) {
	// Add dependency to modules in image_goals
	for module, _ := range r.goalToImage(ctx) {
		ctx.AddVariationDependencies(nil, rawImageTag, module)
	}
	// Add dependency to certificate module, if any.
	cert := android.SrcIsModule(r.getCertString(ctx))
	if cert != "" {
		ctx.AddVariationDependencies(nil, certificateTag, cert)
	}
}

func (r *rawImageOta) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	inputArg := []string{}
	kwargs := []string{}
	implicits := android.Paths{}

	// Handle image_goals
	goalToImage := r.goalToImage(ctx)
	ctx.VisitDirectDepsWithTag(rawImageTag, func(module android.Module) {
		depName := ctx.OtherModuleName(module)
		imgPath := android.OutputFileForModule(ctx, module, "")
		if imgPath != nil {
			implicits = append(implicits, imgPath)
			inputArg = append(inputArg, goalToImage[depName]+":"+imgPath.String())
		} else {
			ctx.ModuleErrorf("image dependency %q does not generate any output", depName)
		}
	})

	// Handle certificate
	ctx.VisitDirectDepsWithTag(certificateTag, func(module android.Module) {
		depName := ctx.OtherModuleName(module)
		if cert, ok := module.(*java.AndroidAppCertificate); ok {
			r.pem = cert.Certificate.Pem
			r.key = cert.Certificate.Key
		} else {
			ctx.ModuleErrorf("certificate dependency %q must be an android_app_certificate module", depName)
		}
	})
	r.setCertificateAndPrivateKey(ctx)
	keyName, keyError := removeCertExt(r.pem)
	if keyError != nil {
		ctx.ModuleErrorf("Cannot get certificate to sign the OTA payload binary: " + keyError.Error())
	}
	implicits = append(implicits, r.pem, r.key)
	kwargs = append(kwargs, "--key "+proptools.String(keyName))

	// Set outputs
	outDir := android.PathForModuleGen(ctx, "payload_files")
	r.outPayload = outDir.Join(ctx, "payload.bin")
	r.outProperties = outDir.Join(ctx, "payload_properties.txt")

	ctx.Build(pctx, android.BuildParams{
		Rule:        otaFromRawImageRule,
		Description: "Generate OTA from raw image",
		Implicits:   implicits,
		Outputs:     android.WritablePaths{r.outPayload, r.outProperties},
		Args: map[string]string{
			"kwargs":   strings.Join(kwargs, " "),
			"outDir":   outDir.String(),
			"inputArg": strings.Join(inputArg, " "),
		},
	})
}

func (r *rawImageOta) setCertificateAndPrivateKey(ctx android.ModuleContext) {
	if r.pem == nil {
		cert := proptools.String(r.properties.Certificate)
		if cert == "" {
			pem, key := ctx.Config().DefaultAppCertificate(ctx)
			r.pem = pem
			r.key = key
		} else {
			defaultDir := ctx.Config().DefaultAppCertificateDir(ctx)
			r.pem = defaultDir.Join(ctx, cert+".x509.pem")
			r.key = defaultDir.Join(ctx, cert+".pk8")
		}
	}
}

func removeCertExt(path android.Path) (*string, error) {
	s := path.String()
	if strings.HasSuffix(s, ".x509.pem") {
		return proptools.StringPtr(strings.TrimSuffix(s, ".x509.pem")), nil
	}
	return nil, fmt.Errorf("Path %q does not end with .x509.pem", s)
}
