
#ifndef V4L2_CAMERA_HAL_METADATA_ENUM_CONVERTER_H_
#define V4L2_CAMERA_HAL_METADATA_ENUM_CONVERTER_H_

#include <map>

#include "../common.h"
#include "converter_interface.h"

namespace v4l2_camera_hal {

// An EnumConverter converts between enum values.
class EnumConverter : public ConverterInterface<uint8_t, int32_t> {
 public:
  EnumConverter(const std::multimap<int32_t, uint8_t>& v4l2_to_metadata);

  virtual int MetadataToV4L2(uint8_t value, int32_t* conversion) override;
  virtual int V4L2ToMetadata(int32_t value, uint8_t* conversion) override;

 private:
  const std::multimap<int32_t, uint8_t> v4l2_to_metadata_;

  DISALLOW_COPY_AND_ASSIGN(EnumConverter);
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_ENUM_CONVERTER_H_
