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

// This header file provides a class template (CircularIterator) which
// implements a mechanism almost identical to Iterator except that it
// allows for wrapping around the range, i.e., a "circular" iterator range.
//
// Example:
//
// void PrintFromLowest(const std::vector<double>& data) {
//   const auto range = CircularRange(data);
//   const auto start = std::min_element(range.begin(), range.end());
//   const auto end = start + range.size();
//   for (auto it = start; it != end; ++it) {
//     std::cout << element << ", ";
//   }
// }
//
// As the example above shows, convenient type-deduced wrappers exist to create
// a range of  iterators (CircularRange) which can take either a range
// (as in a range-based for-loop) or two iterators (begin and end).

#ifndef GENIT_CIRCULAR_ITERATOR_H_
#define GENIT_CIRCULAR_ITERATOR_H_

#include <cassert>
#include <iterator>
#include <type_traits>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

// This class template implements an iterator which allows wrapping around the
// range for a specified number of times.
//
// UnderlyingIter is the type of the underlying iterator that the
// iterator uses to iterate the range.
template <typename UnderlyingIter>
class CircularIterator
    : public IteratorFacade<
          CircularIterator<UnderlyingIter>,
          decltype(*std::declval<UnderlyingIter>()),
          typename std::iterator_traits<UnderlyingIter>::iterator_category> {
 public:
  // Constructs an  iterator from an underlying iterator range.
  // The winding number specifies which turn this corresponds to. So, in the
  // usual case, if you want to iterate one turn around
  // the range [first, last), you would create iter(first, last, 0) for begin
  // and iter(first, last, 1) for end.
  CircularIterator(const UnderlyingIter& it, const UnderlyingIter& it_end,
                   int winding = 0)
      : base_iter_(it), it_begin_(it), it_end_(it_end), winding_(winding) {
    // For empty range, normalize winding number to 0 to simplify comparison.
    if (it_begin_ == it_end_) {
      winding_ = 0;
    }
  }

 private:
  // The following functions implement the requirements of IteratorFacade.

  friend class IteratorFacadePrivateAccess<CircularIterator>;

  using Reference = decltype(*std::declval<UnderlyingIter>());

  Reference Dereference() const { return *base_iter_; }
  void Increment() {
    ++base_iter_;
    if (base_iter_ == it_end_) {
      base_iter_ = it_begin_;
      ++winding_;
    }
  }
  void Decrement() {
    if (base_iter_ == it_begin_) {
      --winding_;
      base_iter_ = it_end_;
    }
    --base_iter_;
  }
  bool IsEqual(const CircularIterator& rhs) const {
    return base_iter_ == rhs.base_iter_ && winding_ == rhs.winding_;
  }
  int DistanceTo(const CircularIterator& rhs) const {
    return (rhs.base_iter_ - base_iter_) +
           (rhs.winding_ - winding_) * (it_end_ - it_begin_);
  }
  void Advance(int n) {
    base_iter_ += n;
    const int distance_from_begin = base_iter_ - it_begin_;
    const int range_size = it_end_ - it_begin_;
    // Apply % twice to allow for negative distances.
    base_iter_ =
        it_begin_ +
        (((distance_from_begin % range_size) + range_size) % range_size);
    winding_ += distance_from_begin / range_size;
  }

  // Stores the set of  iterators.
  UnderlyingIter base_iter_;
  UnderlyingIter it_begin_;
  UnderlyingIter it_end_;
  int winding_;
};  // namespace utils

// Deduction guide
template <typename Iter>
CircularIterator(Iter&& begin, Iter&& end)
    -> CircularIterator<std::decay_t<Iter>>;

// This convenient wrapper function produces a range of circular iterators from
// a given range, wrapping around windings number of times. See CircularIterator
// above.
template <typename Range>
auto CircularRange(Range&& range, int windings = 1) {
  assert(windings >= 0);
  using std::begin;
  using std::end;
  return IteratorRange(CircularIterator(begin(range), end(range)),
                       CircularIterator(begin(range), end(range), windings));
}

// This convenient wrapper function produces a range of  circular iterators from
// a given set of begin / end iterators, wrapping around the original range
// windings number of times. See CircularIterator above.
template <typename Iter>
auto CircularRange(const Iter& first, const Iter& last, int windings = 1) {
  assert(windings >= 0);
  return IteratorRange(CircularIterator(first, last),
                       CircularIterator(first, last, windings));
}

// This convenient wrapper function produces a range of circular iterators from
// a given range, wrapping around the original range to repeat just the first
// element. This is useful, for example, when working with closed polylines.
//
// Range [x0, x1, ..., xn] -> Range [x0, x1, ..., xn, x0].
template <typename Range>
auto CircularConnectRange(Range&& range) {
  using std::begin;
  using std::end;
  CircularIterator b(begin(range), end(range));
  CircularIterator e(begin(range), end(range), 1);
  if (b != e) {
    ++e;
  }
  return IteratorRange(std::move(b), std::move(e));
}

// This convenient wrapper function produces a range of  circular iterators from
// a given set of begin / end iterators, wrapping around the original range to
// repeat just the first element. This is useful, for example, when working with
// closed polylines.
//
// Range [x0, x1, ..., xn] -> Range [x0, x1, ..., xn, x0].
template <typename Iter>
auto CircularConnectRange(const Iter& first, const Iter& last) {
  CircularIterator begin(first, last);
  CircularIterator end(first, last, 1);
  if (begin != end) {
    ++end;
  }
  return IteratorRange(begin, end);
}

}  // namespace genit

#endif  // GENIT_CIRCULAR_ITERATOR_H_
