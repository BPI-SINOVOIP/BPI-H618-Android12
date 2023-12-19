// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "icing/result/projection-tree.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/proto/search.pb.h"

namespace icing {
namespace lib {

namespace {

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(ProjectionTreeTest, CreateEmptyFieldMasks) {
  TypePropertyMask type_field_mask;
  ProjectionTree tree(type_field_mask);
  EXPECT_THAT(tree.root().name, IsEmpty());
  EXPECT_THAT(tree.root().children, IsEmpty());
}

TEST(ProjectionTreeTest, CreateTreeTopLevel) {
  TypePropertyMask type_field_mask;
  type_field_mask.add_paths("subject");

  ProjectionTree tree(type_field_mask);
  EXPECT_THAT(tree.root().name, IsEmpty());
  ASSERT_THAT(tree.root().children, SizeIs(1));
  ASSERT_THAT(tree.root().children.at(0).name, Eq("subject"));
  ASSERT_THAT(tree.root().children.at(0).children, IsEmpty());
}

TEST(ProjectionTreeTest, CreateTreeMultipleTopLevel) {
  TypePropertyMask type_field_mask;
  type_field_mask.add_paths("subject");
  type_field_mask.add_paths("body");

  ProjectionTree tree(type_field_mask);
  EXPECT_THAT(tree.root().name, IsEmpty());
  ASSERT_THAT(tree.root().children, SizeIs(2));
  ASSERT_THAT(tree.root().children.at(0).name, Eq("subject"));
  ASSERT_THAT(tree.root().children.at(0).children, IsEmpty());
  ASSERT_THAT(tree.root().children.at(1).name, Eq("body"));
  ASSERT_THAT(tree.root().children.at(1).children, IsEmpty());
}

TEST(ProjectionTreeTest, CreateTreeNested) {
  TypePropertyMask type_field_mask;
  type_field_mask.add_paths("subject.body");
  type_field_mask.add_paths("body");

  ProjectionTree tree(type_field_mask);
  EXPECT_THAT(tree.root().name, IsEmpty());
  ASSERT_THAT(tree.root().children, SizeIs(2));
  ASSERT_THAT(tree.root().children.at(0).name, Eq("subject"));
  ASSERT_THAT(tree.root().children.at(0).children, SizeIs(1));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).name, Eq("body"));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).children, IsEmpty());
  ASSERT_THAT(tree.root().children.at(1).name, Eq("body"));
  ASSERT_THAT(tree.root().children.at(1).children, IsEmpty());
}

TEST(ProjectionTreeTest, CreateTreeNestedSharedNode) {
  TypePropertyMask type_field_mask;
  type_field_mask.add_paths("sender.name.first");
  type_field_mask.add_paths("sender.emailAddress");

  ProjectionTree tree(type_field_mask);
  EXPECT_THAT(tree.root().name, IsEmpty());
  ASSERT_THAT(tree.root().children, SizeIs(1));
  ASSERT_THAT(tree.root().children.at(0).name, Eq("sender"));
  ASSERT_THAT(tree.root().children.at(0).children, SizeIs(2));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).name, Eq("name"));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).children, SizeIs(1));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).children.at(0).name,
              Eq("first"));
  ASSERT_THAT(tree.root().children.at(0).children.at(0).children.at(0).children,
              IsEmpty());
  ASSERT_THAT(tree.root().children.at(0).children.at(1).name,
              Eq("emailAddress"));
  ASSERT_THAT(tree.root().children.at(0).children.at(1).children, IsEmpty());
}

}  // namespace

}  // namespace lib
}  // namespace icing
