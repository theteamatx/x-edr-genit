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

#ifndef GENIT_ZIP_ITERATOR_H_
#define GENIT_ZIP_ITERATOR_H_

#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"
#include "absl/utility/utility.h"

namespace genit {

namespace zip_iterator_detail {

template <typename Category1, typename Category2>
using LeastPermissivePair =
    std::conditional_t<std::is_convertible<Category1, Category2>::value,
                       Category2, Category1>;

template <typename... CategoryTail>
struct LeastPermissiveImpl {
  static_assert(sizeof...(CategoryTail) != 0, "");
};

template <typename Category1>
struct LeastPermissiveImpl<Category1> {
  using type = Category1;
};

template <typename Category1, typename Category2>
struct LeastPermissiveImpl<Category1, Category2> {
  using type = LeastPermissivePair<Category1, Category2>;
};

template <typename Category1, typename Category2, typename... CategoryTail>
struct LeastPermissiveImpl<Category1, Category2, CategoryTail...> {
  using type =
      typename LeastPermissiveImpl<LeastPermissivePair<Category1, Category2>,
                                   CategoryTail...>::type;
};

template <typename... CategoryTail>
using LeastPermissive = typename LeastPermissiveImpl<CategoryTail...>::type;

template <typename Category>
using ReduceToStdIterCategory = std::conditional_t<
    std::is_convertible_v<Category, std::random_access_iterator_tag>,
    std::random_access_iterator_tag,
    std::conditional_t<
        std::is_convertible_v<Category, std::bidirectional_iterator_tag>,
        std::bidirectional_iterator_tag,
        std::conditional_t<
            std::is_convertible_v<Category, std::forward_iterator_tag>,
            std::forward_iterator_tag,
            std::conditional_t<
                std::is_convertible_v<Category, std::input_iterator_tag>,
                std::input_iterator_tag,
                std::conditional_t<
                    std::is_convertible_v<Category, std::output_iterator_tag>,
                    std::output_iterator_tag, void>>>>>;

template <typename... Iters>
using ComputeIterCategory = LeastPermissive<ReduceToStdIterCategory<
    typename std::iterator_traits<Iters>::iterator_category>...>;

template <typename T>
T&& VariadicMin(T&& val) {
  return std::forward<T>(val);
}

template <typename T0, typename T1, typename... Ts>
auto VariadicMin(T0&& val1, T1&& val2, Ts&&... vs) {
  return VariadicMin((val1 < val2) ? val1 : val2, std::forward<Ts>(vs)...);
}

}  // namespace zip_iterator_detail

// A ZipIterator is an iterator that combines multiple underlying iterators
// into a single iterator that produces a tuple of the underlying values when
// dereferenced.
//
// See MakeZipIterator and ZipRange functions for convenient ways to create
// zip iterators with template argument deduction.
//
// Example:
// We could copy a vector into another like this:
// void Copy(const std::vector<int>& src, std::vector<int>* dest) {
//   for (auto src_dest : ZipRange(src, *dest)) {
//     get<1>(src_dest) = get<0>(src_dest);
//   }
// }
//
template <typename... Iters>
class ZipIterator : public IteratorFacade<
                        ZipIterator<Iters...>,
                        std::tuple<decltype(*std::declval<Iters>())...>,
                        zip_iterator_detail::ComputeIterCategory<Iters...>> {
 public:
  // Universal constructor:
  template <typename... OtherIters>
  explicit ZipIterator(OtherIters... it) : it_tuple_(std::move(it)...) {}

  // Default constructor:
  ZipIterator() : it_tuple_() {}

  ZipIterator(const ZipIterator&) = default;
  ZipIterator(ZipIterator&&) = default;

 private:
  friend class IteratorFacadePrivateAccess<ZipIterator>;

  using OutputRefType = std::tuple<decltype(*std::declval<Iters>())...>;
  using IterIndexSeq = absl::make_index_sequence<sizeof...(Iters)>;

  // Implementation of the IteratorFacade requirements:
  template <size_t... Ids>
  OutputRefType Dereference(absl::index_sequence<Ids...> ids) const {
    return OutputRefType{(*std::get<Ids>(it_tuple_))...};
  }
  template <size_t... Ids>
  void Increment(absl::index_sequence<Ids...> ids) {
    ((void)++std::get<Ids>(it_tuple_), ...);
  }
  template <size_t... Ids>
  void Decrement(absl::index_sequence<Ids...> ids) {
    ((void)--std::get<Ids>(it_tuple_), ...);
  }
  template <size_t... Ids>
  bool IsEqual(const ZipIterator& rhs, absl::index_sequence<Ids...> ids) const {
    // Only require one pair of iterators to match such that iterations are
    // stopped by the shortest range if all ranges don't match.
    return ((std::get<Ids>(it_tuple_) == std::get<Ids>(rhs.it_tuple_)) || ...);
  }
  template <size_t... Ids>
  int DistanceTo(const ZipIterator& rhs,
                 absl::index_sequence<Ids...> ids) const {
    // Return the smallest distance between iterators because that is where
    // iterations will stop.
    return zip_iterator_detail::VariadicMin(
        (std::get<Ids>(rhs.it_tuple_) - std::get<Ids>(it_tuple_))...);
  }
  template <size_t... Ids>
  void Advance(int n, absl::index_sequence<Ids...> ids) {
    ((void)(std::get<Ids>(it_tuple_) += n), ...);
  }

  OutputRefType Dereference() const { return Dereference(IterIndexSeq()); }
  void Increment() { Increment(IterIndexSeq()); }
  void Decrement() { Decrement(IterIndexSeq()); }
  bool IsEqual(const ZipIterator& rhs) const {
    return IsEqual(rhs, IterIndexSeq());
  }
  int DistanceTo(const ZipIterator& rhs) const {
    return DistanceTo(rhs, IterIndexSeq());
  }
  void Advance(int n) { Advance(n, IterIndexSeq()); }

  std::tuple<Iters...> it_tuple_;
};

// Deduction guide
template <typename... OtherIters>
ZipIterator(OtherIters... it)->ZipIterator<std::decay_t<OtherIters>...>;

// Factory function that conveniently creates a zip iterator object
// using template argument deduction to infer the types of the underlying
// iterators.
// iters: The underlying iterators.
// For example, when zipping two vectors v1 and v2:
//   auto zip_it = MakeZipIterator(v1.begin(), v2.begin());
template <typename... UnderlyingIters>
auto MakeZipIterator(UnderlyingIters&&... iters) {
  return ZipIterator(std::forward<UnderlyingIters>(iters)...);
}

// Factory function that conveniently creates a zip iterator range
// using template argument deduction to infer the type of the underlying
// ranges.
// ranges: The underlying ranges.
// For example, when zipping two vectors v1 and v2:
//   auto zip_range = ZipRange(v1, v2);
template <typename... Ranges>
auto ZipRange(Ranges&&... ranges) {
  using std::begin;
  using std::end;
  return IteratorRange(ZipIterator(begin(std::forward<Ranges>(ranges))...),
                       ZipIterator(end(std::forward<Ranges>(ranges))...));
}

// Factory function that conveniently creates an enumerated iterator range
// where each element of the range is paired with its integer index / counter
// represented as an `int` (size of range should be less than max of int).
// For example, when enumerate a vector v:
//   auto enumerated_range = EnumerateRange(v);
// or:
//   for (auto [i, x] : EnumerateRange(v)) { .. }
template <typename Range>
auto EnumerateRange(Range&& range) {
  return ZipRange(IndexRange(0, std::numeric_limits<int>::max()),
                  std::forward<Range>(range));
}

}  // namespace genit

#endif  // GENIT_ZIP_ITERATOR_H_
