# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])

exports_files(["LICENSE"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "iterators",
    hdrs = [
        "adjacent_circular_iterator.h",
        "adjacent_iterator.h",
        "bitset_iterator.h",
        "cached_iterator.h",
        "circular_iterator.h",
        "concat_range.h",
        "filter_iterator.h",
        "iterator_facade.h",
        "iterator_range.h",
        "stride_iterator.h",
        "transform_iterator.h",
        "zip_iterator.h",
    ],
    deps = [
        "@com_google_absl//absl/types:variant",
        "@com_google_absl//absl/utility",
    ],
)

cc_test(
    name = "adjacent_circular_iterator_test",
    srcs = [
        "adjacent_circular_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "adjacent_iterator_test",
    srcs = [
        "adjacent_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "cached_iterator_test",
    srcs = [
        "cached_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "circular_iterator_test",
    srcs = [
        "circular_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "concat_range_test",
    srcs = ["concat_range_test.cc"],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "filter_iterator_test",
    srcs = [
        "bitset_iterator_test.cc",
        "filter_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "functional_helpers",
    hdrs = [
        "functional_helpers.h",
    ],
)

cc_test(
    name = "functional_helpers_test",
    srcs = [
        "functional_helpers_test.cc",
    ],
    deps = [
        ":functional_helpers",
        ":iterators",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "iterator_facade_test",
    srcs = [
        "iterator_facade_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "iterator_range_test",
    srcs = [
        "iterator_range_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "stride_iterator_test",
    srcs = [
        "stride_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "transform_iterator_test",
    srcs = [
        "transform_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "zip_iterator_test",
    srcs = [
        "zip_iterator_test.cc",
    ],
    deps = [
        ":iterators",
        "@com_google_googletest//:gtest_main",
    ],
)
