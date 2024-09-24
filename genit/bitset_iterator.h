/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GENIT_BITSET_ITERATOR_H_
#define GENIT_BITSET_ITERATOR_H_

#include <climits>
#include <type_traits>

#include "genit/filter_iterator.h"
#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

// Functor that converts a bit index to a bitset.
template <typename IntType>
struct BitIndexToBitset {
  IntType operator()(int index) const {
    using UnsignedType = std::make_unsigned_t<IntType>;
    return IntType(UnsignedType(1) << UnsignedType(index));
  }
};

// Predicate that returns true if the bit at operator(index) is set in bitset;
template <typename IntType>
struct SelectSetBits {
  IntType bitset;
  bool operator()(int index) const {
    static_assert(std::is_unsigned<IntType>::value,
                  "SelectSetBits only works for unsigned integer types.");
    return (IntType(1) << index) & bitset;
  }
};

template <typename IntType>
using SetBitIndices = FilterIterator<IndexIterator, SelectSetBits<IntType>>;

// Creates an iterator range that will iterate over all hi bits in bitset.
template <typename IntType>
auto AllSetBitIndices(IntType bitset) {
  return FilterRange(
      MakeIteratorRange(IndexIterator(0),
                        IndexIterator(sizeof(IntType) * CHAR_BIT)),
      SelectSetBits<IntType>{bitset});
}

}  // namespace genit

#endif  // GENIT_BITSET_ITERATOR_H_
