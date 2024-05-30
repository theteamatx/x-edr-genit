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

#include "genit/filter_iterator.h"

#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <sstream>

#include "genit/iterator_range.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

using ::testing::Test;

struct TruePredicate {
  template <typename T>
  bool operator()(T&& x) const {
    return true;
  }
};

struct FalsePredicate {
  template <typename T>
  bool operator()(T&& x) const {
    return false;
  }
};

struct IsEven {
  bool operator()(int x) const { return x % 2 == 0; }
};

struct IsOdd {
  bool operator()(int x) const { return x % 2 == 1; }
};

struct IsPositive {
  bool operator()(int x) const { return x > 0; }
};

TEST(FilterIteratorTest, FullRangeOnTruePredicate) {
  const int xs[] = {1, 2, 3, 4, 5};
  // Since the zip range assumes equally sized ranges, ensure that we do not
  // loop infinitely.
  int count = 0;
  for (const auto pair : ZipRange(FilterRange(xs, TruePredicate{}), xs)) {
    ASSERT_LT(count++, 5);
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

TEST(FilterIteratorTest, EmptyRangeOnFalsePredicate) {
  const int xs[] = {1, 2, 3, 4, 5};
  const auto empty = FilterRange(xs, FalsePredicate{});
  EXPECT_EQ(empty.begin(), empty.end());
}

TEST(FilterIteratorTest, LambdaPredicate) {
  const int xs[] = {1, 2, 3, 4, 5};
  auto predicate = [](int x) { return false; };
  const auto empty = FilterRange(xs, predicate);
  EXPECT_EQ(empty.begin(), empty.end());
  const auto other_empty = FilterRange(xs, std::function<bool(int)>(predicate));
  EXPECT_EQ(other_empty.begin(), other_empty.end());
}

TEST(FilterIteratorTest, ForwardIterationSkippingBoundary) {
  const int xs[] = {1, 2, 3, 4, 5};
  const int expected[] = {2, 4};
  int count = 0;
  for (const auto pair : ZipRange(FilterRange(xs, IsEven{}), expected)) {
    ASSERT_LT(count++, 2);
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

TEST(FilterIteratorTest, ForwardIterationIncludingBoundary) {
  const int xs[] = {1, 2, 3, 4, 5};
  const int expected[] = {1, 3, 5};
  int count = 0;
  for (const auto pair : ZipRange(FilterRange(xs, IsOdd{}), expected)) {
    ASSERT_LT(count++, 3);
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

TEST(FilterIteratorTest, BackwardIterationSkippingBoundary) {
  const int xs[] = {1, 2, 3, 4, 5};
  const int expected[] = {4, 2};
  int count = 0;
  for (const auto pair :
       ZipRange(ReverseRange(FilterRange(xs, IsEven{})), expected)) {
    ASSERT_LT(count++, 2);
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

TEST(FilterIteratorTest, BackwardIterationIncludingBoundary) {
  const int xs[] = {1, 2, 3, 4, 5};
  const int expected[] = {5, 3, 1};
  int count = 0;
  for (const auto pair :
       ZipRange(ReverseRange(FilterRange(xs, IsOdd{})), expected)) {
    ASSERT_LT(count++, 3);
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

TEST(FilterIteratorTest, WriteToFilteredRange) {
  int xs[] = {0, 2, 0, 4, 5};
  const int expected[] = {0, 1, 0, 1, 1};
  for (auto& x : FilterRange(xs, IsPositive{})) {
    x = 1;
  }

  for (const auto pair : ZipRange(xs, expected)) {
    EXPECT_EQ(std::get<0>(pair), std::get<1>(pair));
  }
}

template <typename Iterator, typename Category>
void ExpectIteratorCategory(Iterator&& it, Category&& category) {
  using std::begin;
  static_assert(
      std::is_same_v<typename std::iterator_traits<Iterator>::iterator_category,
                     Category>,
      "");
}

TEST(FilterIteratorTest, IteratorCategory) {
  const int random[] = {1, 2, 3, 4, 5};
  using std::begin;
  ExpectIteratorCategory(begin(FilterRange(random, TruePredicate{})),
                         std::bidirectional_iterator_tag{});

  std::list<int> bidir = {1, 2, 3, 4, 5};
  ExpectIteratorCategory(begin(FilterRange(bidir, TruePredicate{})),
                         std::bidirectional_iterator_tag{});

  std::forward_list<int> forward = {1, 2, 3, 4, 5};
  ExpectIteratorCategory(begin(FilterRange(forward, TruePredicate{})),
                         std::forward_iterator_tag{});

  std::istringstream input("Some input");
  ExpectIteratorCategory(
      begin(FilterRange(MakeIteratorRange(std::istream_iterator<char>(input),
                                          std::istream_iterator<char>()),
                        TruePredicate{})),
      std::input_iterator_tag{});
}

}  // namespace
}  // namespace genit
