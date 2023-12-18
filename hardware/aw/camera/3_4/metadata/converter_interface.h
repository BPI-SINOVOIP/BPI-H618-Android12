
#ifndef V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_H_
#define V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_H_

#include "../common.h"

namespace v4l2_camera_hal {

// A ConverterInterface converts metadata values to V4L2 values vice-versa.
template <typename TMetadata, typename TV4L2>
class ConverterInterface {
 public:
  virtual ~ConverterInterface(){};

  // Convert.
  virtual int MetadataToV4L2(TMetadata value, TV4L2* conversion) = 0;
  virtual int V4L2ToMetadata(TV4L2 value, TMetadata* conversion) = 0;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_CONVERTER_INTERFACE_H_
