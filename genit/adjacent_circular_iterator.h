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


// This header file provides a class template (AdjacentCircularIterator) which
// implements a mechanism almost identical to AdjacentIterator except that it
// allows for wrapping around the range, i.e., a "circular" iterator range.
//
// Example:
//
// std::vector<double> FiniteDifference(const std::vector<double>& data) {
//   std::vector<double> result;
//   for (auto& data_pair : AdjacentElementsCircularRange<2>(data)) {
//     result.push_back(data_pair[1] - data_pair[0]);
//   }
//   return result;
// }
//
// As the example above shows, convenient type-deduced wrappers exist to create
// a range of adjacent iterators (AdjacentElementsCircularRange) which can
// take either a range (as in a range-based for-loop) or two iterators
// (begin and end).

#ifndef GENIT_ADJACENT_CIRCULAR_ITERATOR_H_
#define GENIT_ADJACENT_CIRCULAR_ITERATOR_H_

#include <iterator>
#include <type_traits>

#include "genit/adjacent_iterator.h"
#include "genit/iterator_facade.h"

namespace genit {

// This class template implements an adjacent iterator similar to
// AdjacentIterator (see adjacent_iterator.h), but allows wrapping around the
// range for a specified number of times.
//
// UnderlyingIter is the type of the underlying iterator that the adjacent
// iterator uses to iterate the range.
// N is the number of adjacent elements to package together, or size of the
// window of data being traversed.
template <typename UnderlyingIter, int N>
class AdjacentCircularIterator
    : public IteratorFacade<
          AdjacentCircularIterator<UnderlyingIter, N>,
          adjacent_iterator_internal::ValueArrayProxy<UnderlyingIter, N>,
          typename std::iterator_traits<UnderlyingIter>::iterator_category> {
 public:
  // Constructs an adjacent iterator from an underlying iterator range.
  // The winding number specifies which turn this corresponds to. So, in the
  // usual case, if you want to iterate one turn around
  // the range [first, last), you would create iter(first, last, 0) for begin
  // and iter(first, last, 1) for end.
  AdjacentCircularIterator(const UnderlyingIter& it,
                           const UnderlyingIter& it_end, int winding = 0)
      : base_iter_(it, it_end),
        it_begin_(it),
        it_end_(it_end),
        winding_(winding) {
    // Check for a range smaller than N:
    for (auto& it : base_iter_.iterators_) {
      if (it == it_end_) {
        winding_ = 666000;  // Identifiable number to indicate undefined range.
        break;
      }
    }
  }

 private:
  // The following functions implement the requirements of IteratorFacade.

  friend class IteratorFacadePrivateAccess<AdjacentCircularIterator>;

  using ProxyType = typename AdjacentIterator<UnderlyingIter, N>::ProxyType;

  const UnderlyingIter& FrontIterator() const {
    return base_iter_.iterators_[base_iter_.offset_];
  }

  ProxyType Dereference() const { return base_iter_.Dereference(); }
  void Increment() {
    // Duplicate leading iterator, increment it, and move offset.
    const int trailing_id = (base_iter_.offset_ + N - 1) % N;
    base_iter_.iterators_[base_iter_.offset_] =
        base_iter_.iterators_[trailing_id];
    ++base_iter_.iterators_[base_iter_.offset_];
    if (base_iter_.iterators_[base_iter_.offset_] == it_end_) {
      base_iter_.iterators_[base_iter_.offset_] = it_begin_;
      // The winding is incremented every time the trailing iterator is wrapped.
      ++winding_;
    }
    base_iter_.offset_ = (base_iter_.offset_ + 1) % N;
  }
  void Decrement() {
    // Duplicate trailing iterator, decrement it, and move offset.
    const int trailing_id = (base_iter_.offset_ + N - 1) % N;
    base_iter_.iterators_[trailing_id] =
        base_iter_.iterators_[base_iter_.offset_];
    if (base_iter_.iterators_[trailing_id] == it_begin_) {
      base_iter_.iterators_[trailing_id] = it_end_;
      // The winding is decremented every time the lead iterator is wrapped.
      --winding_;
    }
    --base_iter_.iterators_[trailing_id];
    base_iter_.offset_ = trailing_id;
  }
  bool IsEqual(const AdjacentCircularIterator& rhs) const {
    return FrontIterator() == rhs.FrontIterator() && winding_ == rhs.winding_;
  }
  int DistanceTo(const AdjacentCircularIterator& rhs) const {
    return rhs.FrontIterator() - FrontIterator() +
           (rhs.winding_ - winding_) * (it_end_ - it_begin_);
  }
  void Advance(int n) {
    for (auto& iter : base_iter_.iterators_) {
      iter += n;
      while (iter >= it_end_) {
        iter = (iter - it_end_) + it_begin_;
        if (&iter == &base_iter_.BackIterator()) {
          // Only increment winding if this is the trailing iterator.
          ++winding_;
        }
      }
      while (iter < it_begin_) {
        iter = it_end_ - (it_begin_ - iter);
        if (&iter == &FrontIterator()) {
          // Only decrement winding if this is the lead iterator.
          --winding_;
        }
      }
    }
  }

  // Stores the set of adjacent iterators.
  AdjacentIterator<UnderlyingIter, N> base_iter_;
  UnderlyingIter it_begin_;
  UnderlyingIter it_end_;
  int winding_;
};

// This convenient wrapper function produces a range of adjacent circular
// iterators from a given range. See AdjacentCircularIterator above.
//
// Calling this function with a range whose size is smaller than N will
// return an empty range.
template <int N, typename Range>
auto AdjacentElementsCircularRange(Range&& range) {
  static_assert(N > 0,
                "Number of adjacent elements must be greater than zero!");
  using std::begin;
  using std::end;
  using AdjIter = AdjacentCircularIterator<decltype(begin(range)), N>;
  return IteratorRange(AdjIter(begin(range), end(range)),
                       AdjIter(begin(range), end(range), 1));
}

// This convenient wrapper function produces a range of adjacent circular
// iterators from a given set of begin / end iterators.
// See AdjacentCircularIterator above.
//
// Calling this function with a range whose size is smaller than N will
// return an empty range.
template <int N, typename Iter>
auto AdjacentElementsCircularRange(const Iter& first, const Iter& last) {
  static_assert(N > 0,
                "Number of adjacent elements must be greater than zero!");
  using AdjIter = AdjacentCircularIterator<std::decay_t<Iter>, N>;
  return IteratorRange(AdjIter(first, last), AdjIter(first, last, 1));
}

}  // namespace genit

#endif  // GENIT_ADJACENT_CIRCULAR_ITERATOR_H_
