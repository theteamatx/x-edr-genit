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

#ifndef GENIT_ITERATOR_RANGE_H_
#define GENIT_ITERATOR_RANGE_H_

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

#include "genit/iterator_facade.h"

namespace genit {

namespace iterator_range_detail {

template <typename IteratorT>
inline std::enable_if_t<std::is_pointer_v<IteratorT>, IteratorT>
DefaultIterator() {
  return IteratorT(nullptr);
}

template <typename Left, typename Right>
inline bool Equal(const Left& left, const Right& right) {
  using std::begin;
  using std::end;
  return std::equal(begin(left), end(left), begin(right));
}

// Helper function for converting a range to a begin iterator
template <typename ForwardRange>
static decltype(auto) GetRangeBegin(ForwardRange&& r) {
  using std::begin;
  return begin(std::forward<ForwardRange>(r));
}

// Helper function for converting a range to an end iterator
template <typename ForwardRange>
static decltype(auto) GetRangeEnd(ForwardRange&& r) {
  using std::end;
  return end(std::forward<ForwardRange>(r));
}

}  // End namespace iterator_range_detail

//  An \c IteratorRange delimits a range in a sequence by beginning and ending
//  iterators. An IteratorRange can be passed to an algorithm which requires a
//  sequence as an input.
//
//  Many algorithms working with sequences take a pair of iterators,
//  delimiting a working range, as an arguments. The \c IteratorRange class is
//  an encapsulation of a range identified by a pair of iterators.
// It provides a collection interface, so it is possible to pass an instance
//  to an algorithm requiring a collection as an input.
template <typename IteratorT>
class IteratorRange {
 public:
  // This type
  using this_type = IteratorRange<IteratorT>;

  // Encapsulated value type
  using value_type = typename std::iterator_traits<IteratorT>::value_type;

  // Difference type
  using difference_type =
      typename std::iterator_traits<IteratorT>::difference_type;

  // Size type
  using size_type = std::size_t;

  // Reference type
  // Needed because value_type is the same for const and non-const
  // iterators
  using reference = typename std::iterator_traits<IteratorT>::reference;

  // There is no distinction between const_iterator and iterator.
  // These typedefs are provided to fulfill container interface
  using const_iterator = IteratorT;

  // Iterator type.
  using iterator = IteratorT;

  // Default constructor
  IteratorRange()
      : begin_(iterator_range_detail::DefaultIterator<IteratorT>()),
        end_(iterator_range_detail::DefaultIterator<IteratorT>()) {}

  //  Constructor from a pair of iterators
  template <typename Iterator1, typename Iterator2>
  IteratorRange(Iterator1&& b, Iterator2&& e)
      : begin_(std::forward<Iterator1>(b)), end_(std::forward<Iterator2>(e)) {}

  // Default copy constructor
  IteratorRange(const IteratorRange& copy) = default;

  // Constructor from a Range
  template <typename Iterator>
  IteratorRange(const IteratorRange<Iterator>& r)  // NOLINT
      : begin_(r.begin()), end_(r.end()) {}

  // Constructor from a Range
  template <typename Range>
  explicit IteratorRange(Range&& r)
      : begin_(iterator_range_detail::GetRangeBegin(std::forward<Range>(r))),
        end_(iterator_range_detail::GetRangeEnd(std::forward<Range>(r))) {}

  // Default assignment operator
  IteratorRange& operator=(const IteratorRange& copy) = default;

  // Assignment from an iterator range of another type of iterator
  template <typename Iterator>
  IteratorRange& operator=(const IteratorRange<Iterator>& r) {
    begin_ = r.begin();
    end_ = r.end();
    return *this;
  }

  // Assignment from a const forward range
  template <typename ForwardRange>
  IteratorRange& operator=(ForwardRange&& r) {
    begin_ =
        iterator_range_detail::GetRangeBegin(std::forward<ForwardRange>(r));
    end_ = iterator_range_detail::GetRangeEnd(std::forward<ForwardRange>(r));
    return *this;
  }

  // Return the begin iterator
  IteratorT begin() const { return begin_; }

  // Return the end iterator
  IteratorT end() const { return end_; }

  // Return the size of the range
  difference_type size() const { return std::distance(begin_, end_); }

  // Return whether the range is empty
  bool empty() const { return begin_ == end_; }

  // Cast the range to a bool, so it can be used in conditionals
  explicit operator bool() const { return begin_ != end_; }

  // Returns the front of the non-empty range
  reference front() const { return *begin_; }

  // Return the element in the "at" position of this range.
  reference operator[](difference_type at) const { return begin_[at]; }

