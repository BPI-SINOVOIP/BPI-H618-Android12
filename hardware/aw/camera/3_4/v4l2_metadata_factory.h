
#ifndef V4L2_CAMERA_HAL_V4L2_METADATA_FACTORY_H_
#define V4L2_CAMERA_HAL_V4L2_METADATA_FACTORY_H_

#include <memory>

#include "metadata/metadata.h"
#include "v4l2_wrapper.h"
#include "v4l2_stream.h"
#include "camera_config.h"

namespace v4l2_camera_hal {

// A static function to get a Metadata object populated with V4L2 or other
// controls as appropriate.
int GetV4L2Metadata(std::shared_ptr<V4L2Wrapper> device,
                    std::unique_ptr<Metadata>* result, std::shared_ptr<CCameraConfig> pCameraCfg);

}  // namespace v4l2_camera_hal


#endif  // V4L2_CAMERA_HAL_V4L2_METADATA_FACTORY_H_
