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

#ifndef GENIT_ITERATOR_FACADE_H_
#define GENIT_ITERATOR_FACADE_H_

#include <cassert>
#include <iterator>
#include <type_traits>

namespace genit {

// Befriend this class in the implementation of a Derived class to be used
// with IteratorFacade (see below).
template <typename Derived>
class IteratorFacadePrivateAccess {
 public:
  // Implementation of the IteratorFacade requirements:
  static decltype(auto) Dereference(const Derived& lhs) {
    return lhs.Dereference();
  }
  static void Increment(Derived* this_) { this_->Increment(); }
  static void Decrement(Derived* this_) { this_->Decrement(); }
  static bool IsEqual(const Derived& lhs, const Derived& rhs) {
    return lhs.IsEqual(rhs);
  }
  static int DistanceTo(const Derived& lhs, const Derived& rhs) {
    return lhs.DistanceTo(rhs);
  }
  static void Advance(Derived* this_, int n) { this_->Advance(n); }
};

// IteratorFacade facilitates the creation of iterators by filling in all the
// operator overloads and traits that are required for an iterator type from
// a smaller set of functions provided by a derived type (i.e., CRTP:
// curiously recurring template pattern).
//
// The template arguments are as follows:
//  - Derived: The type of the user-defined iterator being created using this
//             IteratorFacade CRTP class. Derived must be derived from the
//             IteratorFacade instantiation being created.
//  - Reference: The type returned by Derived::Dereference(). It does not have
//               to be a reference type (could be value or rvalue reference
//               too).
//  - Category: The iterator_category tag (see std::iterator_traits).
//
// The set of functions that should be provided by the derived class
// are the following:
// (to make the functions private, befriend IteratorFacadePrivateAccesss)
//
// Reference Dereference() const;
//   Returns the value or a reference to the value pointed to by the iterator.
//   If a value is returned, then the iterator can only be an input iterator
//   and the operator->() is disabled (by static_assert, because it must
//   return a pointer and we cannot return a pointer to a local variable).
//
// bool IsEqual(const Derived& rhs) const;
//   Checks if the iterator is equal to 'rhs'.
//
// void Increment();
//   Advances the iterator by 1. Only needed for a ForwardIterator.
//
// void Decrement();
//   Walks back the iterator by 1. Only needed for a BidirectionalIterator.
//
// void Advance(int i);
//   Advances the iterator by 'i'. Only needed for a RandomAccessIterator.
//
// int DistanceTo(const Derived& rhs) const;
//   Computes the distance from the iterator to 'rhs', i.e., if 'd' is the
//   result of this function call, then '*this + d == rhs'. Only needed for a
//   RandomAccessIterator.
template <typename Derived, typename Reference, typename Category>
class IteratorFacade {
 private:
  using nondecayed_value_type = std::remove_reference_t<Reference>;

 public:
  using value_type = std::decay_t<nondecayed_value_type>;
  using reference = Reference;
  using pointer = nondecayed_value_type*;
  using difference_type = int;
  using iterator_category = Category;

  // Dereference operators:
  reference operator*() const {
    return IteratorFacadePrivateAccess<Derived>::Dereference(
        static_cast<const Derived&>(*this));
  }
  pointer operator->() const {
    static_assert(std::is_reference_v<Reference>,
                  "IteratorFacade::operator->() is only supported if "
                  "Dereference() returns a reference!");
    return &(this->operator*());
  }
  // Note: The operator[] works by creating a temporary iterator for
  // (*this) + i and then dereferencing it, which means that if Dereference()
  // returns a reference to a data member of Derived, this will be ill-formed.
  // This condition is asserted at run-time by check the address of the
  // result of Derived::Dereference() against the iterator's memory footprint.
  reference operator[](difference_type i) const;

  // Increment / Decrement operators:
  Derived& operator++() {
    IteratorFacadePrivateAccess<Derived>::Increment(
        static_cast<Derived*>(this));
    return static_cast<Derived&>(*this);
  }
  Derived operator++(int) {
    Derived old_it = static_cast<const Derived&>(*this);
    IteratorFacadePrivateAccess<Derived>::Increment(
        static_cast<Derived*>(this));
    return old_it;  // NRVO
  }
  Derived& operator--() {
    IteratorFacadePrivateAccess<Derived>::Decrement(
        static_cast<Derived*>(this));
    return static_cast<Derived&>(*this);
  }
  Derived operator--(int) {
    Derived old_it = static_cast<const Derived&>(*this);
    IteratorFacadePrivateAccess<Derived>::Decrement(
        static_cast<Derived*>(this));
    return old_it;  // NRVO
  }

