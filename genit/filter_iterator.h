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

#ifndef GENIT_FILTER_ITERATOR_H_
#define GENIT_FILTER_ITERATOR_H_

// The filter iterator allows to filter a range on a predicate.  The filtered
// range contains the elements for which the predicate returns true.  Since the
// filtered range is not cached, the range is at most bidirectional.  In case
// where random access is preferred, std::copy_if() into a std::vector<>, or
// std::remove_if() on an existing container might be preferable.
//
// Example usage:
// const int values [] = {1,2,3,4,5};
// for (int x : FilterRange(values, is_even)) {
//     // Values are 2, 4.
// }

#include <iterator>
#include <utility>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"
#include "genit/zip_iterator.h"

namespace genit {

// Forward-decl.
template <typename BaseRange, typename Predicate>
class FilteredRange;

template <typename BaseRange, typename Predicate>
class FilterIterator
    : public IteratorFacade<
          FilterIterator<BaseRange, Predicate>,
          typename std::iterator_traits<
              RangeIteratorType<BaseRange>>::reference,
          zip_iterator_detail::LeastPermissive<
              std::bidirectional_iterator_tag,
              zip_iterator_detail::ReduceToStdIterCategory<
                  typename std::iterator_traits<
                      RangeIteratorType<BaseRange>>::iterator_category>>> {
 public:
  FilterIterator(RangeIteratorType<BaseRange> it,
                 const FilteredRange<BaseRange, Predicate>* parent)
      : it_(std::move(it)), parent_(parent) {
    while (it_ != parent_->BaseEnd() && !parent_->EvaluatePredicate(*it_)) {
      ++it_;
    }
  }

 private:
  friend class IteratorFacadePrivateAccess<FilterIterator>;

  using OutputRefType =
      typename std::iterator_traits<RangeIteratorType<BaseRange>>::reference;

  OutputRefType Dereference() const { return *it_; }
  void Increment() {
    while (++it_ != parent_->BaseEnd() && !parent_->EvaluatePredicate(*it_)) {
    }
  }
  void Decrement() {
    while (!parent_->EvaluatePredicate(*--it_)) {
    }
  }
  bool IsEqual(const FilterIterator& other) const { return it_ == other.it_; }

  RangeIteratorType<BaseRange> it_;
  const FilteredRange<BaseRange, Predicate>* parent_ = nullptr;
};

template <typename BaseRange, typename Predicate>
class FilteredRange
    : public AliasRangeFacade<FilteredRange<BaseRange, Predicate>, BaseRange,
                              FilterIterator<BaseRange, Predicate>> {
 public:
  using FiltIter = FilterIterator<BaseRange, Predicate>;
  using BaseFacade = AliasRangeFacade<FilteredRange<BaseRange, Predicate>,
                                      BaseRange, FiltIter>;

  // Constructor from a Range
  template <typename OtherRange, typename OtherPredicate>
  explicit FilteredRange(OtherRange&& r, OtherPredicate&& pred)
      : BaseFacade(std::forward<OtherRange>(r)),
        pred_(std::forward<OtherPredicate>(pred)) {}

  // Default assignment operator
  FilteredRange& operator=(const FilteredRange&) = default;
  FilteredRange& operator=(FilteredRange&&) = default;
  FilteredRange(const FilteredRange&) = default;
  FilteredRange(FilteredRange&&) = default;

 private:
  friend class AliasRangeFacadePrivateAccess<
      FilteredRange<BaseRange, Predicate>>;
  friend class FilterIterator<BaseRange, Predicate>;

  auto Begin(const BaseRange& base_range) const {
    using std::begin;
    return FiltIter(begin(base_range), this);
  }
  auto End(const BaseRange& base_range) const {
    using std::end;
    return FiltIter(end(base_range), this);
  }

  auto BaseEnd() const {
    using std::end;
    return end(this->base_range_);
  }
  bool EvaluatePredicate(
      typename std::iterator_traits<RangeIteratorType<BaseRange>>::reference
          value) const {
    return pred_(value);
  }

  Predicate pred_;
};

template <typename Range, typename Predicate>
auto FilterRange(Range&& range, Predicate&& pred) {
  return FilteredRange<decltype(MoveOrAliasRange(std::forward<Range>(range))),
                       std::decay_t<Predicate>>(
      MoveOrAliasRange(std::forward<Range>(range)),
      std::forward<Predicate>(pred));
}

template <typename BaseIter, typename Predicate>
auto FilterRange(BaseIter&& first, BaseIter&& last, Predicate&& pred) {
  return FilterRange(MakeIteratorRange(first, last),
                     std::forward<Predicate>(pred));
}

}  // namespace genit
#endif  // GENIT_FILTER_ITERATOR_H_
