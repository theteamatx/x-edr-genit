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

#include "genit/circular_iterator.h"

#include <iterator>
#include <type_traits>
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
  explicit NonAssignableType(int x) : value_(x) {}
  NonAssignableType(const NonAssignableType&) = default;
  NonAssignableType(NonAssignableType&&) = default;
  NonAssignableType operator=(const NonAssignableType&) = delete;
  NonAssignableType operator=(NonAssignableType&&) = delete;
  int value_;
};

TEST(CircularIteratorTest, IteratorComparison) {
  auto range = CircularRange(IndexRange(0, 5));
  auto it = range.begin();
  auto it_end = range.end();

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);
}

TEST(CircularIteratorTest, IteratorDereference) {
  auto range = CircularRange(IndexRange(0, 5));
  auto it = range.begin();

  EXPECT_EQ(*it, 0);
}

TEST(CircularIteratorTest, IteratorIncrementDecrement) {
  auto range = CircularRange(IndexRange(0, 5));
  auto it = range.begin();

  ++it;
  auto value = *it;
  EXPECT_EQ(value, 1);

  value = *(it++);
  EXPECT_EQ(value, 1);

  value = *it;
  EXPECT_EQ(value, 2);

  value = *(++it);
  EXPECT_EQ(value, 3);

  value = *(++it);
  EXPECT_EQ(value, 4);

  value = *(--it);
  EXPECT_EQ(value, 3);

  value = *(--it);
  EXPECT_EQ(value, 2);

  --it;
  value = *it;
  EXPECT_EQ(value, 1);

  value = *(it--);
  EXPECT_EQ(value, 1);

  value = *it;
  EXPECT_EQ(value, 0);
}

TEST(CircularIteratorTest, IteratorRandomAccess) {
  auto range = CircularRange(IndexRange(0, 5));
  auto it = range.begin();
  auto it_end = range.end();

  EXPECT_EQ(it_end - it, 5);

  auto value = *(it + 2);
  EXPECT_EQ(value, 2);

  value = *(2 + it);
  EXPECT_EQ(value, 2);

  value = *(it - 4);
  EXPECT_EQ(value, 1);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  value = *it;
  EXPECT_EQ(value, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(CircularIteratorTest, IteratorFromContainer) {
  std::vector<int> values(5);
  auto range = CircularRange(values);
  auto it = range.begin();
  auto it_end = range.end();

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

TEST(CircularIteratorTest, MultipleWindings) {
  constexpr int n_windings = 7;
  auto range = CircularRange(IndexRange(0, 6), n_windings);

  int counter = 0;
  for (const int n : range) {
    EXPECT_EQ(n, counter % 6);
    ++counter;
  }
  EXPECT_EQ(counter, 42);

  for (const int n : ReverseRange(range)) {
    --counter;
    EXPECT_EQ(n, counter % 6);
  }
  EXPECT_EQ(counter, 0);
}

TEST(CircularIteratorTest, ValueIncrementIterator) {
  int arr[] = {0, 1, 2, 3, 4, 5, 6};
  auto range = CircularRange(arr);
  auto it = range.begin();
  auto it_end = range.end();

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype((*it))>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype((*it))>);

  for (int& t : range) {
    t += 1;
  }

  EXPECT_EQ(arr[0], 1);
  EXPECT_EQ(arr[1], 2);
  EXPECT_EQ(arr[2], 3);
  EXPECT_EQ(arr[3], 4);
  EXPECT_EQ(arr[4], 5);
  EXPECT_EQ(arr[5], 6);
  EXPECT_EQ(arr[6], 7);
}

// This is mostly to test that  iterators can be created (and compile
// successfully) for iterators that return values of non-assignable types.
TEST(CircularIteratorTest, IteratorNonAssignableType) {
  auto get_non_assignable = [](int x) { return NonAssignableType(x); };

  auto range =
      CircularRange(TransformRange(IndexRange(0, 5), get_non_assignable));

  int expected = 0;
  for (auto t : range) {
    EXPECT_EQ(t.value_, expected);
    ++expected;
  }
}

TEST(CircularIteratorTest, EmptyRange) {
  std::vector<int> values;
  auto range = CircularRange(values);
  EXPECT_EQ(range.begin(), range.end());
}

TEST(CircularIteratorTest, CircularConnectRange) {
  const int values[] = {2, 3, 4, 5};
  auto range = CircularConnectRange(values);
  auto range_from_it =
      CircularConnectRange(std::begin(values), std::end(values));
  EXPECT_EQ(range, range_from_it);

  const int result[] = {2, 3, 4, 5, 2};
  ASSERT_EQ(range.end() - range.begin(), std::size(result));
  auto it_pair = std::mismatch(range.begin(), range.end(), std::begin(result));
  EXPECT_EQ(it_pair.first, range.end())
      << "Mismatch at " << (it_pair.first - range.begin()) << ": got "
      << *it_pair.first << " instead of " << *it_pair.second;

  auto empty_range =
      CircularConnectRange(IteratorRange(IndexIterator(0), IndexIterator(0)));
  auto empty_range_from_it =
      CircularConnectRange(IndexIterator(0), IndexIterator(0));
  EXPECT_EQ(empty_range, empty_range_from_it);
  EXPECT_EQ(empty_range.begin(), empty_range.end());
}

}  // namespace
}  // namespace genit
