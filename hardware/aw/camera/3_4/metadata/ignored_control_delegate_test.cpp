
#include "ignored_control_delegate.h"

#include <gtest/gtest.h>

using testing::Test;

namespace v4l2_camera_hal {

TEST(IgnoredControlDelegateTest, DefaultGet) {
  int32_t value = 12;
  IgnoredControlDelegate<int32_t> control(value);
  int32_t actual = 0;
  ASSERT_EQ(control.GetValue(&actual), 0);
  EXPECT_EQ(actual, value);
}

TEST(IgnoredControlDelegateTest, GetAndSet) {
  int32_t value = 12;
  IgnoredControlDelegate<int32_t> control(value);
  int32_t new_value = 13;
  ASSERT_EQ(control.SetValue(new_value), 0);
  int32_t actual = 0;
  ASSERT_EQ(control.GetValue(&actual), 0);
  // Should still be the default.
  EXPECT_EQ(actual, value);
}

}  // namespace v4l2_camera_hal
