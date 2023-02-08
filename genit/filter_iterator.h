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

template <typename BaseIterator, typename Predicate>
class FilterIterator
    : public IteratorFacade<
          FilterIterator<BaseIterator, Predicate>,
          typename std::iterator_traits<BaseIterator>::reference,
          zip_iterator_detail::LeastPermissive<
              std::bidirectional_iterator_tag,
              zip_iterator_detail::ReduceToStdIterCategory<
                  typename std::iterator_traits<
                      BaseIterator>::iterator_category>>> {
 public:
  template <typename Iterator, typename InputPredicate>
  FilterIterator(Iterator&& it, Iterator&& end, InputPredicate&& predicate)
      : end_(std::forward<Iterator>(end)),
        it_(std::forward<Iterator>(it)),
        pred_(std::forward<InputPredicate>(predicate)) {
    while (it_ != end_ && !pred_(*it_)) {
      ++it_;
    }
  }

  // The C++ standard requires C++ iterators to be copy-assignable, which
  // can only be done if the unary functor is copy-assignable.
  // A reference-wrapper (std::ref / std::cref) can be used to circumvent this
  // by the caller, if applicable. std::function is another option.
  static_assert(std::is_copy_assignable_v<Predicate>,
                "The predicate must be copy-assignable to fulfill standard "
                "requirements on this iterator type");

 private:
  friend class IteratorFacadePrivateAccess<FilterIterator>;

  using OutputRefType = typename std::iterator_traits<BaseIterator>::reference;

  OutputRefType Dereference() const { return *it_; }
  void Increment() {
    while (++it_ != end_ && !pred_(*it_)) {
    }
  }
  void Decrement() {
    while (!pred_(*--it_)) {
    }
  }
  bool IsEqual(const FilterIterator& other) const { return it_ == other.it_; }

  BaseIterator end_;
  BaseIterator it_;
  Predicate pred_;
};

// Deduction guide
template <typename BaseIterator, typename Predicate>
FilterIterator(BaseIterator&& it, BaseIterator&& end, Predicate&& predicate)
    ->FilterIterator<std::decay_t<BaseIterator>, std::decay_t<Predicate>>;

template <typename BaseIterator, typename Predicate>
auto MakeFilterIterator(BaseIterator&& it, BaseIterator&& end,
                        Predicate&& pred) {
  return FilterIterator(std::forward<BaseIterator>(it),
                        std::forward<BaseIterator>(end),
                        std::forward<Predicate>(pred));
}

template <typename Range, typename Predicate>
auto FilterRange(Range&& range, Predicate&& pred) {
  using std::begin;
  using std::end;
  return IteratorRange(FilterIterator(begin(std::forward<Range>(range)),
                                      end(std::forward<Range>(range)),
                                      std::forward<Predicate>(pred)),
                       FilterIterator(end(std::forward<Range>(range)),
                                      end(std::forward<Range>(range)),
                                      std::forward<Predicate>(pred)));
}

}  // namespace genit
#endif  // GENIT_FILTER_ITERATOR_H_
