#include "src/libShaderTranslator/ShaderTranslator.h"

ANGLE_EXPORT void STInitialize(void) { }
ANGLE_EXPORT void STFinalize(void) { }

ANGLE_EXPORT ST_BuiltInResources*
STGenerateResources(ST_BuiltInResources *pResources) { return nullptr; }

ANGLE_EXPORT void STCompileAndResolve(
    const ST_ShaderCompileInfo* pInfo,
    ST_ShaderCompileResult** ppResult) { }

ANGLE_EXPORT void STFreeShaderResolveState(
    ST_ShaderCompileResult* state) { }

ANGLE_EXPORT ST_ShaderVariable STCopyVariable(const ST_ShaderVariable* pInput) {
    ST_ShaderVariable v; return v;
}

ANGLE_EXPORT ST_InterfaceBlock STCopyInterfaceBlock(const ST_InterfaceBlock* pInput) {
    ST_InterfaceBlock i; return i;
}

ANGLE_EXPORT void STDestroyVariable(ST_ShaderVariable* pInput) { }
ANGLE_EXPORT void STDestroyInterfaceBlock(ST_InterfaceBlock* pInput) { }
