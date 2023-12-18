
#include "v4l2_control_delegate.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../v4l2_wrapper_mock.h"
#include "converter_interface_mock.h"

using testing::Return;
using testing::SetArgPointee;
using testing::Test;
using testing::_;

namespace v4l2_camera_hal {

class V4L2ControlDelegateTest : public Test {
 protected:
  virtual void SetUp() {
    mock_device_.reset(new V4L2WrapperMock());
    mock_converter_.reset(new ConverterInterfaceMock<uint8_t, int32_t>());
    dut_.reset(new V4L2ControlDelegate<uint8_t>(
        mock_device_, control_id_, mock_converter_));
  }

  std::unique_ptr<V4L2ControlDelegate<uint8_t>> dut_;
  std::shared_ptr<V4L2WrapperMock> mock_device_;
  std::shared_ptr<ConverterInterfaceMock<uint8_t, int32_t>> mock_converter_;
  const int control_id_ = 123;
};

TEST_F(V4L2ControlDelegateTest, GetSuccess) {
  int32_t device_result = 99;
  uint8_t conversion_result = 10;
  EXPECT_CALL(*mock_device_, GetControl(control_id_, _))
      .WillOnce(DoAll(SetArgPointee<1>(device_result), Return(0)));
  EXPECT_CALL(*mock_converter_, V4L2ToMetadata(device_result, _))
      .WillOnce(DoAll(SetArgPointee<1>(conversion_result), Return(0)));

  uint8_t actual = conversion_result + 1;  // Something incorrect.
  ASSERT_EQ(dut_->GetValue(&actual), 0);
  EXPECT_EQ(actual, conversion_result);
}

TEST_F(V4L2ControlDelegateTest, GetConverterFailure) {
  int32_t device_result = 99;
  EXPECT_CALL(*mock_device_, GetControl(control_id_, _))
      .WillOnce(DoAll(SetArgPointee<1>(device_result), Return(0)));
  int err = -99;
  EXPECT_CALL(*mock_converter_, V4L2ToMetadata(device_result, _))
      .WillOnce(Return(err));

  uint8_t unused = 1;
  ASSERT_EQ(dut_->GetValue(&unused), err);
}

TEST_F(V4L2ControlDelegateTest, GetDeviceFailure) {
  int err = -99;
  EXPECT_CALL(*mock_device_, GetControl(control_id_, _)).WillOnce(Return(err));

  uint8_t unused = 1;
  ASSERT_EQ(dut_->GetValue(&unused), err);
}

TEST_F(V4L2ControlDelegateTest, SetSuccess) {
  uint8_t input = 10;
  int32_t conversion_result = 99;
  EXPECT_CALL(*mock_converter_, MetadataToV4L2(input, _))
      .WillOnce(DoAll(SetArgPointee<1>(conversion_result), Return(0)));
  EXPECT_CALL(*mock_device_, SetControl(control_id_, conversion_result, _))
      .WillOnce(Return(0));

  ASSERT_EQ(dut_->SetValue(input), 0);
}

TEST_F(V4L2ControlDelegateTest, SetConverterFailure) {
  uint8_t input = 10;
  int err = 12;
  EXPECT_CALL(*mock_converter_, MetadataToV4L2(input, _)).WillOnce(Return(err));
  ASSERT_EQ(dut_->SetValue(input), err);
}

TEST_F(V4L2ControlDelegateTest, SetDeviceFailure) {
  uint8_t input = 10;
  int32_t conversion_result = 99;
  EXPECT_CALL(*mock_converter_, MetadataToV4L2(input, _))
      .WillOnce(DoAll(SetArgPointee<1>(conversion_result), Return(0)));
  int err = 66;
  EXPECT_CALL(*mock_device_, SetControl(control_id_, conversion_result, _))
      .WillOnce(Return(err));

  ASSERT_EQ(dut_->SetValue(input), err);
}

}  // namespace v4l2_camera_hal