 private:
  // begin and end iterators
  IteratorT begin_;
  IteratorT end_;
};

// Pointer range alias
template <typename T>
using PtrRange = IteratorRange<T*>;

template <typename Range>
using RangeIteratorType =
    std::decay_t<decltype(iterator_range_detail::GetRangeBegin(
        std::declval<Range>()))>;

template <typename Range>
using RangeValueType =
    typename std::iterator_traits<RangeIteratorType<Range>>::value_type;

template <typename Range>
using RangeReferenceType =
    typename std::iterator_traits<RangeIteratorType<Range>>::reference;

// Replacement for std::decay_t to handle special cases:
//  T(&)[N] decays to T*, replaced by PtrRange<T>
template <typename UndecayedRange>
using RangeDecayType =
    std::conditional_t<std::is_pointer_v<std::decay_t<UndecayedRange>>,
                       IteratorRange<std::decay_t<UndecayedRange>>,
                       std::decay_t<UndecayedRange>>;

// Deduction guide
template <typename Iterator>
IteratorRange(Iterator&& b, Iterator&& e)
    -> IteratorRange<std::decay_t<Iterator>>;

// Deduction guide
template <typename Range>
IteratorRange(Range&& r) -> IteratorRange<RangeIteratorType<Range>>;

template <typename Iterator1T, typename Iterator2T>
inline bool operator==(const IteratorRange<Iterator1T>& left,
                       const IteratorRange<Iterator2T>& right) {
  return iterator_range_detail::Equal(left, right);
}

// Construct an IteratorRange from two iterators
template <typename IteratorT>
inline auto MakeIteratorRange(IteratorT&& b, IteratorT&& e) {
  return IteratorRange<std::decay_t<IteratorT>>(std::forward<IteratorT>(b),
                                                std::forward<IteratorT>(e));
}

// Construct an IteratorRange from a std::pair of iterators
template <typename IteratorT>
inline auto MakeIteratorRangeFromPair(
    std::pair<IteratorT, IteratorT>&& iter_pair) {
  return IteratorRange<std::decay_t<IteratorT>>(std::move(iter_pair.first),
                                                std::move(iter_pair.second));
}

// Construct an IteratorRange from a Range containing the begin
// and end iterators.
template <typename ForwardRange>
inline auto MakeIteratorRange(ForwardRange&& r) {
  using std::begin;
  return IteratorRange<decltype(begin(std::forward<ForwardRange>(r)))>(r);
}

// Befriend this class in the implementation of a Derived class to be used
// with AliasRangeFacade (see below).
template <typename Derived>
class AliasRangeFacadePrivateAccess {
 public:
  // Implementation of the IteratorFacade requirements:
  template <typename BaseRange>
  static auto Begin(const Derived& lhs, const BaseRange& base_range) {
    return lhs.Begin(base_range);
  }
  template <typename BaseRange>
  static auto End(const Derived& lhs, const BaseRange& base_range) {
    return lhs.End(base_range);
  }
};

// AliasRangeFacade facilitates the creation of ranges by storing a range from
// which begin and end iterators can be obtained via a derived type (i.e., CRTP:
// curiously recurring template pattern).
//
// It provides a collection interface, so it is possible to pass an instance
//  to an algorithm requiring a collection as an input.
template <typename Derived, typename BaseRange,
          typename IteratorT = RangeIteratorType<BaseRange>>
class AliasRangeFacade {
 public:
  // This type
  using this_type = AliasRangeFacade<Derived, BaseRange, IteratorT>;

  // Iterator type.
  using iterator = IteratorT;

  // There is no distinction between const_iterator and iterator.
  // These typedefs are provided to fulfill container interface
  using const_iterator = iterator;

  // Encapsulated value type
  using value_type = typename std::iterator_traits<iterator>::value_type;

  // Difference type
  using difference_type =
      typename std::iterator_traits<iterator>::difference_type;

  // Size type
  using size_type = std::size_t;

  // Reference type
  // Needed because value_type is the same for const and non-const
  // iterators
  using reference = typename std::iterator_traits<iterator>::reference;

  // Constructor from a Range
  template <typename OtherRange>
  explicit AliasRangeFacade(OtherRange&& r)
      : base_range_(std::forward<OtherRange>(r)) {}

  // Default assignment operator
  AliasRangeFacade& operator=(const AliasRangeFacade&) = default;
  AliasRangeFacade& operator=(AliasRangeFacade&&) = default;
  AliasRangeFacade(const AliasRangeFacade&) = default;
  AliasRangeFacade(AliasRangeFacade&&) = default;

