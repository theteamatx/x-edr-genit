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

#include "genit/transform_iterator.h"

#include <functional>
#include <iterator>
#include <map>
#include <vector>

#include "gtest/gtest.h"

namespace genit {
namespace {

TEST(TransformIteratorTest, SquaringIterator) {
  std::vector<int> v = {0, 1, 2, 3, 4};

  auto squaring_func = [](int i) { return i * i; };

  auto it = MakeTransformIterator(v.begin(), std::cref(squaring_func));
  auto it_end = MakeTransformIterator(v.end(), std::cref(squaring_func));

  EXPECT_TRUE((std::is_same_v<decltype(*it), int>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(*it, 0);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(it[2], 4);
  EXPECT_EQ(it[3], 9);
  EXPECT_EQ(it[4], 16);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 4);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), 4);
  EXPECT_EQ(*(3 + it), 9);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(TransformIteratorTest, IteratorForFirstMember) {
  std::map<int, int> m;
  m[0] = 0;
  m[1] = 1;
  m[2] = 4;
  m[3] = 9;
  m[4] = 16;

  auto it = MakeIteratorForFirstMember(m.begin());
  auto it_end = MakeIteratorForFirstMember(m.end());

  EXPECT_TRUE((std::is_same_v<decltype(*it), const int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_EQ(*it, 0);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);
  ++it;
  EXPECT_EQ(*it, 3);

  --it;
  EXPECT_EQ(*it, 2);
  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  std::advance(it, 5);
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  int sum = 0;
  for (int i : RangeOfFirstMember(m)) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);

  sum = 0;
  for (int i : RangeOfFirstMember(m.begin(), m.end())) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);
}

TEST(TransformIteratorTest, IteratorForSecondMember) {
  std::map<int, int> m;
  m[0] = 0;
  m[1] = 1;
  m[2] = 4;
  m[3] = 9;
  m[4] = 16;

  auto it = MakeIteratorForSecondMember(m.begin());
  auto it_end = MakeIteratorForSecondMember(m.end());

  EXPECT_TRUE((std::is_same_v<decltype(*it), int&>));
  EXPECT_FALSE((std::is_same_v<decltype(*it), int>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_EQ(*it, 0);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 4);
  ++it;
  EXPECT_EQ(*it, 9);

  --it;
  EXPECT_EQ(*it, 4);
  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  std::advance(it, 5);
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  int sum = 0;
  for (int i : RangeOfSecondMember(m)) {
    sum += i;
  }
  EXPECT_EQ(sum, 30);

  sum = 0;
  for (int i : RangeOfSecondMember(m.begin(), m.end())) {
    sum += i;
  }
  EXPECT_EQ(sum, 30);
}

TEST(TransformIteratorTest, IteratorWithDereference) {
  int arr[] = {0, 1, 2, 3, 4};
  std::vector<int*> v = {arr, arr + 1, arr + 2, arr + 3, arr + 4};

  auto it = MakeIteratorWithDereference(v.begin());
  auto it_end = MakeIteratorWithDereference(v.end());

  EXPECT_TRUE((std::is_same_v<decltype(*it), int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(*it, 0);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(it[2], 2);
  EXPECT_EQ(it[3], 3);
  EXPECT_EQ(it[4], 4);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), 2);
  EXPECT_EQ(*(3 + it), 3);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  int sum = 0;
  for (int i : RangeWithDereference(v)) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);

  sum = 0;
  for (int i : RangeWithDereference(v.begin(), v.end())) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);
}

TEST(TransformIteratorTest, IteratorToMember) {
  struct Foo {
    int i;
  };
  std::vector<Foo> v = {{0}, {1}, {2}, {3}, {4}};

  auto it = MakeIteratorForMember<&Foo::i>(v.begin());
  auto it_end = MakeIteratorForMember<&Foo::i>(v.end());

  // Test non-const lvalue reference.
  EXPECT_TRUE((std::is_same_v<decltype(*it), int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  // Test rvalue reference.
  EXPECT_TRUE((std::is_same_v<decltype(*MakeIteratorForMember<&Foo::i>(
                                  std::make_move_iterator(v.begin()))),
                              int&&>));
  EXPECT_TRUE(
      std::is_rvalue_reference_v<decltype(*MakeIteratorForMember<&Foo::i>(
          std::make_move_iterator(v.begin())))>);
  EXPECT_FALSE(
      std::is_lvalue_reference_v<decltype(*MakeIteratorForMember<&Foo::i>(
          std::make_move_iterator(v.begin())))>);

  // Test const lvalue reference.
  EXPECT_TRUE(
      (std::is_same_v<decltype(*MakeIteratorForMember<&Foo::i>(v.cbegin())),
                      const int&>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(
                   *MakeIteratorForMember<&Foo::i>(v.cbegin()))>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(
                  *MakeIteratorForMember<&Foo::i>(v.cbegin()))>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(*it, 0);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(it[2], 2);
  EXPECT_EQ(it[3], 3);
  EXPECT_EQ(it[4], 4);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), 2);
  EXPECT_EQ(*(3 + it), 3);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  int sum = 0;
  for (int i : RangeOfMember<&Foo::i>(v)) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);

  sum = 0;
  for (int i : RangeOfMember<&Foo::i>(v.begin(), v.end())) {
    sum += i;
  }
  EXPECT_EQ(sum, 10);
}

TEST(TransformIteratorTest, InclusiveRangeOfEnumValues) {
  enum AllValues {
    kZero = 0,
    kOne,
    kTwo,
    kThree,
    kFour,
  };

  auto enum_range = InclusiveRangeOfEnumValues(kZero, kFour);
  auto it = std::begin(enum_range);
  auto it_end = std::end(enum_range);

  EXPECT_TRUE((std::is_same_v<decltype(*it), AllValues>));
  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(*it, kZero);
  EXPECT_EQ(it[0], kZero);
  EXPECT_EQ(it[1], kOne);
  EXPECT_EQ(it[2], kTwo);
  EXPECT_EQ(it[3], kThree);
  EXPECT_EQ(it[4], kFour);

  ++it;
  EXPECT_EQ(*it, kOne);
  EXPECT_EQ(*(it++), kOne);
  EXPECT_EQ(*it, kTwo);

  --it;
  EXPECT_EQ(*it, kOne);
  EXPECT_EQ(*(it--), kOne);
  EXPECT_EQ(*it, kZero);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), kTwo);
  EXPECT_EQ(*(3 + it), kThree);

  it += 5;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, kZero);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

}  // namespace
}  // namespace genit
