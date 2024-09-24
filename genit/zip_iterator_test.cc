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

#include "genit/zip_iterator.h"

#include <cstdint>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

namespace genit {
namespace {

TEST(ZipIterator, SquareValuesIterator) {
  std::vector<int> v = {0, 1, 2, 3, 4};
  std::vector<int> v_sqr = {0, 1, 4, 9, 16};

  auto it = MakeZipIterator(v.begin(), v_sqr.begin());
  auto it_end = MakeZipIterator(v.end(), v_sqr.end());

  EXPECT_TRUE((std::is_same_v<decltype(std::get<0>(*it)), int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(std::get<0>(*it))>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(std::get<0>(*it))>);

  EXPECT_TRUE((std::is_same_v<decltype(std::get<1>(*it)), int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(std::get<1>(*it))>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(std::get<1>(*it))>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(std::get<0>(*it), 0);
  EXPECT_EQ(std::get<1>(*it), 0);
  EXPECT_EQ(std::get<0>(it[0]), 0);
  EXPECT_EQ(std::get<1>(it[0]), 0);
  EXPECT_EQ(std::get<0>(it[1]), 1);
  EXPECT_EQ(std::get<1>(it[1]), 1);
  EXPECT_EQ(std::get<0>(it[2]), 2);
  EXPECT_EQ(std::get<1>(it[2]), 4);
  EXPECT_EQ(std::get<0>(it[3]), 3);
  EXPECT_EQ(std::get<1>(it[3]), 9);
  EXPECT_EQ(std::get<0>(it[4]), 4);
  EXPECT_EQ(std::get<1>(it[4]), 16);

  ++it;
  EXPECT_EQ(std::get<0>(*it), 1);
  EXPECT_EQ(std::get<1>(*it), 1);
  EXPECT_EQ(std::get<1>(*(it++)), 1);
  EXPECT_EQ(std::get<1>(*it), 4);

  --it;
  EXPECT_EQ(std::get<1>(*it), 1);
  EXPECT_EQ(std::get<1>(*(it--)), 1);
  EXPECT_EQ(std::get<1>(*it), 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(std::get<1>(*(it + 2)), 4);
  EXPECT_EQ(std::get<1>(*(3 + it)), 9);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(std::get<0>(*it), 0);
  EXPECT_EQ(std::get<1>(*it), 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(ZipIterator, CopyVectors) {
  const std::vector<int> v = {0, 1, 2, 3, 4};
  std::vector<int> v_sqr(5);
  for (auto src_dest : ZipRange(v, v_sqr)) {
    EXPECT_TRUE((std::is_same_v<decltype(std::get<0>(src_dest)), const int&>));
    EXPECT_TRUE((std::is_same_v<decltype(std::get<1>(src_dest)), int&>));
    std::get<1>(src_dest) = std::get<0>(src_dest) * std::get<0>(src_dest);
  }
  EXPECT_EQ(v_sqr[0], 0);
  EXPECT_EQ(v_sqr[1], 1);
  EXPECT_EQ(v_sqr[2], 4);
  EXPECT_EQ(v_sqr[3], 9);
  EXPECT_EQ(v_sqr[4], 16);
}

TEST(ZipIterator, RangesOfDifferentLength) {
  const char shortlist[] = {1, 2, 3};
  const uint64_t longlist[] = {1, 2, 3, 4, 5};
  const auto zipped = ZipRange(shortlist, longlist);
  const int common_length = std::size(shortlist);
  EXPECT_EQ(zipped.end() - zipped.begin(), common_length);
  EXPECT_EQ(zipped.begin() + common_length, zipped.end());
}

TEST(ZipIterator, EnumerateRange) {
  const uint64_t values[] = {1, 2, 3, 4, 5};
  int count = 0;
  for (auto t : EnumerateRange(values)) {
    EXPECT_TRUE(
        (std::is_same_v<decltype(t), std::tuple<int, const uint64_t&>>));
    auto [i, j] = t;
    EXPECT_EQ(i, count);
    EXPECT_EQ(i + 1, j);
    EXPECT_EQ(values[i], j);
    ++count;
  }
  EXPECT_EQ(count, sizeof(values) / sizeof(uint64_t));
}

}  // namespace
}  // namespace genit
