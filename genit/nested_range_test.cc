// Copyright 2024 Google LLC
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

#include "genit/nested_range.h"

#include <array>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <sstream>
#include <vector>

#include "genit/iterator_range.h"
#include "genit/zip_iterator.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::DoubleNear;
using ::testing::Ne;

template <typename Category, typename Range>
void ExpectIteratorCategory(Range&& range, Category&& category) {
  using std::begin;
  static_assert(
      std::is_same<typename std::iterator_traits<decltype(begin(
                       std::forward<Range>(range)))>::iterator_category,
                   Category>::value,
      "");
}

struct NonAssignableType {
  explicit NonAssignableType(int x) : value(x) {}
  NonAssignableType(const NonAssignableType&) = delete;
  NonAssignableType& operator=(const NonAssignableType&) = delete;
  NonAssignableType(NonAssignableType&&) = delete;
  NonAssignableType& operator=(NonAssignableType&&) = delete;

  int value = 0;
};

struct ConvertibleType {
  explicit ConvertibleType(int x) : value(x) {}
  operator int() const { return value; }  // NOLINT
  int value = 0;
};

TEST(NestedRange, EmptyRange) {
  std::vector<int> v;
  const auto range = NestRanges(v, v, v);
  EXPECT_THAT(range.begin(), Eq(range.end()));
}

TEST(NestedRange, SomeEmptyRanges) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = NestRanges(empty, xs, empty, ys, empty);
  EXPECT_THAT(range.begin(), Eq(range.end()));
}

TEST(NestedRange, NonEmptyRange) {
  std::vector<int> v = {1, 2, 3};
  const auto range = NestRanges(v, v, v);
  EXPECT_THAT(range.begin(), Ne(range.end()));
}

TEST(NestedRange, Increment) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = NestRanges(xs, ys);

  auto it = range.begin();
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      auto [x, y] = *it;
      EXPECT_THAT(x, Eq(i + 1));
      EXPECT_THAT(y, Eq(j + 4));
      ++it;
    }
  }
  EXPECT_THAT(it, Eq(range.end()));
}

TEST(NestedRange, Decrement) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = NestRanges(xs, ys);

  auto it = range.end();
  for (int i = 2; i >= 0; --i) {
    for (int j = 2; j >= 0; --j) {
      --it;
      auto [x, y] = *it;
      EXPECT_THAT(x, Eq(i + 1));
      EXPECT_THAT(y, Eq(j + 4));
    }
  }
  EXPECT_THAT(it, Eq(range.begin()));
}

TEST(NestedRange, DecrementAndIncrement) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = NestRanges(xs, ys);

  for (auto it = range.begin(); it != range.end(); ++it) {
    const auto copy = it;
    EXPECT_THAT(--(++(it)), Eq(copy));
  }

  for (auto it = range.end(); it != range.begin(); --it) {
    const auto copy = it;
    EXPECT_THAT(++(--(it)), Eq(copy));
  }
}

TEST(NestedRange, DecrementAndIncrementBidirectional) {
  std::list<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = NestRanges(xs, ys);

  for (auto it = range.begin(); it != range.end(); ++it) {
    const auto copy = it;
    EXPECT_THAT(--(++(it)), Eq(copy));
  }

  for (auto it = range.end(); it != range.begin(); --it) {
    const auto copy = it;
    EXPECT_THAT(++(--(it)), Eq(copy));
  }
}

TEST(NestedRange, SingleRangeForLoop) {
  const std::vector<int> values = {1, 2, 3};
  const auto range = NestRanges(values);

  // Test usage in range-based for loop.
  auto expected = values.begin();
  for (const auto& [x] : range) {
    EXPECT_THAT(x, Eq(*expected));
    ++expected;
  }
  EXPECT_THAT(expected, Eq(values.end()));
}

TEST(NestedRange, NonAssignableType) {
  std::array<NonAssignableType, 3> v = {
      NonAssignableType{1}, NonAssignableType{2}, NonAssignableType{3}};
  const auto range = NestRanges(v);

  int i = 1;
  for (const auto& [x] : range) {
    EXPECT_THAT(x.value, Eq(i++));
  }
  EXPECT_THAT(i, Eq(4));
}

TEST(NestedRange, ConcatVariousRangeTypes) {
  const std::vector<int> xs = {1, 2, 3};
  std::vector<double> ys = {4.1, 5.2, 6.3};
  const auto range = NestRanges(xs, ys);

  auto it = range.begin();
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      auto [x, y] = *it;
      EXPECT_THAT(x, Eq(i + 1));
      EXPECT_THAT(y, DoubleNear(j + 4.1 + j * 0.1, 1e-14));
      ++it;
    }
  }
}

TEST(NestedRange, IteratorTag) {
  std::vector<int> random;
  std::list<int> bidirectional;
  std::forward_list<int> forward;

  const auto random_range = NestRanges(random);
  ExpectIteratorCategory(random_range, std::bidirectional_iterator_tag{});

  const auto bidir_range = NestRanges(random, bidirectional);
  ExpectIteratorCategory(bidir_range, std::bidirectional_iterator_tag{});

  const auto forward_range = NestRanges(random, bidirectional, forward);
  ExpectIteratorCategory(forward_range, std::forward_iterator_tag{});
}

}  // namespace
}  // namespace genit
