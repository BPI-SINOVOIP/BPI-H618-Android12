
#ifndef V4L2_CAMERA_HAL_METADATA_V4L2_CONTROL_DELEGATE_H_
#define V4L2_CAMERA_HAL_METADATA_V4L2_CONTROL_DELEGATE_H_

#include "../v4l2_wrapper.h"
#include "../v4l2_stream.h"
#include "control_delegate_interface.h"
#include "converter_interface.h"

namespace v4l2_camera_hal {

// A V4L2ControlDelegate routes getting and setting through V4L2
template <typename TMetadata, typename TV4L2 = int32_t>
class V4L2ControlDelegate : public ControlDelegateInterface<TMetadata> {
 public:
  V4L2ControlDelegate(
      std::shared_ptr<V4L2Wrapper> device,
      int control_id,
      std::shared_ptr<ConverterInterface<TMetadata, TV4L2>> converter)
      : control_id_(control_id),
        converter_(std::move(converter)){

    device_ = device;
    stream_ = device_->getStream(MAIN_STREAM);
    if (device_ == nullptr) {
      HAL_LOGE("Failed to get Stream, we should connect first.");
    }

  };

  int GetValue(TMetadata* value) override {
    TV4L2 v4l2_value;
    stream_ = device_->getStream(MAIN_STREAM);

    int res = stream_->GetControl(control_id_, &v4l2_value);
    if (res) {
      HAL_LOGE("Failed to get device value for control %d.", control_id_);
      return res;
    }
    return converter_->V4L2ToMetadata(v4l2_value, value);
  };

  int SetValue(const TMetadata& value) override {
    TV4L2 v4l2_value;
    stream_ = device_->getStream(MAIN_STREAM);

    int res = converter_->MetadataToV4L2(value, &v4l2_value);
    if (res) {
      HAL_LOGE("Failed to convert metadata value to V4L2.");
      return res;
    }
    return stream_->SetControl(control_id_, v4l2_value);
  };

 private:
  std::shared_ptr<V4L2Stream> stream_;
  std::shared_ptr<V4L2Wrapper> device_;
  int control_id_;
  std::shared_ptr<ConverterInterface<TMetadata, TV4L2>> converter_;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_METADATA_V4L2_CONTROL_DELEGATE_H_
