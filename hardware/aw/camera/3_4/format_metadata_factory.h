
#ifndef V4L2_CAMERA_HAL_FORMAT_METADATA_FACTORY_H_
#define V4L2_CAMERA_HAL_FORMAT_METADATA_FACTORY_H_

#include <iterator>
#include <memory>
#include <set>

#include "common.h"
#include "metadata/metadata_common.h"
#include "v4l2_wrapper.h"
#include "v4l2_stream.h"

namespace v4l2_camera_hal {

// A factory method to construct all the format-related
// partial metadata for a V4L2 device.
int AddFormatComponents(
    std::shared_ptr<V4L2Stream> device,
    std::insert_iterator<PartialMetadataSet> insertion_point,
    std::shared_ptr<CCameraConfig> pCameraCfg);

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_FORMAT_METADATA_FACTORY_H_
