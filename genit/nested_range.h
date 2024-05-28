/*
 * Copyright 2024 Google LLC
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

#ifndef GENIT_NESTED_RANGE_H_
#define GENIT_NESTED_RANGE_H_

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "absl/utility/utility.h"
#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"
#include "genit/zip_iterator.h"

namespace genit {

// Given a sequence of iterator ranges, nest them to provide a single
// multi-dimensional iterator.
//
// For example,
//
// std::vector<int> v1 = {1,2,3};
// const std::list<int> v2 = {4, 5, 6};
// auto range = NestRanges(v1, v2);
// for (auto [x, y] : range) {
//   // Produces:
//   // x = 1; y = 4;
//   // x = 1; y = 5;
//   // x = 1; y = 6;
//   // x = 2; y = 4;
//   // x = 2; y = 5;
//   // x = 2; y = 6;
//   ...
// }
//

namespace nested_range_detail {

struct BeginTag {};
struct EndTag {};

template <typename... Ranges>
using NestedRangeCategory = zip_iterator_detail::LeastPermissive<
    std::bidirectional_iterator_tag,
    zip_iterator_detail::ReduceToStdIterCategory<typename std::iterator_traits<
        RangeIteratorType<Ranges>>::iterator_category>...>;

template <typename... Ranges>
using InnerRangeCategory = zip_iterator_detail::LeastPermissive<
    std::forward_iterator_tag,
    zip_iterator_detail::ReduceToStdIterCategory<typename std::iterator_traits<
        RangeIteratorType<Ranges>>::iterator_category>...>;

}  // namespace nested_range_detail

template <typename FirstRange, typename... Ranges>
class NestedRange {
  static_assert(
      std::is_convertible_v<nested_range_detail::InnerRangeCategory<Ranges...>,
                            std::forward_iterator_tag>,
      "NestedRange requires inner ranges to be at least forward iterators "
      "(multi-pass).");

public:
  template <typename... OtherRanges>
  explicit NestedRange(OtherRanges&&... ranges)
      : ranges_(std::forward<OtherRanges>(ranges)...) {}

  using OutputRefType =
      std::tuple<decltype(*std::declval<RangeIteratorType<FirstRange>>()),
                 decltype(*std::declval<RangeIteratorType<Ranges>>())...>;
  static constexpr size_t kNumberOfRanges = sizeof...(Ranges) + 1;
  using IndexSeq = absl::make_index_sequence<kNumberOfRanges>;

  // Allows iterating over a NestedRange.
  class NestedIterator
      : public IteratorFacade<
            NestedIterator, OutputRefType,
            nested_range_detail::NestedRangeCategory<
                FirstRange, Ranges...>> {
   public:
    NestedIterator(nested_range_detail::BeginTag,
                   const NestedRange<FirstRange, Ranges...>* nested)
        : nested_(nested) {
      MakeBegin(IndexSeq());
    }
    NestedIterator(nested_range_detail::EndTag,
                   const NestedRange<FirstRange, Ranges...>* nested)
        : nested_(nested) {
      MakeEnd(IndexSeq());
    }

    // Default constructor:
    NestedIterator() : nested_(nullptr), it_tuple_() {}

    NestedIterator(const NestedIterator&) = default;
    NestedIterator(NestedIterator&&) = default;

   private:
    friend class IteratorFacadePrivateAccess<NestedIterator>;

    template <size_t... Ids>
    void MakeBegin(absl::index_sequence<Ids...> ids) {
      using std::begin;
      ((void)(std::get<Ids>(it_tuple_) =
          begin(std::get<Ids>(nested_->ranges_))), ...);
    }

    template <size_t... Ids>
    void MakeEnd(absl::index_sequence<Ids...> ids) {
      using std::begin;
      using std::end;
      ((void)(std::get<Ids>(it_tuple_) =
          (Ids == 0 ?
              end(std::get<Ids>(nested_->ranges_)) :
              begin(std::get<Ids>(nested_->ranges_)))), ...);
    }

    template <size_t... Ids>
    OutputRefType Dereference(absl::index_sequence<Ids...> ids) const {
      return OutputRefType{(*std::get<Ids>(it_tuple_))...};
    }
    OutputRefType Dereference() const { return Dereference(IndexSeq()); }

    template <size_t... Ids>
    bool IsEqual(const NestedIterator& rhs,
                 absl::index_sequence<Ids...> ids) const {
      // Require all iterators to match.
      return ((std::get<Ids>(it_tuple_) == std::get<Ids>(rhs.it_tuple_)) &&
              ...);
    }
    bool IsEqual(const NestedIterator& rhs) const {
      return IsEqual(rhs, IndexSeq());
    }

    template <size_t Id>
    bool IncrementOrWrap() {
      using std::begin;
      using std::end;
      if constexpr (Id == 0) {
        ++std::get<0>(it_tuple_);
        return true;
      } else {
        ++std::get<Id>(it_tuple_);
        if (std::get<Id>(it_tuple_) == end(std::get<Id>(nested_->ranges_))) {
          std::get<Id>(it_tuple_) = begin(std::get<Id>(nested_->ranges_));
          return false;
        }
        return true;
      }
    }
    template <size_t... Ids>
    void Increment(absl::index_sequence<Ids...> ids) {
      (IncrementOrWrap<kNumberOfRanges - Ids - 1>() || ...);
    }
    void Increment() { Increment(IndexSeq()); }

    template <size_t Id>
    bool DecrementOrWrap() {
      using std::begin;
      using std::end;
      if constexpr (Id == 0) {
        --std::get<0>(it_tuple_);
        return true;
      } else {
        if (std::get<Id>(it_tuple_) == begin(std::get<Id>(nested_->ranges_))) {
          std::get<Id>(it_tuple_) = end(std::get<Id>(nested_->ranges_));
          --std::get<Id>(it_tuple_);
          return false;
        }
        --std::get<Id>(it_tuple_);
        return true;
      }
    }
    template <size_t... Ids>
    void Decrement(absl::index_sequence<Ids...> ids) {
      (DecrementOrWrap<kNumberOfRanges - Ids - 1>() || ...);
    }
    void Decrement() { Decrement(IndexSeq()); }

    const NestedRange<FirstRange, Ranges...>* nested_;
    std::tuple<RangeIteratorType<FirstRange>,
               RangeIteratorType<Ranges>...> it_tuple_;
  };
  friend class NestedIterator;

  using iterator = NestedIterator;
  using value_type = typename std::iterator_traits<NestedIterator>::value_type;
  using reference = typename std::iterator_traits<NestedIterator>::reference;

  iterator begin() const {
    return NestedIterator(nested_range_detail::BeginTag{}, this);
  }
  iterator end() const {
    return NestedIterator(nested_range_detail::EndTag{}, this);
  }

 private:
  std::tuple<FirstRange, Ranges...> ranges_;
};

// Deduction guide
template <typename... OtherRanges>
NestedRange(OtherRanges&&... ranges)
    -> NestedRange<std::decay_t<OtherRanges>...>;

// Deduction function to create NestedRange for older compilers (gcc<11).
template <typename... OtherRanges>
auto MakeNestedRange(OtherRanges&&... ranges) {
  return NestedRange<std::decay_t<OtherRanges>...>(
      std::forward<OtherRanges>(ranges)...);
}

// Convenience wrapper to nest a sequence of ranges.  Keeps a
// light-weight copy to the supplied range (avoids a deep copy).  This works on
// ranges on which calling MakeIteratorRange() makes sense.
template <typename... Containers>
auto NestRanges(Containers&&... ranges) {
  return MakeNestedRange(
      MakeIteratorRange(std::forward<Containers>(ranges))...);
}

}  // namespace genit

#endif  // GENIT_NESTED_RANGE_H_
