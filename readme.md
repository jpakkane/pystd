# A self-written C++ standard library

Design goals:

- NOT an implementation of the ISO C++ standard library, but instead
  an experiment to see what a stdlib that no backwards compatibility
  requirements might look like
- design-wise copy the Python standard library's APIs whenever
  possible
- minimize build times
- a single header includes everything
- KISS
- do not sacrifice too much performance
- Can use existing C libraries like pcre but not C++ libraries

## API and ABI stability

Do both of these:

- Maintain perfect API and ABI compatibility
- Make it possible to change both in arbitrary ways

This seems impossible but is in fact fairly easily achievable:

- Put everything under `pystd<year>` namespace

- When release happens, copy all files from `foo<year>.[ch]pp` to
  `foo<year+1>.[ch]pp` and rename the namespace accordingly

- The old version remains forever frozen in time

- Arbitrary changes can be done to the new version

Anyone who needs to interoperate with code using the old versions can
`#include` the old header and use things from its namespace as needed.

## Limitations

Only tested with GCC on x86_64 Linux.
