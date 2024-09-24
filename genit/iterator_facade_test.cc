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

#include "genit/iterator_facade.h"

#include <iterator>
#include <type_traits>

#include "gtest/gtest.h"

namespace genit {

namespace {

class CounterIterator : public IteratorFacade<CounterIterator, int,
                                              std::random_access_iterator_tag> {
 public:
  CounterIterator() = default;
  explicit CounterIterator(int n) : i_(n) {}

 private:
  friend class IteratorFacadePrivateAccess<CounterIterator>;

  int Dereference() const { return i_; }
  void Increment() { ++i_; }
  void Decrement() { --i_; }
  bool IsEqual(const CounterIterator& rhs) const { return i_ == rhs.i_; }
  int DistanceTo(const CounterIterator& rhs) const { return rhs.i_ - i_; }
  void Advance(int n) { i_ += n; }

  int i_ = 0;
};

class ConstCounterIterator
    : public IteratorFacade<ConstCounterIterator, const int&,
                            std::random_access_iterator_tag> {
 public:
  ConstCounterIterator() = default;
  explicit ConstCounterIterator(int n) : i_(n) {}

  operator CounterIterator() const { return CounterIterator(i_); }  // NOLINT

 private:
  friend class IteratorFacadePrivateAccess<ConstCounterIterator>;

  const int& Dereference() const { return i_; }
  void Increment() { ++i_; }
  void Decrement() { --i_; }
  bool IsEqual(const ConstCounterIterator& rhs) const { return i_ == rhs.i_; }
  int DistanceTo(const ConstCounterIterator& rhs) const { return rhs.i_ - i_; }
  void Advance(int n) { i_ += n; }

  int i_ = 0;
};

class MovePtrIterator : public IteratorFacade<MovePtrIterator, int&&,
                                              std::random_access_iterator_tag> {
 public:
  explicit MovePtrIterator(int* ptr = nullptr) : ptr_(ptr) {}

 private:
  friend class IteratorFacadePrivateAccess<MovePtrIterator>;

  int&& Dereference() const { return static_cast<int&&>(*ptr_); }
  void Increment() { ++ptr_; }
  void Decrement() { --ptr_; }
  bool IsEqual(const MovePtrIterator& rhs) const { return ptr_ == rhs.ptr_; }
  int DistanceTo(const MovePtrIterator& rhs) const { return rhs.ptr_ - ptr_; }
  void Advance(int n) { ptr_ += n; }

  int* ptr_ = nullptr;
};

class ForwardIterator
    : public IteratorFacade<ForwardIterator, int, std::forward_iterator_tag> {
 public:
  explicit ForwardIterator(int* ptr = nullptr) : ptr_(ptr) {}

 private:
  friend class IteratorFacadePrivateAccess<ForwardIterator>;

  int Dereference() const { return *ptr_; }
  void Increment() { ++ptr_; }
  bool IsEqual(const ForwardIterator& rhs) const { return ptr_ == rhs.ptr_; }

  int* ptr_ = nullptr;
};

class OutputIterator
    : public IteratorFacade<OutputIterator, int&, std::output_iterator_tag> {
 public:
  explicit OutputIterator(int* ptr = nullptr) : ptr_(ptr) {}

 private:
  friend class IteratorFacadePrivateAccess<OutputIterator>;

  int& Dereference() const { return *ptr_; }
  void Increment() { ++ptr_; }
  bool IsEqual(const OutputIterator& rhs) const { return ptr_ == rhs.ptr_; }

  int* ptr_ = nullptr;
};

class BidirectionalIterator
    : public IteratorFacade<BidirectionalIterator, int,
                            std::bidirectional_iterator_tag> {
 public:
  explicit BidirectionalIterator(int* ptr = nullptr) : ptr_(ptr) {}

 private:
  friend class IteratorFacadePrivateAccess<BidirectionalIterator>;

  int Dereference() const { return *ptr_; }
  void Increment() { ++ptr_; }
  void Decrement() { --ptr_; }
  bool IsEqual(const BidirectionalIterator& rhs) const {
    return ptr_ == rhs.ptr_;
  }

  int* ptr_ = nullptr;
};

TEST(IteratorFacadeTest, CounterIterator) {
  CounterIterator it;
  CounterIterator it_end(5);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_EQ(*it, 0);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(it[4], 4);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), 2);
  EXPECT_EQ(*(3 + it), 3);
  EXPECT_EQ(*(it - 4), -4);

