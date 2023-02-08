Generic Iterators Library (genit)

This repository contains a collection of generic iterators and ranges that can
be used to create various transformations on basic iterators. For example:
* Transform the value returned by an iterator via a functor.
* Filter out elements of a range based on a predicate.
* Combine multiple iterators together ("zip").
* Access a member variable of the object returned by the base iterator.
* Create adjacency ranges (e.g., iterate through pairs).
* Create circular ranges that loop around the base range.

All the code is written in C++17 code and tries to comply with iterator
requirements, but may deviate when necessary.
