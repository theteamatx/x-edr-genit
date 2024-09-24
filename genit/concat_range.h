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

#ifndef GENIT_CONCAT_RANGE_H_
#define GENIT_CONCAT_RANGE_H_

#include <array>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "absl/types/variant.h"
#include "absl/utility/utility.h"
#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"
#include "genit/zip_iterator.h"

namespace genit {

// Given a sequence of iterator ranges, concatenate these to provide a single
// range spanning all of them.
//
// For example,
//
// std::vector<int> v1 = {1,2,3};
// const std::list<int> v2 = {4, 5, 6};
// auto range = ConcatenateRanges(v1, v2);
// for (int x : range) {
//   // x ranges over 1, 2, 3, 4, 5, 6
//   ...
// }
//

namespace concat_range_detail {
template <bool... Ts>
struct AllOf : std::true_type {};
template <bool... Ts>
struct AllOf<true, Ts...> : AllOf<Ts...> {};
template <bool... Ts>
struct AllOf<false, Ts...> : std::false_type {};

template <typename T, typename... Ts>
inline constexpr bool are_same_v = AllOf<std::is_same_v<T, Ts>...>::value;

// Given types T0, T1, ..., find a type T that all types are convertible to.
template <typename... Ranges>
using CommonValueType = std::common_type_t<RangeValueType<Ranges>...>;

// Given a common value type T, find the first that can be used amongst
// T&,
// const T&,
// T
// as the reference type.
template <typename... Ranges>
using CommonReferenceType = std::conditional_t<
    // Try T&.
    are_same_v<CommonValueType<Ranges...>&, RangeReferenceType<Ranges>...>,
    CommonValueType<Ranges...>&,
    std::conditional_t<
        // Try const T&.
        are_same_v<const CommonValueType<Ranges...>&,
                   std::add_const_t<RangeReferenceType<Ranges>>...>,
        const CommonValueType<Ranges...>&,
        // Use T.
        CommonValueType<Ranges...>>>;

struct BeginTag {};
struct EndTag {};

template <typename Range>
bool RangeIsEmpty(Range&& range) {
  using std::begin;
  using std::end;
  return (begin(range) == end(range));
}

template <typename Range>
int SizeOfRange(Range&& range) {
  using std::begin;
  using std::end;
  return (end(range) - begin(range));
}

}  // namespace concat_range_detail

template <typename... Ranges>
class ConcatRange {
  static_assert(sizeof...(Ranges) > 0,
                "ConcatRange requires at least one range");

 public:
  template <typename... OtherRanges>
  explicit ConcatRange(OtherRanges&&... ranges)
      : ranges_(std::forward<OtherRanges>(ranges)...) {
    SetAccumulatedSizes(
        typename std::iterator_traits<ConcatIterator>::iterator_category{},
        IndexSeq{});
  }

