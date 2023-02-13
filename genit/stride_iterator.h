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

#ifndef GENIT_STRIDE_ITERATOR_H_
#define GENIT_STRIDE_ITERATOR_H_

#include <iterator>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

// This class template can be used to turn a pointer to a base-type into an
// iterator with a stride.
//
// For example, this can used to turn an array of derived class objects into
// a range of base-class references:
//
//   std::vector<DerivedType> v;
//   using BaseIter = StrideIterator<BaseType>;
//   constexpr int kStride = sizeof(DerivedType);
//   SomePolymorphicFunction(BaseIter(v.data(), kStride),
//                           BaseIter(v.data() + v.size(), kStride));
//
// But this class can also be applied in many other contexts.
//
// Caveats:
//  - Comparing two StrideIterator that have different strides or come from
//    different containers has undefined behavior. No diagnostics.
template <typename ValueType>
class StrideIterator
    : public IteratorFacade<StrideIterator<ValueType>, ValueType&,
                            std::random_access_iterator_tag> {
 public:
  StrideIterator(ValueType* ptr, int stride) : ptr_(ptr), stride_(stride) {}

 private:
  friend class IteratorFacadePrivateAccess<StrideIterator>;

  using RefType = ValueType&;
  using CharType =
      std::conditional_t<std::is_const_v<ValueType>, const char, char>;

  CharType* GetCharPtr() const { return reinterpret_cast<CharType*>(ptr_); }
  void SetCharPtr(CharType* char_ptr) {
    ptr_ = reinterpret_cast<ValueType*>(char_ptr);
  }

  // Implementation of the IteratorFacade requirements:
  RefType Dereference() const { return *ptr_; }
  void Increment() { Advance(1); }
  void Decrement() { Advance(-1); }
  bool IsEqual(const StrideIterator& rhs) const { return ptr_ == rhs.ptr_; }
  int DistanceTo(const StrideIterator& rhs) const {
    return (rhs.GetCharPtr() - GetCharPtr()) / stride_;
  }
  void Advance(int n) { SetCharPtr(GetCharPtr() + n * stride_); }

  ValueType* ptr_;
  int stride_;
};

// Deduction guide
template <typename ValueType>
StrideIterator(ValueType* ptr, int stride) -> StrideIterator<ValueType>;

// Factory function that conveniently creates a stride iterator object
// using template argument deduction to infer the type of the underlying
// value type.
template <typename ValueType>
auto MakeStrideIterator(ValueType* ptr, int stride) {
  return StrideIterator(ptr, stride);
}

// Factory function that conveniently creates a stride iterator range.
template <typename ValueType>
auto StrideRange(ValueType* first, ValueType* last, int stride) {
  return IteratorRange(StrideIterator(first, stride),
                       StrideIterator(last, stride));
}

}  // namespace genit

#endif  // GENIT_STRIDE_ITERATOR_H_
