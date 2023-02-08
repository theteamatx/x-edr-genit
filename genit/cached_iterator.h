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

#ifndef GENIT_CACHED_ITERATOR_H_
#define GENIT_CACHED_ITERATOR_H_

#include <iterator>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

// This class template can be used to turn a "lazy" iterator into an eager
// iterator that caches the current value.
// This is mostly useful in combination with lazy iterators such as
// TransformIterator (transform_iterator) to guarantee that the underlying
// iterator is dereferenced only once to obtain its underlying value.
template <typename UnderlyingIter>
class CachedIterator
    : public IteratorFacade<
          CachedIterator<UnderlyingIter>,
          std::decay_t<decltype(*std::declval<UnderlyingIter>())>,
          typename std::iterator_traits<UnderlyingIter>::iterator_category> {
 public:
  template <typename OtherIter>
  explicit CachedIterator(OtherIter&& it) : it_(std::forward<OtherIter>(it)) {}

  // Default constructor:
  CachedIterator() : it_() {}

 private:
  friend class IteratorFacadePrivateAccess<CachedIterator>;

  using ValueType = std::decay_t<decltype(*std::declval<UnderlyingIter>())>;
  using RefType = const ValueType&;

  static_assert(std::is_copy_assignable_v<ValueType>,
                "Value type must be copy-assignable to fulfill standard "
                "requirements on this iterator type");

  // Implementation of the IteratorFacade requirements:
  RefType Dereference() const {
    if (!evaluated_) {
      evaluated_ = true;
      current_value_ = *it_;
    }
    return current_value_;
  }
  void Increment() {
    ++it_;
    evaluated_ = false;
  }
  void Decrement() {
    --it_;
    evaluated_ = false;
  }
  bool IsEqual(const CachedIterator& rhs) const { return it_ == rhs.it_; }
  int DistanceTo(const CachedIterator& rhs) const { return rhs.it_ - it_; }
  void Advance(int n) {
    it_ += n;
    evaluated_ = false;
  }

  UnderlyingIter it_;
  mutable ValueType current_value_;
  mutable bool evaluated_ = false;
};

// Deduction guide
template <typename OtherIter>
CachedIterator(OtherIter &&)->CachedIterator<std::decay_t<OtherIter>>;

// Factory function that conveniently creates a cached iterator object
// using template argument deduction to infer the type of the underlying
// iterator.
template <typename UnderlyingIter>
auto MakeCachedIterator(UnderlyingIter&& it) {
  return CachedIterator(std::forward<UnderlyingIter>(it));
}

// Factory function that conveniently creates a cached iterator range
// using template argument deduction to infer the type of the underlying
// iterator and unary functor.
template <typename Range>
auto CachedRange(Range&& range) {
  using std::begin;
  using std::end;
  return IteratorRange(CachedIterator(begin(std::forward<Range>(range))),
                       CachedIterator(end(std::forward<Range>(range))));
}

template <typename BaseIter>
auto CachedRange(BaseIter&& first, BaseIter&& last) {
  return IteratorRange(CachedIterator(std::forward<BaseIter>(first)),
                       CachedIterator(std::forward<BaseIter>(last)));
}

}  // namespace genit

#endif  // GENIT_CACHED_ITERATOR_H_
