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
#include "iterator_range.h"

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

// Allows iterating over a NestedRange.
template <typename FirstRange, typename... Ranges>
class NestedIterator
    : public IteratorFacade<
          NestedIterator<FirstRange, Ranges...>,
          std::tuple<decltype(*std::declval<RangeIteratorType<FirstRange>>()),
                     decltype(*std::declval<RangeIteratorType<Ranges>>())...>,
          nested_range_detail::NestedRangeCategory<FirstRange, Ranges...>> {
 public:
  using OutputRefType =
      std::tuple<decltype(*std::declval<RangeIteratorType<FirstRange>>()),
                 decltype(*std::declval<RangeIteratorType<Ranges>>())...>;
  static constexpr size_t kNumberOfRanges = sizeof...(Ranges) + 1;
  using IndexSeq = absl::make_index_sequence<kNumberOfRanges>;

  NestedIterator(nested_range_detail::BeginTag,
                 const std::tuple<FirstRange, Ranges...>* ranges)
      : ranges_(ranges) {
    MakeBegin(IndexSeq());
  }
  NestedIterator(nested_range_detail::EndTag,
                 const std::tuple<FirstRange, Ranges...>* ranges)
      : ranges_(ranges) {
    MakeEnd(IndexSeq());
  }

  // Default constructor:
  NestedIterator() : ranges_(nullptr), it_tuple_() {}

  NestedIterator(const NestedIterator&) = default;
  NestedIterator(NestedIterator&&) = default;
  NestedIterator& operator=(const NestedIterator&) = default;
  NestedIterator& operator=(NestedIterator&&) = default;

 private:
  friend class IteratorFacadePrivateAccess<NestedIterator>;

  template <size_t... Ids>
  void MakeBegin(absl::index_sequence<Ids...> ids) {
    using std::begin;
    ((void)(std::get<Ids>(it_tuple_) = begin(std::get<Ids>(*ranges_))), ...);
  }

  template <size_t... Ids>
  void MakeEnd(absl::index_sequence<Ids...> ids) {
    using std::begin;
    using std::end;
    ((void)(std::get<Ids>(it_tuple_) =
                (Ids == 0 ? end(std::get<Ids>(*ranges_))
                          : begin(std::get<Ids>(*ranges_)))),
     ...);
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
    return ((std::get<Ids>(it_tuple_) == std::get<Ids>(rhs.it_tuple_)) && ...);
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
      if (std::get<Id>(it_tuple_) == end(std::get<Id>(*ranges_))) {
        std::get<Id>(it_tuple_) = begin(std::get<Id>(*ranges_));
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
      if (std::get<Id>(it_tuple_) == begin(std::get<Id>(*ranges_))) {
        std::get<Id>(it_tuple_) = end(std::get<Id>(*ranges_));
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

  const std::tuple<FirstRange, Ranges...>* ranges_;
  std::tuple<RangeIteratorType<FirstRange>, RangeIteratorType<Ranges>...>
      it_tuple_;
};

template <typename FirstRange, typename... Ranges>
class NestedRange
    : public AliasRangeFacade<NestedRange<FirstRange, Ranges...>,
                              std::tuple<FirstRange, Ranges...>,
                              NestedIterator<FirstRange, Ranges...>> {
  static_assert(
      std::is_convertible_v<nested_range_detail::InnerRangeCategory<Ranges...>,
                            std::forward_iterator_tag>,
      "NestedRange requires inner ranges to be at least forward iterators "
      "(multi-pass).");

 public:
  using BaseRange = std::tuple<FirstRange, Ranges...>;
  using NestedIter = NestedIterator<FirstRange, Ranges...>;
  using BaseFacade = AliasRangeFacade<NestedRange<FirstRange, Ranges...>,
                                      BaseRange, NestedIter>;

  template <typename... OtherRanges>
  explicit NestedRange(OtherRanges&&... ranges)
      : BaseFacade(std::tuple<FirstRange, Ranges...>(
            std::forward<OtherRanges>(ranges)...)) {}

 private:
  friend class AliasRangeFacadePrivateAccess<
      NestedRange<FirstRange, Ranges...>>;

  auto Begin(const BaseRange& base_range) const {
    return NestedIter(nested_range_detail::BeginTag{}, &base_range);
  }
  auto End(const BaseRange& base_range) const {
    return NestedIter(nested_range_detail::EndTag{}, &base_range);
  }
};

// Deduction guide
template <typename... OtherRanges>
NestedRange(OtherRanges&&... ranges)
    -> NestedRange<RangeDecayType<OtherRanges>...>;

// Deduction function to create NestedRange for older compilers (gcc<11).
template <typename... OtherRanges>
auto MakeNestedRange(OtherRanges&&... ranges) {
  return NestedRange<RangeDecayType<OtherRanges>...>(
      std::forward<OtherRanges>(ranges)...);
}

// Convenience wrapper to nest a sequence of ranges. Keeps a
// iterator pairs to the supplied ranges if they are lvalue references.
// Makes a copy/move of the supplied ranges if they are rvalue references.
template <typename... Containers>
auto NestRanges(Containers&&... ranges) {
  return MakeNestedRange(MoveOrAliasRange(std::forward<Containers>(ranges))...);
}

}  // namespace genit

#endif  // GENIT_NESTED_RANGE_H_
