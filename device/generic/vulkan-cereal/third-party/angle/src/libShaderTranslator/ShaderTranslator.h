#pragma once

#include <inttypes.h>
#include <stddef.h>

#if defined(_WIN32)
#        define ANGLE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#        define ANGLE_EXPORT __attribute__((visibility("default")))
#else
#    define ANGLE_EXPORT
#endif

extern "C" {

typedef void* ST_Handle;
typedef uint64_t ST_CompileOptions;

enum ST_ShaderSpec {
    ST_GLES2_SPEC,
    ST_WEBGL_SPEC,

    ST_GLES3_SPEC,
    ST_WEBGL2_SPEC,

    ST_GLES3_1_SPEC,
    ST_WEBGL3_SPEC,

    ST_GL_CORE_SPEC,
    ST_GL_COMPATIBILITY_SPEC,
};

enum ST_ShaderOutput {
    // ESSL output only supported in some configurations.
    ST_ESSL_OUTPUT = 0x8B45,

    // GLSL output only supported in some configurations.
    ST_GLSL_COMPATIBILITY_OUTPUT = 0x8B46,
    // Note: GL introduced core profiles in 1.5.
    ST_GLSL_130_OUTPUT      = 0x8B47,
    ST_GLSL_140_OUTPUT      = 0x8B80,
    ST_GLSL_150_CORE_OUTPUT = 0x8B81,
    ST_GLSL_330_CORE_OUTPUT = 0x8B82,
    ST_GLSL_400_CORE_OUTPUT = 0x8B83,
    ST_GLSL_410_CORE_OUTPUT = 0x8B84,
    ST_GLSL_420_CORE_OUTPUT = 0x8B85,
    ST_GLSL_430_CORE_OUTPUT = 0x8B86,
    ST_GLSL_440_CORE_OUTPUT = 0x8B87,
    ST_GLSL_450_CORE_OUTPUT = 0x8B88,

    // Prefer using these to specify HLSL output type:
    ST_HLSL_3_0_OUTPUT       = 0x8B48,  // D3D 9
    ST_HLSL_4_1_OUTPUT       = 0x8B49,  // D3D 11
    ST_HLSL_4_0_FL9_3_OUTPUT = 0x8B4A,  // D3D 11 feature level 9_3

    // Output specialized GLSL to be fed to glslang for Vulkan SPIR.
    ST_GLSL_VULKAN_OUTPUT = 0x8B4B,

    // Output specialized GLSL to be fed to glslang for Vulkan SPIR to be cross compiled to Metal
    // later.
    ST_GLSL_METAL_OUTPUT = 0x8B4C,
};

// Compile options.
const ST_CompileOptions ST_VALIDATE               = 0;
const ST_CompileOptions ST_VALIDATE_LOOP_INDEXING = UINT64_C(1) << 0;
const ST_CompileOptions ST_INTERMEDIATE_TREE      = UINT64_C(1) << 1;
const ST_CompileOptions ST_OBJECT_CODE            = UINT64_C(1) << 2;
const ST_CompileOptions ST_VARIABLES              = UINT64_C(1) << 3;
const ST_CompileOptions ST_LINE_DIRECTIVES        = UINT64_C(1) << 4;
const ST_CompileOptions ST_SOURCE_PATH            = UINT64_C(1) << 5;

// This flag will keep invariant declaration for input in fragment shader for GLSL >=4.20 on AMD.
// From GLSL >= 4.20, it's optional to add invariant for fragment input, but GPU vendors have
// different implementations about this. Some drivers forbid invariant in fragment for GLSL>= 4.20,
// e.g. Linux Mesa, some drivers treat that as optional, e.g. NVIDIA, some drivers require invariant
// must match between vertex and fragment shader, e.g. AMD. The behavior on AMD is obviously wrong.
// Remove invariant for input in fragment shader to workaround the restriction on Intel Mesa.
// But don't remove on AMD Linux to avoid triggering the bug on AMD.
const ST_CompileOptions ST_DONT_REMOVE_INVARIANT_FOR_FRAGMENT_INPUT = UINT64_C(1) << 6;

// Due to spec difference between GLSL 4.1 or lower and ESSL3, some platforms (for example, Mac OSX
// core profile) require a variable's "invariant"/"centroid" qualifiers to match between vertex and
// fragment shader. A simple solution to allow such shaders to link is to omit the two qualifiers.
// AMD driver in Linux requires invariant qualifier to match between vertex and fragment shaders,
// while ESSL3 disallows invariant qualifier in fragment shader and GLSL >= 4.2 doesn't require
// invariant qualifier to match between shaders. Remove invariant qualifier from vertex shader to
// workaround AMD driver bug.
// Note that the two flags take effect on ESSL3 input shaders translated to GLSL 4.1 or lower and to
// GLSL 4.2 or newer on Linux AMD.
// TODO(zmo): This is not a good long-term solution. Simply dropping these qualifiers may break some
// developers' content. A more complex workaround of dynamically generating, compiling, and
// re-linking shaders that use these qualifiers should be implemented.
const ST_CompileOptions ST_REMOVE_INVARIANT_AND_CENTROID_FOR_ESSL3 = UINT64_C(1) << 7;

// This flag works around bug in Intel Mac drivers related to abs(i) where
// i is an integer.
const ST_CompileOptions ST_EMULATE_ABS_INT_FUNCTION = UINT64_C(1) << 8;

// Enforce the GLSL 1.017 Appendix A section 7 packing restrictions.
// This flag only enforces (and can only enforce) the packing
// restrictions for uniform variables in both vertex and fragment
// shaders. ShCheckVariablesWithinPackingLimits() lets embedders
// enforce the packing restrictions for varying variables during
// program link time.
const ST_CompileOptions ST_ENFORCE_PACKING_RESTRICTIONS = UINT64_C(1) << 9;

// This flag ensures all indirect (expression-based) array indexing
// is clamped to the bounds of the array. This ensures, for example,
// that you cannot read off the end of a uniform, whether an array
// vec234, or mat234 type. The ST_ArrayIndexClampingStrategy enum,
// specified in the ST_BuiltInResources when constructing the
// compiler, selects the strategy for the clamping implementation.
// TODO(http://anglebug.com/4361): fix for compute shaders.
const ST_CompileOptions ST_CLAMP_INDIRECT_ARRAY_BOUNDS = UINT64_C(1) << 10;

// This flag limits the complexity of an expression.
const ST_CompileOptions ST_LIMIT_EXPRESSION_COMPLEXITY = UINT64_C(1) << 11;

// This flag limits the depth of the call stack.
const ST_CompileOptions ST_LIMIT_CALL_STACK_DEPTH = UINT64_C(1) << 12;

// This flag initializes gl_Position to vec4(0,0,0,0) at the
// beginning of the vertex shader's main(), and has no effect in the
// fragment shader. It is intended as a workaround for drivers which
// incorrectly fail to link programs if gl_Position is not written.
const ST_CompileOptions ST_INIT_GL_POSITION = UINT64_C(1) << 13;

// This flag replaces
//   "a && b" with "a ? b : false",
//   "a || b" with "a ? true : b".
// This is to work around a MacOSX driver bug that |b| is executed
// independent of |a|'s value.
const ST_CompileOptions ST_UNFOLD_SHORT_CIRCUIT = UINT64_C(1) << 14;

// This flag initializes output variables to 0 at the beginning of main().
// It is to avoid undefined behaviors.
const ST_CompileOptions ST_INIT_OUTPUT_VARIABLES = UINT64_C(1) << 15;

// This flag scalarizes vec/ivec/bvec/mat constructor args.
// It is intended as a workaround for Linux/Mac driver bugs.
const ST_CompileOptions ST_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS = UINT64_C(1) << 16;

// This flag overwrites a struct name with a unique prefix.
// It is intended as a workaround for drivers that do not handle
// struct scopes correctly, including all Mac drivers and Linux AMD.
const ST_CompileOptions ST_REGENERATE_STRUCT_NAMES = UINT64_C(1) << 17;

// This flag makes the compiler not prune unused function early in the
// compilation process. Pruning coupled with ST_LIMIT_CALL_STACK_DEPTH
// helps avoid bad shaders causing stack overflows.
const ST_CompileOptions ST_DONT_PRUNE_UNUSED_FUNCTIONS = UINT64_C(1) << 18;

// This flag works around a bug in NVIDIA 331 series drivers related
// to pow(x, y) where y is a constant vector.
const ST_CompileOptions ST_REMOVE_POW_WITH_CONSTANT_EXPONENT = UINT64_C(1) << 19;

// This flag works around bugs in Mac drivers related to do-while by
// transforming them into an other construct.
const ST_CompileOptions ST_REWRITE_DO_WHILE_LOOPS = UINT64_C(1) << 20;

// This flag works around a bug in the HLSL compiler optimizer that folds certain
// constant pow expressions incorrectly. Only applies to the HLSL back-end. It works
// by expanding the integer pow expressions into a series of multiplies.
const ST_CompileOptions ST_EXPAND_SELECT_HLSL_INTEGER_POW_EXPRESSIONS = UINT64_C(1) << 21;

// Flatten "#pragma STDGL invariant(all)" into the declarations of
// varying variables and built-in GLSL variables. This compiler
// option is enabled automatically when needed.
const ST_CompileOptions ST_FLATTEN_PRAGMA_STDGL_INVARIANT_ALL = UINT64_C(1) << 22;

// Some drivers do not take into account the base level of the texture in the results of the
// HLSL GetDimensions builtin.  This flag instructs the compiler to manually add the base level
// offsetting.
const ST_CompileOptions ST_HLSL_GET_DIMENSIONS_IGNORES_BASE_LEVEL = UINT64_C(1) << 23;

// This flag works around an issue in translating GLSL function texelFetchOffset on
// INTEL drivers. It works by translating texelFetchOffset into texelFetch.
const ST_CompileOptions ST_REWRITE_TEXELFETCHOFFSET_TO_TEXELFETCH = UINT64_C(1) << 24;

// This flag works around condition bug of for and while loops in Intel Mac OSX drivers.
// Condition calculation is not correct. Rewrite it from "CONDITION" to "CONDITION && true".
const ST_CompileOptions ST_ADD_AND_TRUE_TO_LOOP_CONDITION = UINT64_C(1) << 25;

// This flag works around a bug in evaluating unary minus operator on integer on some INTEL
// drivers. It works by translating -(int) into ~(int) + 1.
const ST_CompileOptions ST_REWRITE_INTEGER_UNARY_MINUS_OPERATOR = UINT64_C(1) << 26;

// This flag works around a bug in evaluating isnan() on some INTEL D3D and Mac OSX drivers.
// It works by using an expression to emulate this function.
const ST_CompileOptions ST_EMULATE_ISNAN_FLOAT_FUNCTION = UINT64_C(1) << 27;

// This flag will use all uniforms of unused std140 and shared uniform blocks at the
// beginning of the vertex/fragment shader's main(). It is intended as a workaround for Mac
// drivers with shader version 4.10. In those drivers, they will treat unused
// std140 and shared uniform blocks' members as inactive. However, WebGL2.0 based on
// OpenGL ES3.0.4 requires all members of a named uniform block declared with a shared or std140
// layout qualifier to be considered active. The uniform block itself is also considered active.
const ST_CompileOptions ST_USE_UNUSED_STANDARD_SHARED_BLOCKS = UINT64_C(1) << 28;

// This flag works around a bug in unary minus operator on float numbers on Intel
// Mac OSX 10.11 drivers. It works by translating -float into 0.0 - float.
const ST_CompileOptions ST_REWRITE_FLOAT_UNARY_MINUS_OPERATOR = UINT64_C(1) << 29;

// This flag works around a bug in evaluating atan(y, x) on some NVIDIA OpenGL drivers.
// It works by using an expression to emulate this function.
const ST_CompileOptions ST_EMULATE_ATAN2_FLOAT_FUNCTION = UINT64_C(1) << 30;

// Set to initialize uninitialized local and global temporary variables. Should only be used with
// GLSL output. In HLSL output variables are initialized regardless of if this flag is set.
const ST_CompileOptions ST_INITIALIZE_UNINITIALIZED_LOCALS = UINT64_C(1) << 31;

// The flag modifies the shader in the following way:
// Every occurrence of gl_InstanceID is replaced by the global temporary variable InstanceID.
// Every occurrence of gl_ViewID_OVR is replaced by the varying variable ViewID_OVR.
// At the beginning of the body of main() in a vertex shader the following initializers are added:
// ViewID_OVR = uint(gl_InstanceID) % num_views;
// InstanceID = gl_InstanceID / num_views;
// ViewID_OVR is added as a varying variable to both the vertex and fragment shaders.
const ST_CompileOptions ST_INITIALIZE_BUILTINS_FOR_INSTANCED_MULTIVIEW = UINT64_C(1) << 32;

// With the flag enabled the GLSL/ESSL vertex shader is modified to include code for viewport
// selection in the following way:
// - Code to enable the extension ARB_shader_viewport_layer_array/NV_viewport_array2 is included.
// - Code to select the viewport index or layer is inserted at the beginning of main after
// ViewID_OVR's initialization.
// - A declaration of the uniform multiviewBaseViewLayerIndex.
// Note: The ST_INITIALIZE_BUILTINS_FOR_INSTANCED_MULTIVIEW flag also has to be enabled to have the
// temporary variable ViewID_OVR declared and initialized.
const ST_CompileOptions ST_SELECT_VIEW_IN_NV_GLSL_VERTEX_SHADER = UINT64_C(1) << 33;

// If the flag is enabled, gl_PointSize is clamped to the maximum point size specified in
// ST_BuiltInResources in vertex shaders.
const ST_CompileOptions ST_CLAMP_POINT_SIZE = UINT64_C(1) << 34;

// Turn some arithmetic operations that operate on a float vector-scalar pair into vector-vector
// operations. This is done recursively. Some scalar binary operations inside vector constructors
// are also turned into vector operations.
//
// This is targeted to work around a bug in NVIDIA OpenGL drivers that was reproducible on NVIDIA
// driver version 387.92. It works around the most common occurrences of the bug.
const ST_CompileOptions ST_REWRITE_VECTOR_SCALAR_ARITHMETIC = UINT64_C(1) << 35;

// Don't use loops to initialize uninitialized variables. Only has an effect if some kind of
// variable initialization is turned on.
const ST_CompileOptions ST_DONT_USE_LOOPS_TO_INITIALIZE_VARIABLES = UINT64_C(1) << 36;

// Don't use D3D constant register zero when allocating space for uniforms. This is targeted to work
// around a bug in NVIDIA D3D driver version 388.59 where in very specific cases the driver would
// not handle constant register zero correctly. Only has an effect on HLSL translation.
const ST_CompileOptions ST_SKIP_D3D_CONSTANT_REGISTER_ZERO = UINT64_C(1) << 37;

// Clamp gl_FragDepth to the range [0.0, 1.0] in case it is statically used.
const ST_CompileOptions ST_CLAMP_FRAG_DEPTH = UINT64_C(1) << 38;

// Rewrite expressions like "v.x = z = expression;". Works around a bug in NVIDIA OpenGL drivers
// prior to version 397.31.
const ST_CompileOptions ST_REWRITE_REPEATED_ASSIGN_TO_SWIZZLED = UINT64_C(1) << 39;

// Rewrite gl_DrawID as a uniform int
const ST_CompileOptions ST_EMULATE_GL_DRAW_ID = UINT64_C(1) << 40;

// This flag initializes shared variables to 0.
// It is to avoid ompute shaders being able to read undefined values that could be coming from
// another webpage/application.
const ST_CompileOptions ST_INIT_SHARED_VARIABLES = UINT64_C(1) << 41;

// Forces the value returned from an atomic operations to be always be resolved. This is targeted to
// workaround a bug in NVIDIA D3D driver where the return value from
// RWByteAddressBuffer.InterlockedAdd does not get resolved when used in the .yzw components of a
// RWByteAddressBuffer.Store operation. Only has an effect on HLSL translation.
// http://anglebug.com/3246
const ST_CompileOptions ST_FORCE_ATOMIC_VALUE_RESOLUTION = UINT64_C(1) << 42;

// Rewrite gl_BaseVertex and gl_BaseInstance as uniform int
const ST_CompileOptions ST_EMULATE_GL_BASE_VERTEX_BASE_INSTANCE = UINT64_C(1) << 43;

// Emulate seamful cube map sampling for OpenGL ES2.0.  Currently only applies to the Vulkan
// backend, as is done after samplers are moved out of structs.  Can likely be made to work on
// the other backends as well.
const ST_CompileOptions ST_EMULATE_SEAMFUL_CUBE_MAP_SAMPLING = UINT64_C(1) << 44;

// This flag controls how to translate WEBGL_video_texture sampling function.
const ST_CompileOptions ST_TAKE_VIDEO_TEXTURE_AS_EXTERNAL_OES = UINT64_C(1) << 45;

// If requested, validates the AST after every transformation.  Useful for debugging.
const ST_CompileOptions ST_VALIDATE_AST = UINT64_C(1) << 46;

// Use old version of RewriteStructSamplers, which doesn't produce as many
// sampler arrays in parameters. This causes a few tests to pass on Android.
const ST_CompileOptions ST_USE_OLD_REWRITE_STRUCT_SAMPLERS = UINT64_C(1) << 47;

// This flag works around a inconsistent behavior in Mac AMD driver where gl_VertexID doesn't
// include base vertex value. It replaces gl_VertexID with (gl_VertexID + angle_BaseVertex)
// when angle_BaseVertex is available.
const ST_CompileOptions ST_ADD_BASE_VERTEX_TO_VERTEX_ID = UINT64_C(1) << 48;

// This works around the dynamic lvalue indexing of swizzled vectors on various platforms.
const ST_CompileOptions ST_REMOVE_DYNAMIC_INDEXING_OF_SWIZZLED_VECTOR = UINT64_C(1) << 49;

// This flag works a driver bug that fails to allocate ShaderResourceView for StructuredBuffer
// on Windows 7 and earlier.
const ST_CompileOptions ST_DONT_TRANSLATE_UNIFORM_BLOCK_TO_STRUCTUREDBUFFER = UINT64_C(1) << 50;

// This flag indicates whether Bresenham line raster emulation code should be generated.  This
// emulation is necessary if the backend uses a differnet algorithm to draw lines.  Currently only
// implemented for the Vulkan backend.
const ST_CompileOptions ST_ADD_BRESENHAM_LINE_RASTER_EMULATION = UINT64_C(1) << 51;

// This flag allows disabling ARB_texture_rectangle on a per-compile basis. This is necessary
// for WebGL contexts becuase ARB_texture_rectangle may be necessary for the WebGL implementation
// internally but shouldn't be exposed to WebGL user code.
const ST_CompileOptions ST_DISABLE_ARB_TEXTURE_RECTANGLE = UINT64_C(1) << 52;

// This flag works around a driver bug by rewriting uses of row-major matrices
// as column-major in ESSL 3.00 and greater shaders.
const ST_CompileOptions ST_REWRITE_ROW_MAJOR_MATRICES = UINT64_C(1) << 53;

// Drop any explicit precision qualifiers from shader.
const ST_CompileOptions ST_IGNORE_PRECISION_QUALIFIERS = UINT64_C(1) << 54;

// Allow compiler to do early fragment tests as an optimization.
const ST_CompileOptions ST_EARLY_FRAGMENT_TESTS_OPTIMIZATION = UINT64_C(1) << 55;

// Defines alternate strategies for implementing array index clamping.
enum ST_ArrayIndexClampingStrategy
{
    // Use the clamp intrinsic for array index clamping.
    ST_CLAMP_WITH_CLAMP_INTRINSIC = 1,

    // Use a user-defined function for array index clamping.
    ST_CLAMP_WITH_USER_DEFINED_INT_CLAMP_FUNCTION
};

// The 64 bits hash function. The first parameter is the input string; the
// second parameter is the string length.
typedef uint64_t (*ST_HashFunction64)(const char*, size_t);

// GLenum alias
typedef unsigned int GLenum;

// Should be just like ShBuiltInResources; just keeping a private copy here
// to allow this to be more isolated.
struct ST_BuiltInResources
{
    // Constants.
    int MaxVertexAttribs;
    int MaxVertexUniformVectors;
    int MaxVaryingVectors;
    int MaxVertexTextureImageUnits;
    int MaxCombinedTextureImageUnits;
    int MaxTextureImageUnits;
    int MaxFragmentUniformVectors;
    int MaxDrawBuffers;

    // Extensions.
    // Set to 1 to enable the extension, else 0.
    int OES_standard_derivatives;
    int OES_EGL_image_external;
    int OES_EGL_image_external_essl3;
    int NV_EGL_stream_consumer_external;
    int ARB_texture_rectangle;
    int EXT_blend_func_extended;
    int EXT_draw_buffers;
    int EXT_frag_depth;
    int EXT_shader_texture_lod;
    int WEBGL_debug_shader_precision;
    int EXT_shader_framebuffer_fetch;
    int NV_shader_framebuffer_fetch;
    int NV_shader_noperspective_interpolation;
    int ARM_shader_framebuffer_fetch;
    int OVR_multiview;
    int OVR_multiview2;
    int EXT_multisampled_render_to_texture;
    int EXT_YUV_target;
    int EXT_geometry_shader;
    int EXT_gpu_shader5;
    int EXT_shader_non_constant_global_initializers;
    int OES_texture_storage_multisample_2d_array;
    int OES_texture_3D;
    int ANGLE_texture_multisample;
    int ANGLE_multi_draw;
    int ANGLE_base_vertex_base_instance;
    int WEBGL_video_texture;
    int APPLE_clip_distance;
    int OES_texture_cube_map_array;
    int EXT_texture_cube_map_array;

    // Set to 1 to enable replacing GL_EXT_draw_buffers #extension directives
    // with GL_NV_draw_buffers in ESSL output. This flag can be used to emulate
    // EXT_draw_buffers by using it in combination with GLES3.0 glDrawBuffers
    // function. This applies to Tegra K1 devices.
    int NV_draw_buffers;

    // Set to 1 if highp precision is supported in the ESSL 1.00 version of the
    // fragment language. Does not affect versions of the language where highp
    // support is mandatory.
    // Default is 0.
    int FragmentPrecisionHigh;

    // GLSL ES 3.0 constants.
    int MaxVertexOutputVectors;
    int MaxFragmentInputVectors;
    int MinProgramTexelOffset;
    int MaxProgramTexelOffset;

    // Extension constants.

    // Value of GL_MAX_DUAL_SOURCE_DRAW_BUFFERS_EXT for OpenGL ES output context.
    // Value of GL_MAX_DUAL_SOURCE_DRAW_BUFFERS for OpenGL output context.
    // GLES SL version 100 gl_MaxDualSourceDrawBuffersEXT value for EXT_blend_func_extended.
    int MaxDualSourceDrawBuffers;

    // Value of GL_MAX_VIEWS_OVR.
    int MaxViewsOVR;

    // Name Hashing.
    // Set a 64 bit hash function to enable user-defined name hashing.
    // Default is NULL.
    ST_HashFunction64 HashFunction;

    // Selects a strategy to use when implementing array index clamping.
    // Default is ST_CLAMP_WITH_CLAMP_INTRINSIC.
    ST_ArrayIndexClampingStrategy ArrayIndexClampingStrategy;

    // The maximum complexity an expression can be when ST_LIMIT_EXPRESSION_COMPLEXITY is turned on.
    int MaxExpressionComplexity;

    // The maximum depth a call stack can be.
    int MaxCallStackDepth;

    // The maximum number of parameters a function can have when ST_LIMIT_EXPRESSION_COMPLEXITY is
    // turned on.
    int MaxFunctionParameters;

    // GLES 3.1 constants

    // texture gather offset constraints.
    int MinProgramTextureGatherOffset;
    int MaxProgramTextureGatherOffset;

    // maximum number of available image units
    int MaxImageUnits;

    // maximum number of image uniforms in a vertex shader
    int MaxVertexImageUniforms;

    // maximum number of image uniforms in a fragment shader
    int MaxFragmentImageUniforms;

    // maximum number of image uniforms in a compute shader
    int MaxComputeImageUniforms;

    // maximum total number of image uniforms in a program
    int MaxCombinedImageUniforms;

    // maximum number of uniform locations
    int MaxUniformLocations;

    // maximum number of ssbos and images in a shader
    int MaxCombinedShaderOutputResources;

    // maximum number of groups in each dimension
    int MaxComputeWorkGroupCount[3];
    // maximum number of threads per work group in each dimension
    int MaxComputeWorkGroupSize[3];

    // maximum number of total uniform components
    int MaxComputeUniformComponents;

    // maximum number of texture image units in a compute shader
    int MaxComputeTextureImageUnits;

    // maximum number of atomic counters in a compute shader
    int MaxComputeAtomicCounters;

    // maximum number of atomic counter buffers in a compute shader
    int MaxComputeAtomicCounterBuffers;

    // maximum number of atomic counters in a vertex shader
    int MaxVertexAtomicCounters;

    // maximum number of atomic counters in a fragment shader
    int MaxFragmentAtomicCounters;

    // maximum number of atomic counters in a program
    int MaxCombinedAtomicCounters;

    // maximum binding for an atomic counter
    int MaxAtomicCounterBindings;

    // maximum number of atomic counter buffers in a vertex shader
    int MaxVertexAtomicCounterBuffers;

    // maximum number of atomic counter buffers in a fragment shader
    int MaxFragmentAtomicCounterBuffers;

    // maximum number of atomic counter buffers in a program
    int MaxCombinedAtomicCounterBuffers;

    // maximum number of buffer object storage in machine units
    int MaxAtomicCounterBufferSize;

    // maximum number of uniform block bindings
    int MaxUniformBufferBindings;

    // maximum number of shader storage buffer bindings
    int MaxShaderStorageBufferBindings;

    // maximum point size (higher limit from ALIASED_POINT_SIZE_RANGE)
    float MaxPointSize;

    // EXT_geometry_shader constants
    int MaxGeometryUniformComponents;
    int MaxGeometryUniformBlocks;
    int MaxGeometryInputComponents;
    int MaxGeometryOutputComponents;
    int MaxGeometryOutputVertices;
    int MaxGeometryTotalOutputComponents;
    int MaxGeometryTextureImageUnits;
    int MaxGeometryAtomicCounterBuffers;
    int MaxGeometryAtomicCounters;
    int MaxGeometryShaderStorageBlocks;
    int MaxGeometryShaderInvocations;
    int MaxGeometryImageUniforms;

    // Subpixel bits used in rasterization.
    int SubPixelBits;

    // APPLE_clip_distance/EXT_clip_cull_distance constant
    int MaxClipDistances;
};

// Varying interpolation qualifier, see section 4.3.9 of the ESSL 3.00.4 spec
enum ST_InterpolationType {
    ST_INTERPOLATION_SMOOTH,
    ST_INTERPOLATION_CENTROID,
    ST_INTERPOLATION_FLAT,
    ST_INTERPOLATION_NOPERSPECTIVE
};

// Uniform block layout qualifier, see section 4.3.8.3 of the ESSL 3.00.4 spec
enum ST_BlockLayoutType
{
    ST_BLOCKLAYOUT_STANDARD,
    ST_BLOCKLAYOUT_STD140 = ST_BLOCKLAYOUT_STANDARD,
    ST_BLOCKLAYOUT_STD430,  // Shader storage block layout qualifier
    ST_BLOCKLAYOUT_PACKED,
    ST_BLOCKLAYOUT_SHARED
};

// Interface Blocks, see section 4.3.9 of the ESSL 3.10 spec
enum class ST_BlockType
{
    ST_BLOCK_UNIFORM,
    ST_BLOCK_BUFFER,

    // Required in OpenGL ES 3.1 extension GL_OES_shader_io_blocks.
    // TODO(jiawei.shao@intel.com): add BLOCK_OUT.
    // Also used in GLSL
    ST_BLOCK_IN
};

// Base class for all variables defined in shaders, including Varyings, Uniforms, etc
// Note: we must override the copy constructor and assignment operator so we can
// work around excessive GCC binary bloating:
// See https://code.google.com/p/angleproject/issues/detail?id=697
struct ST_ShaderVariable {
    GLenum type;
    GLenum precision;
    const char* name;
    const char* mappedName;

    unsigned int arraySizeCount;
    unsigned int* pArraySizes;

    unsigned int staticUse;
    unsigned int active;

    unsigned int fieldsCount;
    ST_ShaderVariable* pFields;

    const char* structName;

    // Only applies to interface block fields. Kept here for simplicity.
    unsigned int isRowMajorLayout;

    // VariableWithLocation
    int location;

    // Uniform
    int binding;

    GLenum imageUnitFormat;
    int offset;
    unsigned int readonly;
    unsigned int writeonly;

    // OutputVariable
    // From EXT_blend_func_extended.
    int index;

    // Varying
    ST_InterpolationType interpolation;
    unsigned int isInvariant;

    int flattenedOffsetInParentArrays;
};

struct ST_InterfaceBlock {
    const char* name;
    const char* mappedName;
    const char* instanceName;
    unsigned int arraySize;
    ST_BlockLayoutType layout;

    // Deprecated. Matrix packing should only be queried from individual fields of the block.
    // TODO(oetuaho): Remove this once it is no longer used in Chromium.
    unsigned int isRowMajorLayout;

    int binding;
    unsigned int staticUse;
    unsigned int active;
    ST_BlockType blockType;

    unsigned int fieldsCount;
    ST_ShaderVariable* pFields;
};

struct ST_WorkGroupSize {
    int localSizeQualifiers[3];
};

struct ST_NameHashingMap {
    unsigned int entryCount;
    const char* const* ppUserNames;
    const char* const* ppCompiledNames;
};

struct ST_ShaderCompileInfo {
    ST_Handle inputHandle;
    GLenum type;
    ST_ShaderSpec spec;
    ST_ShaderOutput output;
    ST_CompileOptions compileOptions;
    const ST_BuiltInResources *pResources;
    const char* pShaderString;
};

// A one-stop shop for getting all shader info in one call
// Everything is written and allocated by the library, transferring ownership
// to the consumer. The consumer will need to call STFreeShaderResolveState()
// to free the memory.
struct ST_ShaderCompileResult {
    ST_Handle outputHandle;
    GLenum type;
    int version;

    const char* originalSource;
    const char* translatedSource;
    const char* infoLog;

    ST_NameHashingMap* nameHashingMap;

    ST_WorkGroupSize workGroupSize;
    unsigned int sharedMemSize;

    unsigned int inputVaryingsCount;
    const ST_ShaderVariable* pInputVaryings;

    unsigned int outputVaryingsCount;
    const ST_ShaderVariable* pOutputVaryings;

    unsigned int uniformsCount;
    const ST_ShaderVariable* pUniforms;

    unsigned int uniformBlocksCount;
    const ST_InterfaceBlock* pUniformBlocks;

    unsigned int shaderStorageBlocksCount;
    const ST_InterfaceBlock* pShaderStorageBlocks;

    unsigned int allAttributesCount;
    const ST_ShaderVariable* pAllAttributes;

    unsigned int activeAttributesCount;
    const ST_ShaderVariable* pActiveAttributes;

    unsigned int activeOutputVariablesCount;
    const ST_ShaderVariable* pActiveOutputVariables;

    bool earlyFragmentTestsOptimization;
    int numViews;

    // Geometry shader
    unsigned int geometryShaderInputPrimitiveType;
    unsigned int geometryShaderOutputPrimitiveType;
    unsigned int geometryShaderMaxVertices;
    int geometryShaderMaxInvocations;
    int geometryShaderInvocations;

    bool compileStatus;
};

ANGLE_EXPORT void STInitialize(void);
ANGLE_EXPORT void STFinalize(void);

ANGLE_EXPORT ST_BuiltInResources*
STGenerateResources(ST_BuiltInResources *pResources);

ANGLE_EXPORT void STCompileAndResolve(
    const ST_ShaderCompileInfo* pInfo,
    ST_ShaderCompileResult** ppResult);

ANGLE_EXPORT void STFreeShaderResolveState(
    ST_ShaderCompileResult* state);

ANGLE_EXPORT ST_ShaderVariable STCopyVariable(const ST_ShaderVariable* pInput);
ANGLE_EXPORT ST_InterfaceBlock STCopyInterfaceBlock(const ST_InterfaceBlock* pInput);
ANGLE_EXPORT void STDestroyVariable(ST_ShaderVariable* pInput);
ANGLE_EXPORT void STDestroyInterfaceBlock(ST_InterfaceBlock* pInput);

typedef void (*STInitialize_t)(void);
typedef void (*STFinalize_t)(void);

typedef ST_BuiltInResources*
(*STGenerateResources_t)(ST_BuiltInResources *pResources);

typedef void (*STCompileAndResolve_t)(
    const ST_ShaderCompileInfo* pInfo,
    ST_ShaderCompileResult** ppResult);

typedef void (*STFreeShaderResolveState_t)(
    ST_ShaderCompileResult* state);

typedef ST_ShaderVariable (*STCopyVariable_t)(const ST_ShaderVariable* pInput);
typedef ST_InterfaceBlock (*STCopyInterfaceBlock_t)(const ST_InterfaceBlock* pInput);
typedef void (*STDestroyVariable_t)(ST_ShaderVariable* pInput);
typedef void (*STDestroyInterfaceBlock_t)(ST_InterfaceBlock* pInput);

struct STDispatch {
    STInitialize_t initialize;
    STFinalize_t finalize;
    STGenerateResources_t generateResources;
    STCompileAndResolve_t compileAndResolve;
    STFreeShaderResolveState_t freeShaderResolveState;
    STCopyVariable_t copyVariable;
    STCopyInterfaceBlock_t copyInterfaceBlock;
    STDestroyVariable_t destroyVariable;
    STDestroyInterfaceBlock_t destroyInterfaceBlock;
};

} // extern "C"
