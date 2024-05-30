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

#ifndef GENIT_TRANSFORM_ITERATOR_H_
#define GENIT_TRANSFORM_ITERATOR_H_

#include <iterator>

#include "genit/iterator_facade.h"
#include "genit/iterator_range.h"

namespace genit {

// A TransformIterator is an iterator that combines an underlying iterator
// and a unary functor to convert the dereference result of the underlying
// iterator to whatever results from the unary functor call.
// For example, the unary functor could be a conversion between two similar
// types (e.g., Pose3d -> Pose2d) to make a container of one type look like
// a container of the other type.
//
// See MakeTransformIterator function for a convenient way to create a
// transform iterator with template argument deduction.
//
// Example:
// Say we have a function:
// Vector2d ComputeCentroid(Iterator first, Iterator last);
// where the iterators are assumed to point to Vector2d values.
// But we are given a proto repeated field RepeatedPtrField<Vec2Proto>, then
// we can do this:
//
//   struct ConvertVector {
//     Vector2d operator()(const Vec2Proto& v) const {
//       return Vector2d(v.x(), v.y());
//     }
//   }
//
//   Vector2d ComputeCentroid(const RepeatedPtrField<Vec2Proto>& proto) {
//     return ComputeCentroid(
//         MakeTransformIterator(proto.begin(), ConvertVector{}),
//         MakeTransformIterator(proto.end(), ConvertVector{}));
//   }
//
template <typename UnderlyingIter, typename UnaryFunc>
class TransformIterator
    : public IteratorFacade<
          TransformIterator<UnderlyingIter, UnaryFunc>,
          decltype(std::declval<const UnaryFunc>()(
              *std::declval<UnderlyingIter>())),
          typename std::iterator_traits<UnderlyingIter>::iterator_category> {
 public:
  // Universal constructor:
  template <typename Iter2>
  TransformIterator(Iter2&& it, const UnaryFunc* f)
      : it_(std::forward<Iter2>(it)), f_(f) {}

  // Default constructor:
  TransformIterator() : it_(), f_(nullptr) {}

  // Returns the underlying iterator, removing the top-most transform layer.
  UnderlyingIter base() { return it_; }

 private:
  friend class IteratorFacadePrivateAccess<TransformIterator>;

  // Implementation of the IteratorFacade requirements:
  decltype(auto) Dereference() const { return (*f_)(*it_); }
  void Increment() { ++it_; }
  void Decrement() { --it_; }
  bool IsEqual(const TransformIterator& rhs) const { return it_ == rhs.it_; }
  int DistanceTo(const TransformIterator& rhs) const { return rhs.it_ - it_; }
  void Advance(int n) { it_ += n; }

  UnderlyingIter it_;
  const UnaryFunc* f_;
};

template <typename BaseRange, typename UnaryFunc>
class TransformedRange
    : public AliasRangeFacade<
          TransformedRange<BaseRange, UnaryFunc>, BaseRange,
          TransformIterator<RangeIteratorType<BaseRange>, UnaryFunc>> {
 public:
  using TransIter = TransformIterator<RangeIteratorType<BaseRange>, UnaryFunc>;
  using BaseFacade = AliasRangeFacade<TransformedRange<BaseRange, UnaryFunc>,
                                      BaseRange, TransIter>;

  // Constructor from a Range
  template <typename OtherRange, typename OtherFunc>
  explicit TransformedRange(OtherRange&& r, OtherFunc&& f)
      : BaseFacade(std::forward<OtherRange>(r)),
        f_(std::forward<OtherFunc>(f)) {}

  // Default assignment operator
  TransformedRange& operator=(const TransformedRange&) = default;
  TransformedRange& operator=(TransformedRange&&) = default;
  TransformedRange(const TransformedRange&) = default;
  TransformedRange(TransformedRange&&) = default;

 private:
  friend class AliasRangeFacadePrivateAccess<
      TransformedRange<BaseRange, UnaryFunc>>;

  auto Begin(const BaseRange& base_range) const {
    using std::begin;
    return TransIter(begin(base_range), &f_);
  }
  auto End(const BaseRange& base_range) const {
    using std::end;
    return TransIter(end(base_range), &f_);
  }

  UnaryFunc f_;
};

// Factory function that conveniently creates a transform iterator range
// using template argument deduction to infer the type of the underlying
// iterator and unary functor.
// range: The underlying range.
// first, last: The underlying iterators for the range.
// f: The functor to convert from decltype(*it) to decltype(f(*it)).
template <typename Range, typename UnaryFunc>
auto TransformRange(Range&& range, UnaryFunc&& f) {
  return TransformedRange<decltype(MoveOrAliasRange(
                              std::forward<Range>(range))),
                          std::decay_t<UnaryFunc>>(
      MoveOrAliasRange(std::forward<Range>(range)), std::forward<UnaryFunc>(f));
}

template <typename BaseIter, typename UnaryFunc>
auto TransformRange(BaseIter&& first, BaseIter&& last, UnaryFunc&& f) {
  return TransformRange(MakeIteratorRange(first, last),
                        std::forward<UnaryFunc>(f));
}

namespace transform_iterator_detail {

// Functor to select first member of a std::pair:
struct SelectFirstMember {
  template <typename T>
  decltype(auto) operator()(T&& value) const {
    return std::get<0>(value);  // value.first does not preserve cv-qualifiers.
  }
};

// Functor to select second member of a std::pair:
struct SelectSecondMember {
  template <typename T>
  decltype(auto) operator()(T&& value) const {
    return std::get<1>(value);  // value.second does not preserve cv-qualifiers.
  }
};

// Functor to do one additional dereferencing (e.g., container of pointers):
struct DereferenceValue {
  template <typename T>
  decltype(auto) operator()(T&& value) const {
    return *value;
  }
};

// Functor to select a member of a class:
template <auto MemberPointer>
struct SelectMember {
  static_assert(std::is_member_object_pointer_v<decltype(MemberPointer)>);
  template <typename T>
  decltype(auto) operator()(T&& value) const {
    return std::forward<T>(value).*MemberPointer;
  }
};

// Functor to do one additional dereferencing (e.g., container of pointers):
template <typename DestType>
struct StaticCastToType {
  template <typename T>
  decltype(auto) operator()(T&& value) const {
    return static_cast<DestType>(value);
  }
};

}  // namespace transform_iterator_detail

// Create a range of transform iterators that extract the first member in a
// range where the elements point to std::pair objects.
// For instance, this can be used to transform iterators into a std::map or
// std::unordered_map into iterators that dereference to the key object.
template <typename Range>
auto RangeOfFirstMember(Range&& range) {
  return TransformRange(std::forward<Range>(range),
                        transform_iterator_detail::SelectFirstMember());
}

// Create a range of transform iterators that extract the first member in a
// range where the elements point to std::pair objects.
template <typename BaseIter>
auto RangeOfFirstMember(BaseIter&& first, BaseIter&& last) {
  return TransformRange(std::forward<BaseIter>(first),
                        std::forward<BaseIter>(last),
                        transform_iterator_detail::SelectFirstMember());
}

// Create a range of transform iterators that extract the second member in a
// range where the elements point to std::pair objects.
// For instance, this can be used to transform iterators into a std::map or
// std::unordered_map into iterators that dereference to the value object.
template <typename Range>
auto RangeOfSecondMember(Range&& range) {
  return TransformRange(std::forward<Range>(range),
                        transform_iterator_detail::SelectSecondMember());
}

// Create a range of transform iterators that extract the second member in a
// range where the elements point to std::pair objects.
template <typename BaseIter>
auto RangeOfSecondMember(BaseIter&& first, BaseIter&& last) {
  return TransformRange(std::forward<BaseIter>(first),
                        std::forward<BaseIter>(last),
                        transform_iterator_detail::SelectSecondMember());
}

// Create a range of transform iterators that performs an additional
// dereferencing of the value (e.g. pointer) obtained from a range.
// For instance, this can be used to make iterators into a container of
// pointers look like iterators into a container of the values these
// pointers point to.
template <typename Range>
auto RangeWithDereference(Range&& range) {
  return TransformRange(std::forward<Range>(range),
                        transform_iterator_detail::DereferenceValue());
}

// Create a range of transform iterators that performs an additional
// dereferencing of the value (e.g. pointer) obtained from a range.
template <typename BaseIter>
auto RangeWithDereference(BaseIter&& first, BaseIter&& last) {
  return TransformRange(std::forward<BaseIter>(first),
                        std::forward<BaseIter>(last),
                        transform_iterator_detail::DereferenceValue());
}

// Create a range of transform iterators that extract the given data member
// from  the elements obtained from a range.
// This can be used to create a range to a member like this:
//   auto members = RangeOfMember<&Foo::member>(foo_vector);
template <auto MemberPointer, typename Range>
auto RangeOfMember(Range&& range) {
  return TransformRange(
      std::forward<Range>(range),
      transform_iterator_detail::SelectMember<MemberPointer>());
}

// Create a range of transform iterators that extract the given data member
// from  the elements obtained from a range of iterators.
template <auto MemberPointer, typename BaseIter>
auto RangeOfMember(BaseIter&& first, BaseIter&& last) {
  return TransformRange(
      std::forward<BaseIter>(first), std::forward<BaseIter>(last),
      transform_iterator_detail::SelectMember<MemberPointer>());
}

// Create a range of iterators that can be used to iterate over a number of
// contiguous enum values of some type.
// CAVEAT: This function requires the enum type to have contiguous values
// without gaps, and the underlying type should be `int` (vanilla enum type).
template <typename EnumType>
auto RangeOfEnumValues(EnumType from, EnumType to) {
  return TransformRange(
      IndexRange(static_cast<int>(from), static_cast<int>(to)),
      transform_iterator_detail::StaticCastToType<EnumType>());
}

// Same as RangeOfEnumValues, except the range is inclusive of the last element
// since often enum values don't have a convenient one-past-last value.
template <typename EnumType>
auto InclusiveRangeOfEnumValues(EnumType from, EnumType to) {
  return TransformRange(
      IndexRange(static_cast<int>(from), static_cast<int>(to) + 1),
      transform_iterator_detail::StaticCastToType<EnumType>());
}

}  // namespace genit

#endif  // GENIT_TRANSFORM_ITERATOR_H_
