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

#include "genit/adjacent_circular_iterator.h"

#include <functional>
#include <vector>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"
#include "genit/transform_iterator.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

class NonAssignableType {
 public:
  NonAssignableType() = default;
  NonAssignableType(const NonAssignableType&) = default;
  NonAssignableType(NonAssignableType&&) = default;
  NonAssignableType operator=(const NonAssignableType&) = delete;
  NonAssignableType operator=(NonAssignableType&&) = delete;
};

TEST(AdjacentCircularIteratorTest, TripleIteratorComparison) {
  auto triplet_range =
      AdjacentElementsCircularRange<3>(IndexIterator(0), IndexIterator(5));
  auto it = triplet_range.begin();
  auto it_end = triplet_range.end();

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);
}

TEST(AdjacentCircularIteratorTest, TripleIteratorDereference) {
  auto triplet_range =
      AdjacentElementsCircularRange<3>(IndexIterator(0), IndexIterator(5));
  auto it = triplet_range.begin();

  auto value = *it;
  EXPECT_EQ(value.size(), 3);
  EXPECT_EQ(value.front(), 0);
  EXPECT_EQ(value.back(), 2);

  EXPECT_EQ(value[0], 0);
  EXPECT_EQ(value[1], 1);
  EXPECT_EQ(value[2], 2);

  EXPECT_EQ(it[0][0], 0);
  EXPECT_EQ(it[0][1], 1);
  EXPECT_EQ(it[0][2], 2);

  EXPECT_EQ(it[1][0], 1);
  EXPECT_EQ(it[1][1], 2);
  EXPECT_EQ(it[1][2], 3);

  EXPECT_EQ(it[2][0], 2);
  EXPECT_EQ(it[2][1], 3);
  EXPECT_EQ(it[2][2], 4);

  EXPECT_EQ(it[3][0], 3);
  EXPECT_EQ(it[3][1], 4);
  EXPECT_EQ(it[3][2], 0);

  EXPECT_EQ(it[4][0], 4);
  EXPECT_EQ(it[4][1], 0);
  EXPECT_EQ(it[4][2], 1);
}

TEST(AdjacentCircularIteratorTest, TripleIteratorIncrementDecrement) {
  auto triplet_range =
      AdjacentElementsCircularRange<3>(IndexIterator(0), IndexIterator(5));
  auto it = triplet_range.begin();

  ++it;
  auto value = *it;
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);

  value = *(it++);
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);

  value = *it;
  EXPECT_EQ(value[0], 2);
  EXPECT_EQ(value[1], 3);
  EXPECT_EQ(value[2], 4);

  value = *(++it);
  EXPECT_EQ(value[0], 3);
  EXPECT_EQ(value[1], 4);
  EXPECT_EQ(value[2], 0);

  value = *(++it);
  EXPECT_EQ(value[0], 4);
  EXPECT_EQ(value[1], 0);
  EXPECT_EQ(value[2], 1);

  value = *(--it);
  EXPECT_EQ(value[0], 3);
  EXPECT_EQ(value[1], 4);
  EXPECT_EQ(value[2], 0);

  value = *(--it);
  EXPECT_EQ(value[0], 2);
  EXPECT_EQ(value[1], 3);
  EXPECT_EQ(value[2], 4);

  --it;
  value = *it;
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);

  value = *(it--);
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);

  value = *it;
  EXPECT_EQ(value[0], 0);
  EXPECT_EQ(value[1], 1);
  EXPECT_EQ(value[2], 2);
}

TEST(AdjacentCircularIteratorTest, TripleIteratorRandomAccess) {
  auto triplet_range =
      AdjacentElementsCircularRange<3>(IndexIterator(0), IndexIterator(5));
  auto it = triplet_range.begin();
  auto it_end = triplet_range.end();

  EXPECT_EQ(it_end - it, 5);

  auto value = *(it + 2);
  EXPECT_EQ(value[0], 2);
  EXPECT_EQ(value[1], 3);
  EXPECT_EQ(value[2], 4);

  value = *(2 + it);
  EXPECT_EQ(value[0], 2);
  EXPECT_EQ(value[1], 3);
  EXPECT_EQ(value[2], 4);

  value = *(it - 4);
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  value = *it;
  EXPECT_EQ(value[0], 0);
  EXPECT_EQ(value[1], 1);
  EXPECT_EQ(value[2], 2);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(AdjacentCircularIteratorTest, TripleIteratorFromContainer) {
  std::vector<int> values(5);
  auto triplet_range = AdjacentElementsCircularRange<3>(values);
  auto it = triplet_range.begin();
  auto it_end = triplet_range.end();

  EXPECT_EQ(it_end - it, 5);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it < it_end);

  it -= 5;
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_TRUE(it < it_end);

  it += 10;
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it > it_end);
  EXPECT_FALSE(it < it_end);

  it -= 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it < it_end);
}

TEST(AdjacentCircularIteratorTest, FibonacciOutputIterator) {
  int arr[] = {0, 1, 2, 3, 4, 5, 6};
  auto triplet_range = AdjacentElementsCircularRange<3>(arr);
  auto it = triplet_range.begin();
  auto it_end = triplet_range.end();

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype((*it)[0])>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype((*it)[0])>);

  for (auto triplet : triplet_range) {
    EXPECT_EQ(triplet.size(), 3);
    EXPECT_EQ(triplet.front(), triplet[0]);
    triplet[2] = triplet[0] + triplet[1];
    EXPECT_EQ(triplet.back(), triplet[2]);
  }

  EXPECT_EQ(arr[0], 13);
  EXPECT_EQ(arr[1], 21);
  EXPECT_EQ(arr[2], 1);
  EXPECT_EQ(arr[3], 2);
  EXPECT_EQ(arr[4], 3);
  EXPECT_EQ(arr[5], 5);
  EXPECT_EQ(arr[6], 8);
}

// This is mostly to test that adjacent iterators can be created (and compile
// successfully) for iterators that return values of non-assignable types.
TEST(AdjacentCircularIteratorTest, TripleIteratorNonAssignableType) {
  auto get_non_assignable = [](int) { return NonAssignableType(); };

  auto triplet_range = AdjacentElementsCircularRange<3>(
      MakeTransformIterator(IndexIterator(0), std::cref(get_non_assignable)),
      MakeTransformIterator(IndexIterator(5), std::cref(get_non_assignable)));

  for (auto triplet : triplet_range) {
    EXPECT_EQ(triplet.size(), 3);
  }
}

TEST(AdjacentCircularIteratorTest, EmptyRangeIfRangeTooSmall) {
  std::vector<int> values(2);
  auto triplet_range = AdjacentElementsCircularRange<3>(values);
  EXPECT_EQ(triplet_range.begin(), triplet_range.end());
}

TEST(AdjacentCircularIteratorTest, TwoPassesBothDirections) {
  const int values[] = {1, 2, 3, 4, 5};
  auto range = AdjacentElementsCircularRange<2>(values);
  range = IteratorRange(range.begin(), range.end() + std::size(values));

  int counter = 0;
  for (const auto pair : range) {
    EXPECT_EQ(pair.front(), 1 + (counter % 5));
    ++counter;
  }
  EXPECT_EQ(counter, 2 * std::size(values));
  for (const auto pair : ReverseRange(range)) {
    --counter;
    EXPECT_EQ(pair.front(), 1 + (counter % 5));
  }
  EXPECT_EQ(counter, 0);
}

}  // namespace
}  // namespace genit
