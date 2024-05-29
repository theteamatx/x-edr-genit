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
#include "iterator_range.h"

namespace genit {

template <typename BaseRange>
class CircularIterator
    : public IteratorFacade<
          CircularIterator<BaseRange>,
          decltype(*std::declval<RangeIteratorType<BaseRange>>()),
          typename std::iterator_traits<
              RangeIteratorType<BaseRange>>::iterator_category> {
 public:
  // Constructs an  iterator from an underlying iterator range.
  // The winding number specifies which turn this corresponds to. So, in the
  // usual case, if you want to iterate one turn around
  // the range [first, last), you would create iter(first, last, 0) for begin
  // and iter(first, last, 1) for end.
  explicit CircularIterator(const BaseRange* base_range, int winding = 0)
      : base_iter_(base_range->begin()),
        base_range_(base_range),
        winding_(winding) {
    // For empty range, normalize winding number to 0 to simplify comparison.
    if (BaseBegin() == BaseEnd()) {
      winding_ = 0;
    }
  }

 private:
  // The following functions implement the requirements of IteratorFacade.

  friend class IteratorFacadePrivateAccess<CircularIterator>;

  using BaseIter = RangeIteratorType<BaseRange>;
  using Reference = decltype(*std::declval<BaseIter>());

  BaseIter BaseBegin() const {
    using std::begin;
    return begin(*base_range_);
  }
  BaseIter BaseEnd() const {
    using std::end;
    return end(*base_range_);
  }

  Reference Dereference() const { return *base_iter_; }
  void Increment() {
    ++base_iter_;
    if (base_iter_ == BaseEnd()) {
      base_iter_ = BaseBegin();
      ++winding_;
    }
  }
  void Decrement() {
    if (base_iter_ == BaseBegin()) {
      --winding_;
      base_iter_ = BaseEnd();
    }
    --base_iter_;
  }
  bool IsEqual(const CircularIterator& rhs) const {
    return base_iter_ == rhs.base_iter_ && winding_ == rhs.winding_;
  }
  int DistanceTo(const CircularIterator& rhs) const {
    return (rhs.base_iter_ - base_iter_) +
           (rhs.winding_ - winding_) * (BaseEnd() - BaseBegin());
  }
  void Advance(int n) {
    base_iter_ += n;
    const int distance_from_begin = base_iter_ - BaseBegin();
    const int range_size = BaseEnd() - BaseBegin();
    // Apply % twice to allow for negative distances.
    base_iter_ =
        BaseBegin() +
        (((distance_from_begin % range_size) + range_size) % range_size);
    winding_ += distance_from_begin / range_size;
  }

  // Stores the set of  iterators.
  BaseIter base_iter_;
  const BaseRange* base_range_;
  int winding_;
};

// This class template implements a range which allows wrapping around the
// underlying range for a specified number of times.
//
// BaseRange is the type of the underlying range to circle over.
template <typename BaseRange>
class CircularRangeT
    : public AliasRangeFacade<CircularRangeT<BaseRange>, BaseRange,
                              CircularIterator<BaseRange>> {
 public:
  using CircIter = CircularIterator<BaseRange>;
  using BaseFacade =
      AliasRangeFacade<CircularRangeT<BaseRange>, BaseRange, CircIter>;

  // Constructor from a Range
  template <typename OtherRange>
  explicit CircularRangeT(OtherRange&& r, int windings, bool connect)
      : BaseFacade(std::forward<OtherRange>(r)),
        windings_(windings),
        connect_(connect) {}

  // Default assignment operator
  CircularRangeT& operator=(const CircularRangeT&) = default;
  CircularRangeT& operator=(CircularRangeT&&) = default;
  CircularRangeT(const CircularRangeT&) = default;
  CircularRangeT(CircularRangeT&&) = default;

 private:
  friend class AliasRangeFacadePrivateAccess<CircularRangeT<BaseRange>>;

  auto Begin(const BaseRange& base_range) const {
    return CircIter(&base_range, 0);
  }
  auto End(const BaseRange& base_range) const {
    auto e = CircIter(&base_range, windings_);
    if (connect_ && e != Begin(base_range)) {
      ++e;
    }
    return e;
  }

  int windings_ = 1;
  bool connect_ = false;
};

// This convenient wrapper function produces a range of circular iterators from
// a given range, wrapping around windings number of times. See CircularIterator
// above.
template <typename Range>
auto CircularRange(Range&& range, int windings = 1) {
  assert(windings >= 0);
  return CircularRangeT<decltype(MoveOrAliasRange(std::forward<Range>(range)))>(
      MoveOrAliasRange(std::forward<Range>(range)), windings,
      /*connect=*/false);
}

// This convenient wrapper function produces a range of  circular iterators from
// a given set of begin / end iterators, wrapping around the original range
// windings number of times. See CircularIterator above.
template <typename Iter>
auto CircularRange(const Iter& first, const Iter& last, int windings = 1) {
  assert(windings >= 0);
  return CircularRange(MakeIteratorRange(first, last), windings);
}

// This convenient wrapper function produces a range of circular iterators from
// a given range, wrapping around the original range to repeat just the first
// element. This is useful, for example, when working with closed polylines.
//
// Range [x0, x1, ..., xn] -> Range [x0, x1, ..., xn, x0].
template <typename Range>
auto CircularConnectRange(Range&& range, int windings = 1) {
  assert(windings >= 0);
  return CircularRangeT<decltype(MoveOrAliasRange(std::forward<Range>(range)))>(
      MoveOrAliasRange(std::forward<Range>(range)), windings, /*connect=*/true);
}

// This convenient wrapper function produces a range of  circular iterators from
// a given set of begin / end iterators, wrapping around the original range to
// repeat just the first element. This is useful, for example, when working with
// closed polylines.
//
// Range [x0, x1, ..., xn] -> Range [x0, x1, ..., xn, x0].
template <typename Iter>
auto CircularConnectRange(const Iter& first, const Iter& last,
                          int windings = 1) {
  return CircularConnectRange(MakeIteratorRange(first, last), windings);
}

}  // namespace genit

#endif  // GENIT_CIRCULAR_ITERATOR_H_
