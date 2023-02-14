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

// This header file provides a class template (AdjacentIterator) that can be
// used to transform an iterator into a set of N consecutive iterators packaged
// as one which enables the traversal of a range of elements in groups of N
// consecutive elements. Each increment of an AdjacentIterator iterator
// increments each of its internal iterator by one. In other words, this
// implements a moving window of N elements over the original range.
//
// The general intention of this library is to provide a way to extend the
// benefits of using range-based for-loops over "error-prone" raw for-loops
// to the cases where one wants to look at several adjacent elements of the
// container at once, which is even more error-prone when implemented with
// more basic loop constructs.
//
// Example:
//
// std::vector<double> FiniteDifference(const std::vector<double>& data) {
//   std::vector<double> result;
//   for (const auto data_pair : AdjacentElementsRange<2>(data)) {
//     result.push_back(data_pair[1] - data_pair[0]);
//   }
//   return result;
// }
//
// As the example above shows, convenient type-deduced wrappers exist to create
// a range of adjacent iterators (AdjacentElementsRange) which can take either
// a range (as in a range-based for-loop) or two iterators (begin and end).

#ifndef GENIT_ADJACENT_ITERATOR_H_
#define GENIT_ADJACENT_ITERATOR_H_

#include <array>
#include <iterator>
#include <type_traits>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

namespace adjacent_iterator_internal {

// Below are some implementation details used by the AdjacentIterator class
// template, see below.

// Rvalue iterator version
template <typename UnderlyingIter, int N,
          bool ContainsLValue = std::is_lvalue_reference_v<
              decltype(*std::declval<UnderlyingIter>())>>
struct ValueArrayProxyTraits {
  // Store data as a value and deliver it as a const-reference.
  using Storage = std::decay_t<decltype(*std::declval<UnderlyingIter>())>;
  using Reference = const Storage&;

  static Reference StorageToReference(Reference element) { return element; }
  static Reference ReferenceToStorage(Reference element) { return element; }
};

// Lvalue iterator version
template <typename UnderlyingIter, int N>
struct ValueArrayProxyTraits<UnderlyingIter, N, /*ContainsLValue == */ true> {
  // Store data as a (const-)pointer and deliver it as a (const-)reference.
  using Reference = decltype(*std::declval<UnderlyingIter>());
  using Storage = decltype(&std::declval<Reference>());

  static Reference StorageToReference(Storage element) { return *element; }
  static Storage ReferenceToStorage(Reference element) { return &element; }
};

template <typename UnderlyingIter, int N>
class ValueArrayProxy {
 public:
  using Traits = ValueArrayProxyTraits<UnderlyingIter, N>;
  using Reference = typename Traits::Reference;
  using Storage = typename Traits::Storage;

  // This constructor starts a compile-time recursion
  // to generate an index sequence that is used to unpack the iterators array
  // to get to each element being referenced by them.
  ValueArrayProxy(int offset, const std::array<UnderlyingIter, N>& iterators)
      : ValueArrayProxy{offset, iterators, IndexConstant<N - 1>()} {}

  Reference operator[](int i) const {
    return Traits::StorageToReference(data_[(i + offset_) % N]);
  }

  int size() const { return N; }
  Reference front() const { return (*this)[0]; }
  Reference back() const { return (*this)[N - 1]; }

 private:
  template <int I>
  using IndexConstant = std::integral_constant<int, I>;

  // The variadic template recursion below unwraps indices from 0 to N as
  // a set of integral constants to be able to unpack the dereferenced
  // iterators directly into the static array aggregate initialization to
  // avoid a default-construct-and-assign implementation that would require
  // value-types to be assignable, and potentially less efficient.

  // For more details on this technique, a search for "unpacking tuples"
  // or "index sequence" examples in C++ reveals many articles and tutorials
  // that explain the technique used below.

