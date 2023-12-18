
// Mock for converter interfaces.

#ifndef V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_MOCK_H_
#define V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_MOCK_H_

#include <gmock/gmock.h>

#include "converter_interface.h"

namespace v4l2_camera_hal {

template <typename TMetadata, typename TV4L2>
class ConverterInterfaceMock : public ConverterInterface<TMetadata, TV4L2> {
 public:
  ConverterInterfaceMock(){};
  MOCK_METHOD2_T(MetadataToV4L2, int(TMetadata, TV4L2*));
  MOCK_METHOD2_T(V4L2ToMetadata, int(TV4L2, TMetadata*));
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_MOCK_H_