  // Random-access operators:
  Derived& operator+=(difference_type i) {
    IteratorFacadePrivateAccess<Derived>::Advance(static_cast<Derived*>(this),
                                                  i);
    return static_cast<Derived&>(*this);
  }
  Derived& operator-=(difference_type i) {
    IteratorFacadePrivateAccess<Derived>::Advance(static_cast<Derived*>(this),
                                                  -i);
    return static_cast<Derived&>(*this);
  }
};

namespace iterator_facade_detail {

// Used to convert iterators to the common type:
template <typename Derived1, typename Derived2,
          std::enable_if_t<std::is_convertible_v<Derived2, Derived1>, int> = 0>
bool DispatchedIsEqual(const Derived1& lhs, const Derived2& rhs) {
  return IteratorFacadePrivateAccess<Derived1>::IsEqual(lhs, rhs);
}

template <typename Derived1, typename Derived2,
          std::enable_if_t<std::is_convertible_v<Derived1, Derived2> &&
                               !std::is_convertible_v<Derived2, Derived1>,
                           int> = 0>
bool DispatchedIsEqual(const Derived1& lhs, const Derived2& rhs) {
  return IteratorFacadePrivateAccess<Derived2>::IsEqual(rhs, lhs);
}

// Used to convert iterators to the common type:
template <typename Derived1, typename Derived2,
          std::enable_if_t<std::is_convertible_v<Derived2, Derived1>, int> = 0>
int DispatchedDistanceFromTo(const Derived1& lhs, const Derived2& rhs) {
  return IteratorFacadePrivateAccess<Derived1>::DistanceTo(lhs, rhs);
}

template <typename Derived1, typename Derived2,
          std::enable_if_t<std::is_convertible_v<Derived1, Derived2> &&
                               !std::is_convertible_v<Derived2, Derived1>,
                           int> = 0>
int DispatchedDistanceFromTo(const Derived1& lhs, const Derived2& rhs) {
  return -IteratorFacadePrivateAccess<Derived2>::DistanceTo(rhs, lhs);
}

// Check if iterator and dereference result are within the same object:
// This can only be checked for lvalue references. It is technically possible
// to create an iterator that returns an rvalue reference to one of its own
// data members, but C++ rules make it hard to check for that kind of abuse.
template <typename ResultType, typename DerivedIter,
          std::enable_if_t<std::is_lvalue_reference_v<ResultType>, int> = 0>
bool IsValueInsideIterator(const DerivedIter& it) {
  const char* it_ptr = reinterpret_cast<const char*>(&it);
  const char* val_ptr = reinterpret_cast<const char*>(
      &IteratorFacadePrivateAccess<DerivedIter>::Dereference(it));
  return (val_ptr >= it_ptr) && (val_ptr < it_ptr + sizeof(DerivedIter));
}

template <typename ResultType, typename DerivedIter,
          std::enable_if_t<!std::is_lvalue_reference_v<ResultType>, int> = 0>
bool IsValueInsideIterator(const DerivedIter&) {
  return false;
}

}  // namespace iterator_facade_detail

template <typename Derived, typename Reference, typename Category>
typename IteratorFacade<Derived, Reference, Category>::reference
    IteratorFacade<Derived, Reference, Category>::operator[](int i) const {
  Derived adv_it = static_cast<const Derived&>(*this);
  IteratorFacadePrivateAccess<Derived>::Advance(&adv_it, i);
  assert(!iterator_facade_detail::IsValueInsideIterator<reference>(adv_it));
  return IteratorFacadePrivateAccess<Derived>::Dereference(adv_it);
}

// Comparison operators:
template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator==(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
                const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return iterator_facade_detail::DispatchedIsEqual(
      static_cast<const Derived1&>(lhs), static_cast<const Derived2&>(rhs));
}

template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator!=(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
                const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return !(lhs == rhs);
}

// Random-access comparison operators:
template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
int operator-(const IteratorFacade<Derived1, Ref1, Cat1>& destination,
              const IteratorFacade<Derived2, Ref2, Cat2>& source) {
  return iterator_facade_detail::DispatchedDistanceFromTo(
      static_cast<const Derived2&>(source),
      static_cast<const Derived1&>(destination));
}

template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator<(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
               const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return (lhs - rhs < 0);
}

template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator<=(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
                const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return (lhs - rhs <= 0);
}

template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator>(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
               const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return (lhs - rhs > 0);
}

template <typename Derived1, typename Ref1, typename Cat1, typename Derived2,
          typename Ref2, typename Cat2>
bool operator>=(const IteratorFacade<Derived1, Ref1, Cat1>& lhs,
                const IteratorFacade<Derived2, Ref2, Cat2>& rhs) {
  return (lhs - rhs >= 0);
}

// Random-access operators (non-member versions):
template <typename Derived, typename Reference, typename Category>
Derived operator-(const IteratorFacade<Derived, Reference, Category>& it,
                  int i) {
  Derived result = static_cast<const Derived&>(it);
  result -= i;
  return result;  // NRVO
}

template <typename Derived, typename Reference, typename Category>
Derived operator+(const IteratorFacade<Derived, Reference, Category>& it,
                  int i) {
  Derived result = static_cast<const Derived&>(it);
  result += i;
  return result;  // NRVO
}

template <typename Derived, typename Reference, typename Category>
Derived operator+(int i,
                  const IteratorFacade<Derived, Reference, Category>& it) {
  return it + i;
}

// This iterator class is a simple application of IteratorFacade and its
// main purpose is to provide an iterator class that can iterate through
// indices (e.g., of a vector container). There are some use-cases where
// this is useful. And it also provides an easy way to create an iterator
// range for getting sequential integer numbers without having to create
// a container with those integers (similar to Python-3 range() function).
class IndexIterator : public IteratorFacade<IndexIterator, int,
                                            std::random_access_iterator_tag> {
 public:
  IndexIterator() = default;
  explicit IndexIterator(int n) : index_(n) {}

 private:
  friend class IteratorFacadePrivateAccess<IndexIterator>;

  int Dereference() const { return index_; }
  void Increment() { ++index_; }
  void Decrement() { --index_; }
  bool IsEqual(const IndexIterator& rhs) const { return index_ == rhs.index_; }
  int DistanceTo(const IndexIterator& rhs) const { return rhs.index_ - index_; }
  void Advance(int n) { index_ += n; }

  int index_ = 0;
};

}  // namespace genit

#endif  // GENIT_ITERATOR_FACADE_H_
