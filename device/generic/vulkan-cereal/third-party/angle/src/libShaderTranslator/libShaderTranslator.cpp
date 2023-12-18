#include "GLSLANG/ShaderLang.h"

#include "angle_gl.h"
#include "ShaderTranslator.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <map>
#include <string>

namespace {

unsigned int boolToUint(bool b) {
    return b ? 1 : 0;
}

template<class S, class T>
static T* toAllocedCArray(const S& xs) {
    size_t bytes = sizeof(T) * xs.size();
    T* res = (T *)malloc(bytes);
    memset(res, 0, bytes);
    memcpy(res, xs.data(), bytes);
    return res;
}

static const char* toAllocedCString(const std::string& str) {
    size_t bytes = sizeof(char) * str.size() + 1;
    char* res = (char *)malloc(bytes);
    memset(res, 0, bytes);
    memcpy(res, str.data(), bytes);
    return res;
}

template <class T>
T* dupCArray(unsigned int count, const T* pItems) {
    if (0 == count) return nullptr;

    size_t bytes = count * sizeof(T);
    T* res = (T*)malloc(count * bytes);
    memcpy(res, pItems, bytes);
    return res;
}

static void fillShaderVariable(const sh::ShaderVariable& var, ST_ShaderVariable* out);
static void fillInterfaceBlock(const sh::InterfaceBlock& ib, ST_InterfaceBlock* out);

static ST_NameHashingMap* createNameHashingMap(const std::map<std::string, std::string>& inputMap);
static ST_ShaderVariable* createShaderVariableArray(const std::vector<sh::ShaderVariable>& inputVars);
static ST_InterfaceBlock* createInterfaceBlockArray(const std::vector<sh::InterfaceBlock>& inputBlocks);

static void initShaderVariableInfo(
    const std::vector<sh::ShaderVariable>* pInputs,
    unsigned int* pCount,
    ST_ShaderVariable** pOutputs);

static void initInterfaceBlockInfo(
    const std::vector<sh::InterfaceBlock>* pInputs,
    unsigned int* pCount,
    ST_InterfaceBlock** pOutputs);

static std::vector<sh::ShaderVariable> getActiveVariables(
    const std::vector<sh::ShaderVariable>* pInputs);

static void freeNameHashingMap(ST_NameHashingMap* nameHashingMap);
static void destroyShaderVariable(ST_ShaderVariable* var);
static void freeShaderVariableArray(uint32_t count, ST_ShaderVariable* vars);
static void destroyInterfaceBlock(ST_InterfaceBlock* ib);
static void freeInterfaceBlockArray(uint32_t count, ST_InterfaceBlock* ibs);

static void fillShaderVariable(const sh::ShaderVariable& var, ST_ShaderVariable* out) {
    out->type = var.type;
    out->precision = var.precision;

    out->name = toAllocedCString(var.name);
    out->mappedName = toAllocedCString(var.mappedName);

    out->arraySizeCount = (uint32_t)var.arraySizes.size();
    out->pArraySizes = toAllocedCArray<std::vector<unsigned int>, unsigned int>(var.arraySizes);

    out->staticUse = boolToUint(var.staticUse);
    out->active = boolToUint(var.active);

    out->fieldsCount = (uint32_t)var.fields.size();
    out->pFields = toAllocedCArray<std::vector<sh::ShaderVariable>, ST_ShaderVariable>(var.fields);
    for (uint32_t i = 0; i < out->fieldsCount; ++i) {
        fillShaderVariable(var.fields[i], out->pFields + i);
    }

    out->structName = toAllocedCString(var.structName);

    out->isRowMajorLayout = boolToUint(var.isRowMajorLayout);

    out->location = var.location;

    out->binding = var.binding;

    out->imageUnitFormat = var.imageUnitFormat;
    out->offset = var.offset;
    out->readonly = boolToUint(var.readonly);
    out->writeonly = boolToUint(var.writeonly);

    out->index = var.index;

    out->interpolation = (ST_InterpolationType)(var.interpolation);
    out->isInvariant = boolToUint(var.isInvariant);

    out->flattenedOffsetInParentArrays = var.parentArrayIndex();
}

static void fillInterfaceBlock(const sh::InterfaceBlock& ib, ST_InterfaceBlock* out) {
    out->name = toAllocedCString(ib.name);
    out->mappedName = toAllocedCString(ib.mappedName);
    out->instanceName = toAllocedCString(ib.mappedName);

    out->arraySize = ib.arraySize;
    out->layout = (ST_BlockLayoutType)ib.layout;

    out->isRowMajorLayout = boolToUint(ib.isRowMajorLayout);

    out->binding = ib.binding;
    out->staticUse = boolToUint(ib.staticUse);
    out->active = boolToUint(ib.active);
    out->blockType = (ST_BlockType)ib.blockType;

    out->fieldsCount = (uint32_t)ib.fields.size();
    out->pFields = toAllocedCArray<std::vector<sh::ShaderVariable>, ST_ShaderVariable>(ib.fields);
    for (uint32_t i = 0; i < out->fieldsCount; ++i) {
        fillShaderVariable(ib.fields[i], out->pFields + i);
    }
}

static ST_NameHashingMap* createNameHashingMap(const std::map<std::string, std::string>& inputMap) {
    size_t elements = inputMap.size();

    const char** ppUserNames = (const char**)malloc(elements * sizeof(const char*));
    const char** ppCompiledNames = (const char**)malloc(elements * sizeof(const char* const*));
    size_t i = 0;

    for (auto it : inputMap) {
        ppUserNames[i] = toAllocedCString(it.first);
        ppCompiledNames[i] = toAllocedCString(it.second);
        ++i;
    }

    ST_NameHashingMap* res = (ST_NameHashingMap*)(malloc(sizeof(ST_NameHashingMap)));

    res->entryCount = (uint32_t)elements;
    res->ppUserNames = ppUserNames;
    res->ppCompiledNames = ppCompiledNames;

    return res;
}


static ST_ShaderVariable* createShaderVariableArray(const std::vector<sh::ShaderVariable>& inputVars) {
    ST_ShaderVariable* res =
        toAllocedCArray<std::vector<sh::ShaderVariable>, ST_ShaderVariable>(inputVars);
    for (size_t i = 0; i < inputVars.size(); ++i) {
        fillShaderVariable(inputVars[i], res + i);
    }
    return res;
}

static ST_InterfaceBlock* createInterfaceBlockArray(const std::vector<sh::InterfaceBlock>& inputBlocks) {
    ST_InterfaceBlock* res =
        toAllocedCArray<std::vector<sh::InterfaceBlock>, ST_InterfaceBlock>(inputBlocks);
    for (size_t i = 0; i < inputBlocks.size(); ++i) {
        fillInterfaceBlock(inputBlocks[i], res + i);
    }
    return res;
}

static void initShaderVariableInfo(
    const std::vector<sh::ShaderVariable>* pInputs,
    unsigned int* pCount,
    ST_ShaderVariable** pOutputs) {
    *pCount = pInputs->size();
    *pOutputs = createShaderVariableArray(*pInputs);
}

static void initInterfaceBlockInfo(
    const std::vector<sh::InterfaceBlock>* pInputs,
    unsigned int* pCount,
    ST_InterfaceBlock** pOutputs) {
    *pCount = pInputs->size();
    *pOutputs = createInterfaceBlockArray(*pInputs);
}

static std::vector<sh::ShaderVariable> getActiveVariables(
    const std::vector<sh::ShaderVariable>* pInputs) {
    std::vector<sh::ShaderVariable> activeVars;
    for (const auto& var: *pInputs) {
        if (var.active) {
            activeVars.push_back(var);
        }
    }
    return activeVars;
}

static void freeShaderVariableArray(uint32_t count, ST_ShaderVariable* vars) {
    for (uint32_t i = 0; i < count; ++i) {
        destroyShaderVariable(vars + i);
    }
    free(vars);
}

static void freeInterfaceBlockArray(uint32_t count, ST_InterfaceBlock* ibs) {
    for (uint32_t i = 0; i < count; ++i) {
        destroyInterfaceBlock(ibs + i);
    }
    free(ibs);
}

static void freeNameHashingMap(ST_NameHashingMap* nameHashingMap) {
    for (uint32_t i = 0; i < nameHashingMap->entryCount; ++i) {
        free((void*)nameHashingMap->ppUserNames[i]);
        free((void*)nameHashingMap->ppCompiledNames[i]);
    }

    free((void*)nameHashingMap->ppUserNames);
    free((void*)nameHashingMap->ppCompiledNames);

    free((void*)nameHashingMap);
}

static void destroyShaderVariable(ST_ShaderVariable* var) {
    free((void*)var->name);
    free((void*)var->mappedName);

    free((void*)var->pArraySizes);
    freeShaderVariableArray(var->fieldsCount, var->pFields);

    free((void*)var->structName);
}

static void destroyInterfaceBlock(ST_InterfaceBlock* ib) {
    free((void*)ib->name);
    free((void*)ib->mappedName);
    free((void*)ib->instanceName);

    freeShaderVariableArray(ib->fieldsCount, ib->pFields);
}

} // namespace