  it += 5;
  EXPECT_EQ(*it, 5);
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(IteratorFacadeTest, IteratorConversions) {
  CounterIterator it_value;
  ConstCounterIterator it_const;

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it_const)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it_const)>);

  EXPECT_TRUE(it_value == it_const);
  EXPECT_FALSE(it_const != it_value);
  EXPECT_FALSE(it_value < it_const);
  EXPECT_TRUE(it_const <= it_value);
  EXPECT_FALSE(it_value > it_const);
  EXPECT_TRUE(it_const >= it_value);

  it_const += 2;
  EXPECT_EQ(it_const - it_value, 2);
  it_value += 5;
  EXPECT_EQ(it_value - it_const, 3);
}

TEST(IteratorFacadeTest, MoveIterator) {
  int arr[] = {0, 1, 2, 3, 4};
  MovePtrIterator it(arr);
  MovePtrIterator it_end(arr + 5);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
  EXPECT_TRUE(it < it_end);
  EXPECT_TRUE(it <= it_end);
  EXPECT_FALSE(it > it_end);
  EXPECT_FALSE(it >= it_end);

  EXPECT_TRUE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_EQ(*it, 0);
  EXPECT_EQ(it[0], 0);
  EXPECT_EQ(it[1], 1);
  EXPECT_EQ(it[4], 4);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  EXPECT_EQ(it_end - it, 5);
  EXPECT_EQ(*(it + 2), 2);
  EXPECT_EQ(*(3 + it), 3);

  it += 5;
  EXPECT_EQ(*(it - 3), 2);
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);

  it -= 5;
  EXPECT_EQ(*it, 0);
  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);
}

TEST(IteratorFacadeTest, ForwardIterator) {
  int arr[] = {0, 1, 2, 3, 4};
  ForwardIterator it(arr);
  ForwardIterator it_end(arr + 5);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_EQ(*it, 0);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  ++it;
  ++it;
  ++it;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
}

TEST(IteratorFacadeTest, OutputIterator) {
  int arr[] = {0, 1, 2, 3, 4};
  OutputIterator it(arr);
  OutputIterator it_end(arr + 5);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_TRUE(std::is_lvalue_reference_v<decltype(*it)>);

  *it = 10;
  EXPECT_EQ(arr[0], 10);

  ++it;
  *it = 11;
  EXPECT_EQ(arr[1], 11);
  *it++ = 110;
  EXPECT_EQ(arr[1], 110);
  *it = 12;
  EXPECT_EQ(arr[2], 12);

  ++it;
  ++it;
  ++it;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
}

TEST(IteratorFacadeTest, BidirectionalIterator) {
  int arr[] = {0, 1, 2, 3, 4};
  BidirectionalIterator it(arr);
  BidirectionalIterator it_end(arr + 5);

  EXPECT_FALSE(it == it_end);
  EXPECT_TRUE(it != it_end);

  EXPECT_FALSE(std::is_rvalue_reference_v<decltype(*it)>);
  EXPECT_FALSE(std::is_lvalue_reference_v<decltype(*it)>);

  EXPECT_EQ(*it, 0);

  ++it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it++), 1);
  EXPECT_EQ(*it, 2);

  --it;
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);

  ++it;
  ++it;
  ++it;
  ++it;
  ++it;
  EXPECT_TRUE(it == it_end);
  EXPECT_FALSE(it != it_end);
}

}  // namespace
}  // namespace genit
