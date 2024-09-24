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

#include "genit/concat_range.h"

#include <array>
#include <forward_list>
#include <iterator>
#include <list>
#include <type_traits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ne;

template <typename ReferenceType, typename Range>
void ExpectReferenceType(Range&& range) {
  static_assert(
      std::is_same<
          typename std::iterator_traits<decltype(range.begin())>::reference,
          ReferenceType>::value,
      "The reference type of the range is other than expected.");
}

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

TEST(ConcatRange, EmptyRange) {
  std::vector<int> v;
  const auto range = ConcatenateRanges(v, v, v);
  ExpectReferenceType<int&>(range);
  EXPECT_THAT(range.begin(), Eq(range.end()));
}

TEST(ConcatRange, NonEmptyRange) {
  std::vector<int> v = {1, 2, 3};
  const auto range = ConcatenateRanges(v, v, v);
  ExpectReferenceType<int&>(range);
  EXPECT_THAT(range.begin(), Ne(range.end()));
}

TEST(ConcatRange, Increment) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = ConcatenateRanges(xs, ys);
  ExpectReferenceType<int&>(range);

  auto it = range.begin();
  for (int i = 0; i < 6; ++i) {
    EXPECT_THAT(*it, Eq(i + 1));
    ++it;
  }
  EXPECT_THAT(it, Eq(range.end()));
}

TEST(ConcatRange, IncrementWithSomeEmptyRanges) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = ConcatenateRanges(empty, xs, empty, ys, empty);
  ExpectReferenceType<int&>(range);

  auto it = range.begin();
  for (int i = 0; i < 6; ++i) {
    EXPECT_THAT(*it, Eq(i + 1));
    ++it;
  }
  EXPECT_THAT(it, Eq(range.end()));
}

TEST(ConcatRange, Decrement) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = ConcatenateRanges(xs, ys);
  ExpectReferenceType<int&>(range);

  auto it = range.end();
  for (int i = 5; i >= 0; --i) {
    --it;
    EXPECT_THAT(*it, Eq(i + 1));
    EXPECT_THAT(range.begin() + i, Eq(it));
  }
  EXPECT_THAT(it, Eq(range.begin()));
}

TEST(ConcatRange, DecrementWithSomeEmptyRanges) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = ConcatenateRanges(empty, xs, empty, ys, empty);
  ExpectReferenceType<int&>(range);

  auto it = range.end();
  for (int i = 5; i >= 0; --i) {
    --it;
    EXPECT_THAT(*it, Eq(i + 1));
    EXPECT_THAT(range.begin() + i, Eq(it));
  }
  EXPECT_THAT(it, Eq(range.begin()));
}

TEST(ConcatRange, Distance) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  const auto range = ConcatenateRanges(xs, ys);
  ExpectReferenceType<int&>(range);

  for (int i = 0; i < 6; ++i) {
    const auto it = range.begin() + i;
    EXPECT_THAT(*it, Eq(i + 1));
    EXPECT_THAT(it - range.begin(), Eq(i));
    EXPECT_THAT(range.end() - it, Eq(6 - i));
    EXPECT_THAT(it + (6 - i), Eq(range.end()));
  }
}

TEST(ConcatRange, DistanceWithSomeEmptyRanges) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = ConcatenateRanges(empty, xs, empty, ys, empty);
  ExpectReferenceType<int&>(range);

  for (int i = 0; i < 6; ++i) {
    const auto it = range.begin() + i;
    EXPECT_THAT(*it, Eq(i + 1));
    EXPECT_THAT(it - range.begin(), Eq(i));
    EXPECT_THAT(range.end() - it, Eq(6 - i));
    EXPECT_THAT(it + (6 - i), Eq(range.end()));
  }
}

TEST(ConcatRange, DecrementAndIncrement) {
  std::vector<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = ConcatenateRanges(empty, xs, empty, ys, empty);
  ExpectReferenceType<int&>(range);

  for (auto it = range.begin(); it != range.end(); ++it) {
    const auto copy = it;
    EXPECT_THAT(--(++(it)), Eq(copy));
  }

  for (auto it = range.end(); it != range.begin(); --it) {
    const auto copy = it;
    EXPECT_THAT(++(--(it)), Eq(copy));
  }
}

