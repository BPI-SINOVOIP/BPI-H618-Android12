/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <optional>
#include <vector>

#include <utils/String16.h>
#include <utils/String8.h>

#include "aidl_test_client.h"
#include "gmock/gmock.h"

using android::BBinder;
using android::IBinder;
using android::sp;
using android::String16;
using android::String8;
using android::binder::Status;

using android::aidl::tests::BackendType;
using android::aidl::tests::ByteEnum;
using android::aidl::tests::INamedCallback;
using android::aidl::tests::IntEnum;
using android::aidl::tests::ITestService;
using android::aidl::tests::LongEnum;
using android::aidl::tests::SimpleParcelable;
using android::aidl::tests::StructuredParcelable;

using testing::Eq;
using testing::Ne;

struct RepeatNullableTest : public AidlTest {
  template <typename T>
  void DoTest(Status (ITestService::*func)(const std::optional<T>&, std::optional<T>*),
              std::optional<T> input) {
    std::optional<T> output;
    auto status = (*service.*func)(input, &output);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(output.has_value());
    ASSERT_THAT(*output, Eq(*input));

    input.reset();
    status = (*service.*func)(input, &output);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(output.has_value());
  }
};

TEST_F(RepeatNullableTest, intArray) {
  DoTest(&ITestService::RepeatNullableIntArray, std::make_optional(std::vector<int32_t>{1, 2, 3}));
}

TEST_F(RepeatNullableTest, byteEnumArray) {
  DoTest(&ITestService::RepeatNullableByteEnumArray,
         std::make_optional(std::vector<ByteEnum>{ByteEnum::FOO, ByteEnum::BAR}));
}

TEST_F(RepeatNullableTest, intEnumArray) {
  DoTest(&ITestService::RepeatNullableIntEnumArray,
         std::make_optional(std::vector<IntEnum>{IntEnum::FOO, IntEnum::BAR}));
}

TEST_F(RepeatNullableTest, longEnumArray) {
  DoTest(&ITestService::RepeatNullableLongEnumArray,
         std::make_optional(std::vector<LongEnum>{LongEnum::FOO, LongEnum::BAR}));
}

TEST_F(RepeatNullableTest, string) {
  DoTest(&ITestService::RepeatNullableString, std::optional<String16>("Blooob"));
}

TEST_F(RepeatNullableTest, stringArray) {
  std::vector<std::optional<String16>> input;
  input.push_back(String16("Wat"));
  input.push_back(String16("Blooob"));
  input.push_back(String16("Wat"));
  input.push_back(std::nullopt);
  input.push_back(String16("YEAH"));
  input.push_back(String16("OKAAAAY"));

  DoTest(&ITestService::RepeatNullableStringList, std::make_optional(input));
}

TEST_F(RepeatNullableTest, parcelable) {
  auto input = std::make_optional<StructuredParcelable>();
  input->f = 42;

  std::optional<StructuredParcelable> output;
  auto status = service->RepeatNullableParcelable(input, &output);
  ASSERT_TRUE(status.isOk());
  ASSERT_TRUE(output.has_value());
  ASSERT_THAT(*output, Eq(*input));

  input.reset();
  status = service->RepeatNullableParcelable(input, &output);
  ASSERT_TRUE(status.isOk());
  ASSERT_FALSE(output.has_value());
}

TEST_F(AidlTest, nullBinder) {
  auto status = service->TakesAnIBinder(nullptr);

  if (backend == BackendType::JAVA) {
    ASSERT_TRUE(status.isOk()) << status;
  } else {
    ASSERT_THAT(status.exceptionCode(), Eq(android::binder::Status::EX_NULL_POINTER)) << status;
  }
}

TEST_F(AidlTest, binderListWithNull) {
  if (!cpp_java_tests) GTEST_SKIP() << "Service does not support the CPP/Java-only tests.";

  std::vector<sp<IBinder>> input{new BBinder(), nullptr};
  auto status = cpp_java_tests->TakesAnIBinderList(input);

  if (backend == BackendType::JAVA) {
    ASSERT_TRUE(status.isOk()) << status;
  } else {
    ASSERT_THAT(status.exceptionCode(), Eq(android::binder::Status::EX_NULL_POINTER));
  }
}

TEST_F(AidlTest, nonNullBinder) {
  sp<IBinder> input = new BBinder();
  auto status = service->TakesAnIBinder(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, binderListWithoutNull) {
  if (!cpp_java_tests) GTEST_SKIP() << "Service does not support the CPP/Java-only tests.";

  std::vector<sp<IBinder>> input{new BBinder(), new BBinder()};
  auto status = cpp_java_tests->TakesAnIBinderList(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, nullBinderToAnnotatedMethod) {
  auto status = service->TakesANullableIBinder(nullptr);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, binderListWithNullToAnnotatedMethod) {
  if (!cpp_java_tests) GTEST_SKIP() << "Service does not support the CPP/Java-only tests.";

  std::vector<sp<IBinder>> input{new BBinder(), nullptr};
  auto status = cpp_java_tests->TakesANullableIBinderList(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, nonNullBinderToAnnotatedMethod) {
  sp<IBinder> input = new BBinder();
  auto status = service->TakesANullableIBinder(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, binderListWithoutNullToAnnotatedMethod) {
  if (!cpp_java_tests) GTEST_SKIP() << "Service does not support the CPP/Java-only tests.";

  std::vector<sp<IBinder>> input{new BBinder(), new BBinder()};
  auto status = cpp_java_tests->TakesANullableIBinderList(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlTest, interface) {
  sp<INamedCallback> callback;
  auto status = service->GetCallback(false, &callback);
  ASSERT_TRUE(status.isOk());
  ASSERT_THAT(callback.get(), Ne(nullptr));
}

TEST_F(AidlTest, nullInterface) {
  sp<INamedCallback> callback;
  auto status = service->GetCallback(true, &callback);
  ASSERT_TRUE(status.isOk());
  ASSERT_THAT(callback.get(), Eq(nullptr));
}