  // Return the begin iterator
  auto begin() const {
    return AliasRangeFacadePrivateAccess<Derived>::Begin(
        static_cast<const Derived&>(*this), base_range_);
  }

  // Return the end iterator
  auto end() const {
    return AliasRangeFacadePrivateAccess<Derived>::End(
        static_cast<const Derived&>(*this), base_range_);
  }

  // Return the size of the range
  difference_type size() const { return std::distance(begin(), end()); }

  // Return whether the range is empty
  bool empty() const { return begin() == end(); }

  // Cast the range to a bool, so it can be used in conditionals
  explicit operator bool() const { return begin() != end(); }

  // Returns the front of the non-empty range
  reference front() const { return *begin(); }

  // Return the element in the "at" position of this range.
  reference operator[](difference_type at) const {
    return *std::advance(begin(), at);
  }

 protected:
  // base range
  BaseRange base_range_;
};

template <typename Derived1, typename BaseRange1, typename IteratorT1,
          typename Derived2, typename BaseRange2, typename IteratorT2>
inline bool operator==(
    const AliasRangeFacade<Derived1, BaseRange1, IteratorT1>& left,
    const AliasRangeFacade<Derived2, BaseRange2, IteratorT2>& right) {
  return iterator_range_detail::Equal(left, right);
}

// WrappedRange is just a wrapper around a range, stores it by value (copy), and
// provides a collection and range interface, so it is possible to pass an
// instance to an algorithm requiring a collection as an input.
template <typename BaseRange>
class WrappedRange
    : public AliasRangeFacade<WrappedRange<BaseRange>, BaseRange> {
 public:
  using AliasRangeFacade<WrappedRange<BaseRange>, BaseRange>::AliasRangeFacade;

 private:
  friend class AliasRangeFacadePrivateAccess<WrappedRange<BaseRange>>;

  auto Begin(const BaseRange& base_range) const {
    using std::begin;
    return begin(base_range);
  }
  auto End(const BaseRange& base_range) const {
    using std::end;
    return end(base_range);
  }
};

// Construct a wrapped range from a given range.
template <typename Range>
inline auto WrapRange(Range&& r) {
  return WrappedRange<RangeDecayType<Range>>(std::forward<Range>(r));
}

// Move or alias a given range.
// Move rvalue ranges, but alias lvalue ranges (cf. ref-wrapper).
template <typename Range>
inline RangeDecayType<Range> MoveOrAliasRange(Range&& r) {
  return std::move(r);
}
template <typename Range>
inline RangeDecayType<Range> MoveOrAliasRange(const Range&& r) {
  return std::move(r);
}
template <typename Range>
inline auto MoveOrAliasRange(Range& r) {
  return MakeIteratorRange(r);
}
template <typename Range>
inline auto MoveOrAliasRange(const Range& r) {
  return MakeIteratorRange(r);
}

// ReversedRange wraps a range and reverses the iterators.
template <typename BaseRange>
class ReversedRange : public AliasRangeFacade<
                          ReversedRange<BaseRange>, BaseRange,
                          std::reverse_iterator<RangeIteratorType<BaseRange>>> {
 public:
  using RevIter = std::reverse_iterator<RangeIteratorType<BaseRange>>;
  using AliasRangeFacade<ReversedRange<BaseRange>, BaseRange,
                         RevIter>::AliasRangeFacade;

 private:
  friend class AliasRangeFacadePrivateAccess<ReversedRange<BaseRange>>;

  auto Begin(const BaseRange& base_range) const {
    using std::end;
    return RevIter(end(base_range));
  }
  auto End(const BaseRange& base_range) const {
    using std::begin;
    return RevIter(begin(base_range));
  }
};

// Construct a reversed range from a given range.
template <typename Range>
inline auto ReverseRange(Range&& r) {
  return ReversedRange<decltype(MoveOrAliasRange(std::forward<Range>(r)))>(
      MoveOrAliasRange(std::forward<Range>(r)));
}

// Construct a new sequence of the specified type from the elements
// in the given range
template <typename Sequence, typename Range>
inline Sequence CopyRange(Range&& r) {
  using std::begin;
  using std::end;
  return Sequence(begin(std::forward<Range>(r)), end(std::forward<Range>(r)));
}

// A range of consecutive integers.
inline auto IndexRange(int b, int e) {
  return IteratorRange<IndexIterator>(b < e ? b : e, e);
}

}  // namespace genit

#endif  // GENIT_ITERATOR_RANGE_H_