TEST(ConcatRange, DecrementAndIncrementBidirectional) {
  std::list<int> xs = {1, 2, 3};
  std::vector<int> ys = {4, 5, 6};
  std::vector<int> empty;
  const auto range = ConcatenateRanges(empty, xs, empty, ys, empty);
  ExpectReferenceType<int&>(range);

  for (auto it = range.begin(); it != range.end(); ++it) {
    const auto copy = it;
    EXPECT_THAT(--(++(it)), Eq(copy));
  }

  for (auto it = range.end(); it != range.begin(); --it) {
    const auto copy = it;
    EXPECT_THAT(++(--(it)), Eq(copy));
  }
}

TEST(ConcatRange, SingleRangeForLoop) {
  const std::vector<int> values = {1, 2, 3};
  const auto range = ConcatenateRanges(values);
  ExpectReferenceType<const int&>(range);

  int size = range.end() - range.begin();
  EXPECT_THAT(size, Eq(3));
  EXPECT_THAT(range.begin() + size, Eq(range.end()));

  // Test usage in range-based for loop.
  auto expected = values.begin();
  for (const auto& x : range) {
    EXPECT_THAT(x, Eq(*expected));
    ++expected;
  }
  EXPECT_THAT(expected, Eq(values.end()));
}

TEST(ConcatRange, NonAssignableType) {
  std::array<NonAssignableType, 3> v = {
      NonAssignableType{1}, NonAssignableType{2}, NonAssignableType{3}};
  const auto range = ConcatenateRanges(v);
  ExpectReferenceType<NonAssignableType&>(range);

  int i = 1;
  for (const auto& x : range) {
    EXPECT_THAT(x.value, Eq(i++));
  }
  EXPECT_THAT(i, Eq(4));
}

TEST(ConcatRange, CommonType) {
  std::vector<int> v1 = {1, 2, 3};
  std::vector<ConvertibleType> v2;
  v2.emplace_back(4);
  v2.emplace_back(5);
  v2.emplace_back(6);

  const auto range = ConcatenateRanges(v1, v2);
  ExpectReferenceType<int>(range);

  int i = 1;
  for (const auto x : range) {
    EXPECT_THAT(x, Eq(i++));
  }
  EXPECT_THAT(i, Eq(7));
}

TEST(ConcatRange, ConcatVariousRangeTypes) {
  const std::vector<int> xs = {1, 2, 3};
  const std::vector<int> ys = {4, 5, 6};
  std::vector<ConvertibleType> zs;
  zs.emplace_back(7);
  zs.emplace_back(8);
  zs.emplace_back(9);

  const auto range = ConcatenateRanges(xs, ys, zs);
  ExpectReferenceType<int>(range);

  int i = 1;
  for (const auto x : range) {
    EXPECT_THAT(x, Eq(i++));
  }
  EXPECT_THAT(i, Eq(10));

  const auto const_ref_range = ConcatenateRanges(xs, ys);
  ExpectReferenceType<const int&>(const_ref_range);
}

TEST(ConcatRange, CommonWritableIterator) {
  std::vector<int> xs = {1, 2, 3};
  int ys[] = {4, 5, 6};
  std::forward_list<int> zs = {7, 8, 9};

  const auto range = ConcatenateRanges(xs, ys, zs);
  ExpectReferenceType<int&>(range);

  for (auto& x : range) {
    x = -2 * x;
  }
  EXPECT_THAT(xs, ElementsAre(-2, -4, -6));
  EXPECT_THAT(ys, ElementsAre(-8, -10, -12));
  EXPECT_THAT(zs, ElementsAre(-14, -16, -18));
}

TEST(ConcatRange, IteratorTag) {
  std::vector<int> random;
  std::list<int> bidirectional;
  std::forward_list<int> forward;

  const auto random_range = ConcatenateRanges(random);
  ExpectIteratorCategory(random_range, std::random_access_iterator_tag{});

  const auto bidir_range = ConcatenateRanges(random, bidirectional);
  ExpectIteratorCategory(bidir_range, std::bidirectional_iterator_tag{});

  const auto forward_range = ConcatenateRanges(random, bidirectional, forward);
  ExpectIteratorCategory(forward_range, std::forward_iterator_tag{});
}

}  // namespace
}  // namespace genit
