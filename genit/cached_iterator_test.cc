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

#include "genit/cached_iterator.h"

#include <functional>
#include <map>
#include <vector>

#include "genit/transform_iterator.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

TEST(CachedIteratorTest, SquaringIterator) {
  std::vector<int> v = {0, 1, 2, 3, 4};

  int invocation_count = 0;
  auto squaring_func = [&invocation_count](int i) {
    ++invocation_count;
    return i * i;
  };

  auto c_range = CachedRange(TransformRange(v, squaring_func));
  auto it = c_range.begin();
  auto it_end = c_range.end();

  EXPECT_TRUE((std::is_same_v<decltype(*it), int>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(invocation_count, 0);
  EXPECT_EQ(*it, 0);
  EXPECT_EQ(invocation_count, 1);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(invocation_count, 2);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(invocation_count, 3);
  EXPECT_EQ(it[2], 4);
  EXPECT_EQ(invocation_count, 4);
  EXPECT_EQ(it[3], 9);
  EXPECT_EQ(invocation_count, 5);
  EXPECT_EQ(it[4], 16);
  EXPECT_EQ(invocation_count, 6);

  ++it;
  EXPECT_EQ(invocation_count, 6);
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(invocation_count, 7);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(invocation_count, 7);
  EXPECT_EQ(*it, 4);
  EXPECT_EQ(invocation_count, 8);

  --it;
  EXPECT_EQ(invocation_count, 8);
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(invocation_count, 9);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(invocation_count, 9);
  EXPECT_EQ(*it, 0);
  EXPECT_EQ(invocation_count, 10);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(invocation_count, 10);
  EXPECT_EQ(*(it + 2), 4);
  EXPECT_EQ(invocation_count, 11);
  EXPECT_EQ(*(3 + it), 9);
  EXPECT_EQ(invocation_count, 12);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
  EXPECT_EQ(invocation_count, 12);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_EQ(invocation_count, 13);
}

TEST(CachedIteratorTest, CachedRange) {
  std::vector<int> v = {0, 1, 2, 3, 4};
  int sum = 0;
  for (auto x : CachedRange(v)) {
    sum += x;
  }
  EXPECT_EQ(sum, 10);
  sum = 0;
  for (auto x : CachedRange(v.begin(), v.end())) {
    sum += x;
  }
  EXPECT_EQ(sum, 10);
}

}  // namespace
}  // namespace genit
