if(NOT trout_NATIVE_VHAL_ROOT_DIR)
  set(trout_NATIVE_VHAL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/default_native_vhal)
endif()

if(EXISTS "${trout_NATIVE_VHAL_ROOT_DIR}")
  set(trout_NAVTIVE_VHAL_LIBRARIES "vhal_default_impl_lib")

  set(trout_NAVTIVE_VHAL_COMMON_INCLUDE_DIRS
      "${trout_NATIVE_VHAL_ROOT_DIR}"
      "${trout_NATIVE_VHAL_ROOT_DIR}/common/include"
      "${trout_NATIVE_VHAL_ROOT_DIR}/common/include/vhal_v2_0")
  set(trout_NAVTIVE_VHAL_IMPL_INCLUDE_DIRS "${trout_NATIVE_VHAL_ROOT_DIR}/impl")
else()
  message(FATAL_ERROR "${trout_NATIVE_VHAL_ROOT_DIR} not found")
endif()
