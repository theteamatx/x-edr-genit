// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "genit/functional_helpers.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "genit/iterator_range.h"
#include "genit/transform_iterator.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace genit {
namespace {

using ::testing::ElementsAreArray;

bool IsOdd(int x) { return x % 2 == 1; }

TEST(DereferencingCaller, FunctionPointerPredicateOnIterator) {
  const std::list<int> values = {1, 2, 3};
  auto deref_odd = DereferencingCaller{IsOdd};
  EXPECT_THAT(deref_odd(values.begin()), true);
}

TEST(DereferencingCaller, LambdaPredicateOnPointer) {
  std::unique_ptr<int> values[] = {std::make_unique<int>(1),
                                   std::make_unique<int>(2),
                                   std::make_unique<int>(3)};
  auto is_odd = [](int x) { return IsOdd(x); };
  const int odds_count = std::count_if(std::begin(values), std::end(values),
                                       DereferencingCaller{is_odd});
  EXPECT_THAT(odds_count, 2);
}

TEST(DereferencingCaller, FunctionObjectComparisonOnIterators) {
  const std::list<int> values = {1, 2, 3};
  auto deref_comp = DereferencingCaller(std::less<int>{});
  EXPECT_TRUE(deref_comp(values.begin(), values.end()));
  EXPECT_FALSE(deref_comp(values.end(), values.begin()));
}

bool Less(int lhs, int rhs) { return lhs < rhs; }

TEST(DereferencingCaller, FunctionPointerComparisonOnPointers) {
  std::unique_ptr<int> values[] = {
      std::make_unique<int>(3), std::make_unique<int>(2),
      std::make_unique<int>(1), std::make_unique<int>(4),
      std::make_unique<int>(5), std::make_unique<int>(2),
      std::make_unique<int>(6)};
  std::sort(std::begin(values), std::end(values), DereferencingCaller(Less));
  auto copied_values =
      CopyRange<std::vector<int>>(RangeWithDereference(values));
  EXPECT_THAT(copied_values, ElementsAreArray({1, 2, 2, 3, 4, 5, 6}));
}

class MyClass {
 public:
  bool Method(float a, int b, MyClass* c) { return c == this; }
};

bool FreeFunction(float a, int b) { return a == b; }

// Use cases for the Signature class.
// This test verifies that Signature extracts the right types for a method.
TEST(Signature, MethodSignatureExample) {
  MyClass object;

  Signature<decltype(&MyClass::Method)>::Argument<0> arg_a{};
  Signature<decltype(&MyClass::Method)>::Argument<1> arg_b{};
  Signature<decltype(&MyClass::Method)>::Argument<2> arg_c = &object;

  ASSERT_TRUE(object.Method(arg_a, arg_b, arg_c));
}

// This test verifies that Signature extracts the right types for a function.
TEST(Signature, FunctionSignatureExample) {
  Signature<decltype(&FreeFunction)>::Argument<0> arg_a = 0.0f;
  Signature<decltype(&FreeFunction)>::Argument<1> arg_b = 0;

  ASSERT_TRUE(FreeFunction(arg_a, arg_b));
}

// static_assert tests for the Signature class.

// This test verifies that Signature extracts the right types for a method.
TEST(Signature, MethodSignatureStaticAssert) {
  static_assert(
      std::is_same_v<Signature<decltype(&MyClass::Method)>::Argument<0>, float>,
      "Failed to deduce argument 1's type.");
  static_assert(
      std::is_same_v<Signature<decltype(&MyClass::Method)>::Argument<1>, int>,
      "Failed to deduce argument 2's type.");
  static_assert(
      std::is_same_v<Signature<decltype(&MyClass::Method)>::Argument<2>,
                     MyClass*>,
      "Failed to deduce argument 3's type.");
  static_assert(
      std::is_same_v<Signature<decltype(&MyClass::Method)>::ReturnType, bool>,
      "Failed to deduce argument return type.");
  static_assert(Signature<decltype(&MyClass::Method)>::kNumberOfArguments == 3,
                "Failed to deduce number of arguments.");
}

// This test verifies that Signature extracts the right types for a function.
TEST(Signature, FunctionSignatureStaticAssert) {
  static_assert(
      std::is_same_v<Signature<decltype(&FreeFunction)>::Argument<0>, float>,
      "Failed to deduce argument 1's type.");
  static_assert(
      std::is_same_v<Signature<decltype(&FreeFunction)>::Argument<1>, int>,
      "Failed to deduce argument 2's type.");
  static_assert(
      std::is_same_v<Signature<decltype(&FreeFunction)>::ReturnType, bool>,
      "Failed to deduce argument return type.");
  static_assert(Signature<decltype(&FreeFunction)>::kNumberOfArguments == 2,
                "Failed to deduce number of arguments.");
}

// This test verifies that Signature extracts the right types for a function.
TEST(Signature, AnyInvocableSignatureStaticAssert) {
  absl::AnyInvocable<bool(float, int) const&> func;
  auto invoker = &decltype(func)::operator();
  using FunctionType = decltype(invoker);
  static_assert(std::is_same_v<Signature<FunctionType>::Argument<0>, float>,
                "Failed to deduce argument 1's type.");
  static_assert(std::is_same_v<Signature<FunctionType>::Argument<1>, int>,
                "Failed to deduce argument 2's type.");
  static_assert(std::is_same_v<Signature<FunctionType>::ReturnType, bool>,
                "Failed to deduce argument return type.");
  static_assert(Signature<FunctionType>::kNumberOfArguments == 2,
                "Failed to deduce number of arguments.");
}

}  // namespace
}  // namespace genit
