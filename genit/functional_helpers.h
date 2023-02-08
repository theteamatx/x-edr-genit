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

#ifndef GENIT_FUNCTIONAL_HELPERS_H_
#define GENIT_FUNCTIONAL_HELPERS_H_

#include <tuple>
#include <utility>

namespace genit {

// Calls the supplied functor on dereferenced arguments.
template <typename Functor>
class DereferencingCaller {
 public:
  explicit DereferencingCaller(Functor&& func)
      : func_(std::forward<Functor>(func)) {}

  template <typename... Args>
  auto operator()(Args&&... args) const
      -> decltype(std::declval<Functor>()(*std::forward<Args>(args)...)) {
    return func_(*std::forward<Args>(args)...);
  }

 private:
  Functor func_;
};

#if __cplusplus >= 201703L
// Deduction guide.
template <typename Functor>
DereferencingCaller(Functor&& func)->DereferencingCaller<Functor>;
#endif  // __cplusplus >= 201703L

// This class allows the user to extract the return and arguments types of a
// function or of a method at compile time.
template <typename Callable>
class Signature;

// Specialization for methods.
template <class R, class T, class... Args>
class Signature<R (T::*)(Args...)> {
 public:
  using ReturnType = R;

  static constexpr size_t kNumberOfArguments = sizeof...(Args);

  template <size_t index>
  using Argument =
      typename std::tuple_element<index, std::tuple<Args...>>::type;
};

// Specialization for const methods.
template <class R, class T, class... Args>
class Signature<R (T::*)(Args...) const> {
 public:
  using ReturnType = R;

  static constexpr size_t kNumberOfArguments = sizeof...(Args);

  template <size_t index>
  using Argument =
      typename std::tuple_element<index, std::tuple<Args...>>::type;
};

#if __cplusplus >= 201703L
// Specialization for const& methods.
template <class R, class T, class... Args>
class Signature<R (T::*)(Args...) const&> {
 public:
  using ReturnType = R;

  static constexpr size_t kNumberOfArguments = sizeof...(Args);

  template <size_t index>
  using Argument =
      typename std::tuple_element<index, std::tuple<Args...>>::type;
};

// Specialization for const&& methods.
template <class R, class T, class... Args>
class Signature<R (T::*)(Args...) const&&> {
 public:
  using ReturnType = R;

  static constexpr size_t kNumberOfArguments = sizeof...(Args);

  template <size_t index>
  using Argument =
      typename std::tuple_element<index, std::tuple<Args...>>::type;
};
#endif  // __cplusplus >= 201703L

// Specialization for functions.
template <class R, class... Args>
class Signature<R (*)(Args...)> {
 public:
  using ReturnType = R;

  static constexpr size_t kNumberOfArguments = sizeof...(Args);

  template <size_t index>
  using Argument =
      typename std::tuple_element<index, std::tuple<Args...>>::type;
};

}  // namespace genit

#endif  // GENIT_FUNCTIONAL_HELPERS_H_
