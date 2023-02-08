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

#include "genit/iterator_range.h"

#include <map>
#include <type_traits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

TEST(IteratorRange, WholeVector) {
  std::vector<int> v = {2, 3, 5, 7, 11, 13};
  IteratorRange<std::vector<int>::iterator> range(v.begin(), v.end());
  EXPECT_THAT(range, ElementsAreArray(v));
}

TEST(IteratorRange, WholeVectorAsRange) {
  std::vector<int> v = {2, 3, 5, 7, 11, 13};
  IteratorRange<std::vector<int>::iterator> range(v);
  EXPECT_THAT(range, ElementsAreArray(v));
}

TEST(IteratorRange, VectorMakeRange) {
  std::vector<int> v = {2, 3, 5, 7, 11, 13};
  EXPECT_THAT(MakeIteratorRange(v.begin(), v.end()), ElementsAreArray(v));
  EXPECT_THAT(IteratorRange(v.begin(), v.end()), ElementsAreArray(v));
  EXPECT_THAT(IteratorRange(v), ElementsAreArray(v));
}

TEST(IteratorRange, WholeArray) {
  int v[] = {2, 3, 5, 7, 11, 13};
  IteratorRange<int*> range(v);
  EXPECT_THAT(range, ElementsAre(2, 3, 5, 7, 11, 13));
  EXPECT_THAT(IteratorRange(std::begin(v), std::end(v)), ElementsAreArray(v));
  EXPECT_THAT(IteratorRange(v), ElementsAreArray(v));
}

TEST(IteratorRange, PartArray) {
  int v[] = {2, 3, 5, 7, 11, 13};
  IteratorRange<int*> range(&v[1], &v[4]);  // 3, 5, 7
  EXPECT_THAT(range, ElementsAre(3, 5, 7));
  EXPECT_THAT(IteratorRange(&v[1], &v[4]), ElementsAre(3, 5, 7));
}

TEST(IteratorRange, WholeArrayMakeRange) {
  int v[] = {2, 3, 5, 7, 11, 13};
  EXPECT_THAT(MakeIteratorRange(v), ElementsAre(2, 3, 5, 7, 11, 13));
}

TEST(IteratorRange, WholeArrayReverseRange) {
  int v[] = {2, 3, 5, 7, 11, 13};
  EXPECT_THAT(ReverseRange(v), ElementsAre(13, 11, 7, 5, 3, 2));
}

TEST(IteratorRange, ArrayMakeRange) {
  int v[] = {2, 3, 5, 7, 11, 13};
  EXPECT_THAT(MakeIteratorRange(&v[1], &v[4]), ElementsAre(3, 5, 7));
}

TEST(IteratorRange, MakeIteratorRangeFromPair) {
  int v[] = {2, 3, 5, 7, 11, 13};
  EXPECT_THAT(MakeIteratorRangeFromPair(std::make_pair(&v[1], &v[4])),
              ElementsAre(3, 5, 7));
}

TEST(IteratorRange, MakeIteratorRangeFromPairMultimap) {
  std::multimap<int, int> m = {{2, 3}, {2, 5}, {3, 5}, {3, 7}, {5, 11}};
  EXPECT_THAT(MakeIteratorRangeFromPair(m.equal_range(3)),
              UnorderedElementsAre(Pair(3, 5), Pair(3, 7)));
}

TEST(IteratorRange, IteratorMemberTypeIsSameAsOnePassedIn) {
  std::vector<int> v = {0, 2, 3, 4};
  static_assert(::std::is_same<typename decltype(MakeIteratorRange(
                                   v.begin(), v.end()))::iterator,
                               std::vector<int>::iterator>::value,
                "IteratorRange::iterator type is not the same as the type of "
                "the underlying iterator");
}

TEST(IndexRange, NonEmptyRange) {
  auto range = IndexRange(3, 6);
  EXPECT_THAT(range, ElementsAre(3, 4, 5));
}

TEST(IndexRange, EmptyRange) {
  auto range = IndexRange(6, 3);
  EXPECT_THAT(range, ElementsAre());
}

}  // namespace
}  // namespace genit