  // Allows iterating over a ConcatRange.
  class ConcatIterator
      : public IteratorFacade<
            ConcatIterator, concat_range_detail::CommonReferenceType<Ranges...>,
            zip_iterator_detail::ComputeIterCategory<
                RangeIteratorType<Ranges>...>> {
   public:
    ConcatIterator(concat_range_detail::BeginTag,
                   const ConcatRange<Ranges...>* range)
        : concat_(range), it_(absl::in_place_index<0>, BeginOf<0>()) {
      // Skip empty ranges and find a valid begin.
      if (concat_range_detail::RangeIsEmpty(std::get<0>(concat_->ranges_))) {
        Increment();
      }
    }
    ConcatIterator(concat_range_detail::EndTag,
                   const ConcatRange<Ranges...>* range)
        : concat_(range),
          it_(absl::in_place_index<kNumberOfRanges - 1>,
              EndOf<kNumberOfRanges - 1>()) {}

   private:
    friend class IteratorFacadePrivateAccess<ConcatIterator>;

    static constexpr size_t kNumberOfRanges = sizeof...(Ranges);
    using IndexSeq = absl::make_index_sequence<kNumberOfRanges>;

    template <size_t Id>
    auto BeginOf() const {
      using std::begin;
      return begin(std::get<Id>(concat_->ranges_));
    }
    template <size_t Id>
    auto EndOf() const {
      using std::end;
      return end(std::get<Id>(concat_->ranges_));
    }

    // Calculates the offset of it_ in the concatenated range.
    template <size_t Id>
    int IndexOfIterator() const {
      return concat_->accumulated_sizes_[Id] -
             (EndOf<Id>() - std::get<Id>(it_));
    }
    template <size_t... Ids>
    int IndexOfIterator(absl::index_sequence<Ids...> ids) const {
      static_assert(
          std::is_same_v<
              typename std::iterator_traits<ConcatIterator>::iterator_category,
              std::random_access_iterator_tag>,
          "This function should only be instantiated for random access "
          "iterators.");
      int index = 0;
      const int variant_index = it_.index();
      (void)((Ids == variant_index && (index = IndexOfIterator<Ids>(), true)) ||
             ...);
      return index;
    }

    // Sets the iterator to point to a desired index.
    template <size_t Id>
    void SetToIndex(int index) {
      const int size =
          concat_range_detail::SizeOfRange(std::get<Id>(concat_->ranges_));
      const int offset = concat_->accumulated_sizes_[Id] - size;
      const int relative_index = index - offset;
      it_ = VariantIt(absl::in_place_index<Id>, BeginOf<Id>() + relative_index);
    }
    template <size_t... Ids>
    void SetToIndex(absl::index_sequence<Ids...> ids, int index) {
      const auto it =
          std::upper_bound(concat_->accumulated_sizes_.cbegin(),
                           concat_->accumulated_sizes_.cend(), index);
      const int variant_index = std::min<int>(
          kNumberOfRanges - 1, (it - concat_->accumulated_sizes_.cbegin()));
      (void)((Ids == variant_index && (SetToIndex<Ids>(index), true)) || ...);
    }

    // Increments the iterator within the current range, or sets it to the
    // beginning of the next range.  Returns true if the incrementing finished
    // to allow skipping empty ranges.
    template <size_t Id>
    bool Increment(absl::in_place_index_t<Id>) {
      auto& it = std::get<Id>(it_);
      if constexpr (Id == kNumberOfRanges - 1) {
        ++it;
      } else {
        const auto last = EndOf<Id>();
        if (it != last) {
          ++it;
        }
        if (it == last) {
          it_ = VariantIt(absl::in_place_index<Id + 1>, BeginOf<Id + 1>());
          // Finished if the set iterator is valid, or the end.
          return (Id + 2 == kNumberOfRanges) ||
                 (!concat_range_detail::RangeIsEmpty(
                     std::get<Id + 1>(concat_->ranges_)));
        }
      }
      return true;
    }
    template <size_t... Ids>
    void Increment(absl::index_sequence<Ids...> ids) {
      const int variant_index = it_.index();
      (void)((Ids >= variant_index && Increment(absl::in_place_index<Ids>)) ||
             ...);
    }

    template <size_t Id>
    bool Decrement(absl::in_place_index_t<Id>) {
      auto& it = std::get<Id>(it_);
      if constexpr (Id != 0) {
        if (it == BeginOf<Id>()) {
          it_ = VariantIt(absl::in_place_index<Id - 1>, EndOf<Id - 1>());
          return false;
        }
      }
      --it;
      return true;
    }
    template <size_t... Ids>
    void Decrement(absl::index_sequence<Ids...> ids) {
      const int variant_index = it_.index();
      // Use reverse iteration, using (kNumberOfRanges-1) - Ids as index.
      (void)(((variant_index >= (kNumberOfRanges - 1) - Ids) &&
              Decrement(absl::in_place_index<(kNumberOfRanges - 1) - Ids>)) ||
             ...);
    }

    using OutputRefType = concat_range_detail::CommonReferenceType<Ranges...>;
    OutputRefType Dereference() const {
      return std::visit([](const auto& it) -> OutputRefType { return *it; },
                        it_);
    }

    void Increment() { Increment(IndexSeq()); }
    void Decrement() { Decrement(IndexSeq()); }
    bool IsEqual(const ConcatIterator& rhs) const { return it_ == rhs.it_; }
    int DistanceTo(const ConcatIterator& rhs) const {
      // Assumes that both iterators refer to the same range.
      const int index_lhs = IndexOfIterator(IndexSeq());
      const int index_rhs = rhs.IndexOfIterator(IndexSeq());
      return index_rhs - index_lhs;
    }
    void Advance(int n) {
      const int prev_index = IndexOfIterator(IndexSeq());
      const int index = prev_index + n;
      SetToIndex(IndexSeq(), index);
    }

    const ConcatRange<Ranges...>* concat_;
    using VariantIt = std::variant<RangeIteratorType<Ranges>...>;
    VariantIt it_;
  };
  friend class ConcatIterator;

  using iterator = ConcatIterator;
  using value_type = typename std::iterator_traits<ConcatIterator>::value_type;
  using reference = typename std::iterator_traits<ConcatIterator>::reference;

  iterator begin() const {
    return ConcatIterator(concat_range_detail::BeginTag{}, this);
  }
  iterator end() const {
    return ConcatIterator(concat_range_detail::EndTag{}, this);
  }

 private:
  using IndexSeq = absl::make_index_sequence<sizeof...(Ranges)>;

  template <typename Category, size_t... Ids>
  void SetAccumulatedSizes(Category, absl::index_sequence<Ids...> ids) {
    // No op.
  }
  template <size_t... Ids>
  void SetAccumulatedSizes(std::random_access_iterator_tag,
                           absl::index_sequence<Ids...> ids) {
    int acc = 0;
    auto it = std::begin(accumulated_sizes_);
    (void)(((*(it++) = acc +=
             concat_range_detail::SizeOfRange(std::get<Ids>(ranges_))) == -1) ||
           ...);
  }

  struct None {};
  using AccumulatedSizes =
      std::conditional_t<std::is_same_v<typename std::iterator_traits<
                                            ConcatIterator>::iterator_category,
                                        std::random_access_iterator_tag>,
                         std::array<int, sizeof...(Ranges)>, None>;

  std::tuple<Ranges...> ranges_;
  AccumulatedSizes accumulated_sizes_;
};

// Deduction guide
template <typename... OtherRanges>
ConcatRange(OtherRanges&&... ranges)
    -> ConcatRange<RangeDecayType<OtherRanges>...>;

// Deduction function to create ConcatRange for older compilers (gcc<11).
template <typename... OtherRanges>
auto MakeConcatRange(OtherRanges&&... ranges) {
  return ConcatRange<RangeDecayType<OtherRanges>...>(
      std::forward<OtherRanges>(ranges)...);
}

// Convenience wrapper to concatenate a sequence of ranges. Keeps a
// iterator pairs to the supplied ranges if they are lvalue references.
// Makes a copy/move of the supplied ranges if they are rvalue references.
template <typename... Containers>
auto ConcatenateRanges(Containers&&... ranges) {
  return MakeConcatRange(MoveOrAliasRange(std::forward<Containers>(ranges))...);
}

}  // namespace genit

#endif  // GENIT_CONCAT_RANGE_H_