extern "C" {

ANGLE_EXPORT void STInitialize() {
    sh::Initialize();
}

ANGLE_EXPORT void STFinalize() {
    sh::Finalize();
}

ANGLE_EXPORT ST_BuiltInResources*
STGenerateResources(ST_BuiltInResources *pResources) {
    static_assert(sizeof(ST_BuiltInResources) == sizeof(ShBuiltInResources),
        "mismatch in ST_BuiltInResources versus ShBuiltInResources");
    sh::InitBuiltInResources(reinterpret_cast<ShBuiltInResources*>(pResources));
    return pResources;
}

ANGLE_EXPORT void STCompileAndResolve(
    const ST_ShaderCompileInfo* pInfo,
    ST_ShaderCompileResult** ppResult) {

    ShHandle handle = reinterpret_cast<ShHandle>(pInfo->inputHandle);

    GLenum type = pInfo->type;
    ST_ShaderSpec spec = pInfo->spec; 
    ST_ShaderOutput output = pInfo->output;
    const ST_BuiltInResources* resources = pInfo->pResources;
    const char* shaderString = pInfo->pShaderString;
    ST_CompileOptions compileOptions = pInfo->compileOptions;

    if (!handle) {
        handle =
            sh::ConstructCompiler(
                    type,
                    static_cast<ShShaderSpec>(spec),
                    static_cast<ShShaderOutput>(output),
                    reinterpret_cast<const ShBuiltInResources*>(resources));
    }

    (void)createNameHashingMap;
    (void)createShaderVariableArray;
    (void)createInterfaceBlockArray;

    bool compileResult =
        sh::Compile(
            static_cast<ShHandle>(handle),
            &shaderString, 1,
            static_cast<ST_CompileOptions>(compileOptions));

    ST_ShaderCompileResult* res =
        (ST_ShaderCompileResult*)(malloc(sizeof(ST_ShaderCompileResult)));
    memset(res, 0, sizeof(ST_ShaderCompileResult));

    res->outputHandle = handle;

    res->originalSource = strdup(shaderString);
    res->translatedSource = toAllocedCString(sh::GetObjectCode(handle));
    res->infoLog = toAllocedCString(sh::GetInfoLog(handle));

    res->version = sh::GetShaderVersion(handle);

    res->nameHashingMap = createNameHashingMap(*(sh::GetNameHashingMap(handle)));

    initShaderVariableInfo(
        sh::GetUniforms(handle), &res->uniformsCount,
        (ST_ShaderVariable**)&res->pUniforms);
    initInterfaceBlockInfo(
        sh::GetUniformBlocks(handle), &res->uniformBlocksCount,
        (ST_InterfaceBlock**)&res->pUniformBlocks);
    initInterfaceBlockInfo(
        sh::GetShaderStorageBlocks(handle), &res->shaderStorageBlocksCount,
        (ST_InterfaceBlock**)&res->pShaderStorageBlocks);

    const GLenum kShaderTypeVertex = 0x8b31; 
    const GLenum kShaderTypeFragment = 0x8b30; 
    const GLenum kShaderTypeCompute = 0x91b9; 
    const GLenum kShaderTypeGeometry = 0x8dd9;
    const GLenum kShaderTypeTessControl = 0x8e88;
    const GLenum kShaderTypeTessEval = 0x8e87;

    switch (type) {
        case kShaderTypeVertex: {

            initShaderVariableInfo(
                sh::GetOutputVaryings(handle), &res->outputVaryingsCount,
                (ST_ShaderVariable**)&res->pOutputVaryings);
            initShaderVariableInfo(
                sh::GetAttributes(handle), &res->allAttributesCount,
                (ST_ShaderVariable**)&res->pAllAttributes);

            std::vector<sh::ShaderVariable> activeAttribs =
                getActiveVariables(sh::GetAttributes(handle));
            initShaderVariableInfo(
                &activeAttribs, &res->activeAttributesCount,
                (ST_ShaderVariable**)&res->pActiveAttributes);

            res->numViews = sh::GetVertexShaderNumViews(handle);
            break;
        }
        case kShaderTypeFragment: {
            initShaderVariableInfo(
                sh::GetAttributes(handle), &res->allAttributesCount,
                (ST_ShaderVariable**)&res->pAllAttributes);

            std::vector<sh::ShaderVariable> activeAttribs =
                getActiveVariables(sh::GetAttributes(handle));
            initShaderVariableInfo(
                &activeAttribs, &res->activeAttributesCount,
                (ST_ShaderVariable**)&res->pActiveAttributes);

            // TODO(jmadill): These aren't sorted. Hope it works out?
            initShaderVariableInfo(
                sh::GetInputVaryings(handle),
                &res->inputVaryingsCount,
                (ST_ShaderVariable**)&res->pInputVaryings);

            std::vector<sh::ShaderVariable> activeOutputVariables =
                getActiveVariables(sh::GetOutputVariables(handle));
            initShaderVariableInfo(
                &activeOutputVariables, &res->activeOutputVariablesCount,
                (ST_ShaderVariable**)&res->pActiveOutputVariables);

            res->earlyFragmentTestsOptimization =
                boolToUint(sh::HasEarlyFragmentTestsOptimization(handle));
            break;
        }
        case kShaderTypeCompute: {
            initShaderVariableInfo(
                sh::GetAttributes(handle), &res->allAttributesCount,
                (ST_ShaderVariable**)&res->pAllAttributes);
            std::vector<sh::ShaderVariable> activeAttribs =
                getActiveVariables(sh::GetAttributes(handle));
            initShaderVariableInfo(
                &activeAttribs, &res->activeAttributesCount,
                (ST_ShaderVariable**)&res->pActiveAttributes);

            sh::WorkGroupSize size = sh::GetComputeShaderLocalGroupSize(handle);
            res->workGroupSize.localSizeQualifiers[0] = size[0];
            res->workGroupSize.localSizeQualifiers[1] = size[1];
            res->workGroupSize.localSizeQualifiers[2] = size[2];

            res->sharedMemSize = sh::GetShaderSharedMemorySize(handle);
            break;
        }
        case kShaderTypeGeometry: {
            initShaderVariableInfo(
                sh::GetInputVaryings(handle),
                &res->inputVaryingsCount,
                (ST_ShaderVariable**)&res->pInputVaryings);
            initShaderVariableInfo(
                sh::GetOutputVaryings(handle),
                &res->outputVaryingsCount,
                (ST_ShaderVariable**)&res->pOutputVaryings);

            if (sh::HasValidGeometryShaderInputPrimitiveType(handle)) {
                res->geometryShaderInputPrimitiveType =
                    sh::GetGeometryShaderInputPrimitiveType(handle);
            }

            if (sh::HasValidGeometryShaderOutputPrimitiveType(handle)) {
                res->geometryShaderOutputPrimitiveType =
                    sh::GetGeometryShaderOutputPrimitiveType(handle);
            }

            if (sh::HasValidGeometryShaderMaxVertices(handle)) {
                res->geometryShaderMaxVertices =
                    sh::GetGeometryShaderMaxVertices(handle);
            }

            res->geometryShaderInvocations =
                sh::GetGeometryShaderInvocations(handle);
            break;
        }
        case kShaderTypeTessControl: { // TODO
            break;
        }
        case kShaderTypeTessEval: { // TODO
            break;
        }
        default:
            fprintf(stderr, "%s: unknown shader type: 0x%x\n", __func__, type);
            abort();
            break;
    }

    res->compileStatus = boolToUint(compileResult);
    *ppResult = res;

    sh::ClearResults(handle);
}

ANGLE_EXPORT void STFreeShaderResolveState(
    ST_ShaderCompileResult* state) {

    free((void*)state->originalSource);
    free((void*)state->translatedSource);
    free((void*)state->infoLog);

    freeNameHashingMap(state->nameHashingMap);

    freeShaderVariableArray(state->inputVaryingsCount, (ST_ShaderVariable*)state->pInputVaryings);
    freeShaderVariableArray(state->outputVaryingsCount, (ST_ShaderVariable*)state->pOutputVaryings);
    freeShaderVariableArray(state->uniformsCount, (ST_ShaderVariable*)state->pUniforms);

    freeInterfaceBlockArray(state->uniformBlocksCount, (ST_InterfaceBlock*)state->pUniformBlocks);
    freeInterfaceBlockArray(state->shaderStorageBlocksCount, (ST_InterfaceBlock*)state->pShaderStorageBlocks);

    freeShaderVariableArray(state->activeAttributesCount, (ST_ShaderVariable*)state->pActiveAttributes);
    freeShaderVariableArray(state->allAttributesCount, (ST_ShaderVariable*)state->pAllAttributes);

    freeShaderVariableArray(state->activeOutputVariablesCount, (ST_ShaderVariable*)state->pActiveOutputVariables);

    free((void*)state);
}

ANGLE_EXPORT ST_ShaderVariable STCopyVariable(const ST_ShaderVariable* pInput) {
    ST_ShaderVariable res = *pInput;

    if (pInput->name) res.name = strdup(pInput->name);
    if (pInput->mappedName) res.mappedName = strdup(pInput->mappedName);

    res.pArraySizes = dupCArray(pInput->arraySizeCount, pInput->pArraySizes);
    res.pFields = dupCArray(pInput->fieldsCount, pInput->pFields);

    for (unsigned int i = 0; i < res.fieldsCount; ++i) {
        res.pFields[i] = STCopyVariable(pInput->pFields + i);
    }

    if (pInput->structName) res.structName = strdup(pInput->structName);

    return res;
}

ANGLE_EXPORT ST_InterfaceBlock STCopyInterfaceBlock(const ST_InterfaceBlock* pInput) {
    ST_InterfaceBlock res = *pInput;

    if (pInput->name) res.name = strdup(pInput->name);
    if (pInput->mappedName) res.mappedName = strdup(pInput->mappedName);
    if (pInput->instanceName) res.instanceName = strdup(pInput->instanceName);

    res.pFields = dupCArray(pInput->fieldsCount, pInput->pFields);

    for (unsigned int i = 0; i < res.fieldsCount; ++i) {
        res.pFields[i] = STCopyVariable(pInput->pFields + i);
    }

    return res;
}

ANGLE_EXPORT void STDestroyVariable(ST_ShaderVariable* pInput) {
    destroyShaderVariable(pInput);
}

ANGLE_EXPORT void STDestroyInterfaceBlock(ST_InterfaceBlock* pInput) {
    destroyInterfaceBlock(pInput);
}

} // extern "C"
