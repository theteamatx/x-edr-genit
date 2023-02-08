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


#include "genit/bitset_iterator.h"

#include <forward_list>
#include <ios>
#include <iterator>
#include <sstream>

#include "genit/filter_iterator.h"
#include "genit/iterator_range.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

template <typename Traits>
class AllSetBitIndicesTest : public testing::Test {
 public:
};
TYPED_TEST_SUITE_P(AllSetBitIndicesTest);

TYPED_TEST_P(AllSetBitIndicesTest, BitIndexToBitsetShift) {
  TypeParam bitmask = 1;
  BitIndexToBitset<TypeParam> functor;
  for (unsigned u = 0; u < sizeof(TypeParam); u += 1) {
    EXPECT_EQ(functor(u), bitmask);
    bitmask *= 2;
  }
}

TYPED_TEST_P(AllSetBitIndicesTest, Iterator) {
  TypeParam vals_aa = 0x0;
  TypeParam vals_55 = 0x0;
  TypeParam vals_ff = 0x0;
  for (unsigned u = 0; u < sizeof(TypeParam); u += 1) {
    vals_aa |= TypeParam(0xaa) << u * CHAR_BIT;
    vals_55 |= TypeParam(0x55) << u * CHAR_BIT;
    vals_ff |= TypeParam(0xff) << u * CHAR_BIT;
  }
  // Sanity check generation of test values.
  EXPECT_EQ(vals_ff, std::numeric_limits<TypeParam>::max());
  std::vector<TypeParam> test_set = {0x0, 0x1,     1 << 3,  0x5,
                                     0xA, vals_55, vals_aa, vals_ff};

  for (uint32_t bitset : test_set) {
    uint64_t bitset_check = 0x0;
    for (auto it : AllSetBitIndices<uint32_t>(bitset)) {
      bitset_check |= uint64_t{1} << it;
    }
    EXPECT_EQ(bitset, bitset_check)
        << "bitset= 0x" << std::hex << bitset << " check: 0x" << bitset_check;
  }
}
TYPED_TEST_P(AllSetBitIndicesTest, IterateOverAllBits) {
  constexpr TypeParam kAllOnes = ~TypeParam(0);
  TypeParam expected_iterator(0);
  for (auto it : AllSetBitIndices<TypeParam>(kAllOnes)) {
    EXPECT_EQ(it, expected_iterator);
    expected_iterator++;
  }
}

REGISTER_TYPED_TEST_SUITE_P(AllSetBitIndicesTest, BitIndexToBitsetShift,
                            Iterator, IterateOverAllBits);
typedef ::testing::Types<uint8_t, uint16_t, uint32_t, uint64_t> UnsignedTypes;

INSTANTIATE_TYPED_TEST_SUITE_P(TheAllSetBitIndicesTest, AllSetBitIndicesTest,
                               UnsignedTypes);
}  // namespace
}  // namespace genit
