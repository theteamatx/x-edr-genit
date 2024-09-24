// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "genit/stride_iterator.h"

#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

namespace genit {
namespace {

TEST(StrideIteratorTest, IntIterator) {
  std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto even_it = MakeStrideIterator(v.data(), 2 * sizeof(int));
  auto even_it_end = MakeStrideIterator(v.data() + v.size(), 2 * sizeof(int));

  EXPECT_TRUE((std::is_same_v<decltype(*even_it), int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*even_it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*even_it)>);

  EXPECT_FALSE(even_it == even_it_end);
  EXPECT_TRUE(even_it != even_it_end);
  EXPECT_TRUE(even_it < even_it_end);
  EXPECT_TRUE(even_it <= even_it_end);
  EXPECT_FALSE(even_it > even_it_end);
  EXPECT_FALSE(even_it >= even_it_end);

  EXPECT_EQ(*even_it, 0);
  EXPECT_EQ(even_it[0], 0);
  EXPECT_EQ(even_it[1], 2);
  EXPECT_EQ(even_it[2], 4);
  EXPECT_EQ(even_it[3], 6);
  EXPECT_EQ(even_it[4], 8);

  ++even_it;
  EXPECT_EQ(*even_it, 2);
  EXPECT_EQ(*(even_it++), 2);
  EXPECT_EQ(*even_it, 4);

  --even_it;
  EXPECT_EQ(*even_it, 2);
  EXPECT_EQ(*(even_it--), 2);
  EXPECT_EQ(*even_it, 0);

  EXPECT_EQ(even_it_end - even_it, 5);
  EXPECT_EQ(*(even_it + 2), 4);
  EXPECT_EQ(*(3 + even_it), 6);

  even_it += 5;
  EXPECT_TRUE(even_it == even_it_end);
  EXPECT_FALSE(even_it != even_it_end);

  even_it -= 5;
  EXPECT_EQ(*even_it, 0);
  EXPECT_FALSE(even_it == even_it_end);
  EXPECT_TRUE(even_it != even_it_end);
}

TEST(StrideIteratorTest, StrideRange) {
  std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int sum = 0;
  for (auto x : StrideRange(v.data(), v.data() + v.size(), 2 * sizeof(int))) {
    sum += x;
  }
  EXPECT_EQ(sum, 20);
}

struct BaseClassForTest {
  int derived_value = 0;
};

struct DerivedClassForTest : BaseClassForTest {
  explicit DerivedClassForTest(int value) { derived_value = value; }

  // Unused data to add stride.
  double more_data;  // NOLINT
};

TEST(StrideIteratorTest, BaseClassIterator) {
  std::vector<DerivedClassForTest> v;
  for (int i : {0, 1, 2, 3, 4}) {
    v.emplace_back(i);
  }

  auto it = MakeStrideIterator<BaseClassForTest>(v.data(),
                                                 sizeof(DerivedClassForTest));
  auto it_end = MakeStrideIterator<BaseClassForTest>(
      v.data() + v.size(), sizeof(DerivedClassForTest));

  EXPECT_TRUE((std::is_same_v<decltype(*it), BaseClassForTest&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(it->derived_value, 0);
  EXPECT_EQ(it[0].derived_value, 0);
  EXPECT_EQ(it[1].derived_value, 1);
  EXPECT_EQ(it[2].derived_value, 2);
  EXPECT_EQ(it[3].derived_value, 3);
  EXPECT_EQ(it[4].derived_value, 4);

  ++it;
  EXPECT_EQ(it->derived_value, 1);
  EXPECT_EQ((it++)->derived_value, 1);
  EXPECT_EQ(it->derived_value, 2);

  --it;
  EXPECT_EQ(it->derived_value, 1);
  EXPECT_EQ((it--)->derived_value, 1);
  EXPECT_EQ(it->derived_value, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ((it + 2)->derived_value, 2);
  EXPECT_EQ((3 + it)->derived_value, 3);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(it->derived_value, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

}  // namespace
}  // namespace genit