  // This terminates the index sequence recursion and unpacks the elements
  // referenced by the iterators into the value array.
  template <typename... Idx>
  ValueArrayProxy(int offset, const std::array<UnderlyingIter, N>& iterators,
                  IndexConstant<0> id0, Idx... idx)
      : data_{Traits::ReferenceToStorage(*iterators[0]),
              Traits::ReferenceToStorage(*iterators[Idx::value])...},
        offset_(offset) {}

  // This constructor recursively calls itself with one more index in its
  // index sequence.
  template <int I, typename... Idx>
  ValueArrayProxy(int offset, const std::array<UnderlyingIter, N>& iterators,
                  IndexConstant<I> id, Idx... idx)
      : ValueArrayProxy{offset, iterators, IndexConstant<I - 1>(), id, idx...} {
  }

  std::array<Storage, N> data_;
  int offset_;
};

}  // namespace adjacent_iterator_internal

// Forward-declaration.
template <typename UnderlyingIter, int N>
class AdjacentCircularIterator;

// This class template can be used to transform an iterator into a set of N
// consecutive iterators packaged as one which enables the traversal of a
// range of elements in groups of N consecutive elements. Each increment of
// an AdjacentIterator iterator increments each of its internal iterator by
// one. In other words, this implements a moving window of N elements over the
// original range.
//
// UnderlyingIter is the type of the underlying iterator that the adjacent
// iterator uses to iterate the range.
// N is the number of adjacent elements to package together, or size of the
// window of data being traversed.
template <typename UnderlyingIter, int N>
class AdjacentIterator
    : public IteratorFacade<
          AdjacentIterator<UnderlyingIter, N>,
          adjacent_iterator_internal::ValueArrayProxy<UnderlyingIter, N>,
          typename std::iterator_traits<UnderlyingIter>::iterator_category> {
 public:
  using UnderlyingCategory =
      typename std::iterator_traits<UnderlyingIter>::iterator_category;
  // Adjacent iterators must provide the multi-pass guarantee, otherwise, we
  // cannot maintain several consecutive valid iterators.
  static_assert(
      std::is_convertible_v<UnderlyingCategory, std::forward_iterator_tag>,
      "Underlying iterator type must offer the multi-pass guarantee!");

  // Constructs an adjacent iterator from an underlying iterator.
  // If this is not the "end" iterator, it will fill the array of iterators
  // with consecutive iterators starting at "it".
  // If this is the "end" iterator, it will fill the array of iterators with
  // that iterator only, making it a sentinel.
  AdjacentIterator(const UnderlyingIter& it, const UnderlyingIter& it_end)
      : AdjacentIterator{it, it_end, IndexConstant<N - 1>()} {}

  explicit AdjacentIterator(const UnderlyingIter& it)
      : AdjacentIterator{it, IndexConstant<N - 1>()} {}

 private:
  template <int I>
  using IndexConstant = std::integral_constant<int, I>;

  // The variadic template recursion below unwraps indices from 0 to N as
  // a set of integral constants to be able to unpack the iterators directly
  // into the static array aggregate initialization to avoid a
  // default-construct-and-assign implementation that would require
  // iterators to have a default constructor.

  // For more details on this technique, a search for "unpacking tuples"
  // or "index sequence" examples in C++ reveals many articles and tutorials
  // that explain the technique used below.

  // This is a helper to silence the -Werror=unused-value error when discarding
  // the index values in the recursive constructors below.
  static const UnderlyingIter& DiscardIndex(const UnderlyingIter& it,
                                            int /* discarded */) {
    return it;
  }

  // Recursion to initialize the "end" iterator.
  template <typename... Idx>
  AdjacentIterator(const UnderlyingIter& it, IndexConstant<0> id0, Idx... idx)
      : iterators_{it, DiscardIndex(it, Idx::value)...}, offset_(0) {}

  template <int I, typename... Idx>
  AdjacentIterator(const UnderlyingIter& it, IndexConstant<I> id, Idx... idx)
      : AdjacentIterator{it, IndexConstant<I - 1>(), id, idx...} {}

  // This is a helper to check range and to silence the -Werror=unused-value
  // error when discarding the index values in the recursive constructors below.
  static UnderlyingIter GuardedPostIncrement(UnderlyingIter* it,
                                             const UnderlyingIter& it_end,
                                             int /* discarded */) {
    UnderlyingIter prev_it = *it;
    if (*it != it_end) {
      ++(*it);
    }
    return prev_it;
  }

  // Recursion to initialize a set of consecutive iterators.
  // This relies on the standard guarantee that braced-initializations
  // evaluate arguments in the order that they appear.
  template <typename... Idx>
  AdjacentIterator(UnderlyingIter it, const UnderlyingIter& it_end,
                   IndexConstant<0> id0, Idx... idx)
      : iterators_{GuardedPostIncrement(&it, it_end, 0),
                   GuardedPostIncrement(&it, it_end, Idx::value)...},
        offset_(0) {}

  template <int I, typename... Idx>
  AdjacentIterator(const UnderlyingIter& it, const UnderlyingIter& it_end,
                   IndexConstant<I> id, Idx... idx)
      : AdjacentIterator{it, it_end, IndexConstant<I - 1>(), id, idx...} {}

  // The following functions implement the requirements of IteratorFacade.

  friend class IteratorFacadePrivateAccess<AdjacentIterator>;
  friend class AdjacentCircularIterator<UnderlyingIter, N>;

  using ProxyType =
      adjacent_iterator_internal::ValueArrayProxy<UnderlyingIter, N>;

  const UnderlyingIter& BackIterator() const {
    return iterators_[(offset_ + N - 1) % N];
  }

  ProxyType Dereference() const { return ProxyType(offset_, iterators_); }
  void Increment() {
    // Duplicate leading iterator, increment it, and move offset.
    const int trailing_id = (offset_ + N - 1) % N;
    iterators_[offset_] = iterators_[trailing_id];
    ++iterators_[offset_];
    offset_ = (offset_ + 1) % N;
  }
  void Decrement() {
    // Duplicate trailing iterator, decrement it, and move offset.
    const int trailing_id = (offset_ + N - 1) % N;
    iterators_[trailing_id] = iterators_[offset_];
    --iterators_[trailing_id];
    offset_ = trailing_id;
  }
  bool IsEqual(const AdjacentIterator& rhs) const {
    return rhs.BackIterator() == BackIterator();
  }
  int DistanceTo(const AdjacentIterator& rhs) const {
    return rhs.BackIterator() - BackIterator();
  }
  void Advance(int n) {
    for (auto& iter : iterators_) {
      iter += n;
    }
  }

  // Stores the set of adjacent iterators.
  std::array<UnderlyingIter, N> iterators_;
  int offset_;
};

// This convenient wrapper function produces a range of adjacent iterators
// from a given range. See AdjacentIterator above.
//
// Calling this function with a range whose size is smaller than N will
// return an empty range.
template <int N, typename Range>
auto AdjacentElementsRange(Range&& range) {
  static_assert(N > 0,
                "Number of adjacent elements must be greater than zero!");
  using std::begin;
  using std::end;
  using AdjIter = AdjacentIterator<decltype(begin(range)), N>;
  return IteratorRange(AdjIter(begin(range), end(range)), AdjIter(end(range)));
}

// This convenient wrapper function produces a range of adjacent iterators
// from a given set of begin / end iterators. See AdjacentIterator above.
//
// Calling this function with a range whose size is smaller than N will
// return an empty range.
template <int N, typename Iter>
auto AdjacentElementsRange(const Iter& first, const Iter& last) {
  static_assert(N > 0,
                "Number of adjacent elements must be greater than zero!");
  using AdjIter = AdjacentIterator<std::decay_t<Iter>, N>;
  return IteratorRange(AdjIter(first, last), AdjIter(last));
}

}  // namespace genit

#endif  // GENIT_ADJACENT_ITERATOR_H_
