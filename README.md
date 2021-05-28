# sexplib
A simple header-only library for parsing s-expressions to C++ structures

## Dependencies
Only [catch2](https://github.com/catchorg/Catch2) if you intend to build tests.

## Installation
Copy `include/sexplib.hh` to your project, or use the `meson` install target (will also build, but not install, tests unless configured otherwise).

## Usage
The main functionality is the `sexp::parse` function, which takes a string containing your s-expression (a single top-level s-expression) and is generic over types implementing the `sexp::Deserializable` concept.

`parse` will build the `Deserializable` datatype you call it with from your s-expression, and return the result. Precisely how this datatype is built depends on your implementation of the functions required by `Deserializable`; this is the "coolness" of `sexplib`, that it makes it easy to parse s-expressions into specialized structures.

`sexplib` provides two example implementations: `sexp::Sexp` is the recommended default, and uses `std::optional` with a `head` atom and a `tail` list of `Sexp` to model the s-expression. If enabled by including `#define SEXPLIB_USE_VECTORSEXP`, the `sexp::VectorSexp` type is an alternative, based around `std::variant`. See `sexplib.hh` or the files in `test/` for examples of use.
