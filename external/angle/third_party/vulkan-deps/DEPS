# This file is used to manage Vulkan dependencies for several repos. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

vars = {
  'chromium_git': 'https://chromium.googlesource.com',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '9158061398a96033c990e69156bd28c67114544b',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': 'e51630595fadfa99ba7f41229176667214d1a83d',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '1d4e3a7e3a04ba205ed8cb1485f7cb7369bec609',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': 'f084bcfe2bf94d8d9867e7fd4baed78381539ea1',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': '0193e158bc9f4d17e3c3a61c9311a0439ed5572d',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': '99c0b1433a0965ba7cd0dbef0e19fce649ddc12e',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '33a87ce6daecf60742277408ca9fa7ec6a4a5aae',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'd444097efd55c176eced51777cf22bd0a66dd114',
}

deps = {
  'glslang/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/glslang@{glslang_revision}',
  },

  'spirv-cross/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Cross@{spirv_cross_revision}',
  },

  'spirv-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Headers@{spirv_headers_revision}',
  },

  'spirv-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Tools@{spirv_tools_revision}',
  },

  'vulkan-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Headers@{vulkan_headers_revision}',
  },

  'vulkan-loader/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Loader@{vulkan_loader_revision}',
  },

  'vulkan-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Tools@{vulkan_tools_revision}',
  },

  'vulkan-validation-layers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@{vulkan_validation_revision}',
  },
}
