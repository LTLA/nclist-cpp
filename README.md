# Nested containment lists in C++

![Unit tests](https://github.com/LTLA/nclist-cpp/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/LTLA/nclist-cpp/actions/workflows/doxygenate.yaml/badge.svg)
[![Codecov](https://codecov.io/gh/LTLA/nclist-cpp/branch/master/graph/badge.svg?token=GByG4StuqU)](https://codecov.io/gh/LTLA/nclist)

## Overview

This header-only library implements the [nested containment list (NCList)](https://doi.org/10.1093/bioinformatics/btl647) algorithm for interval queries.
Specifically, the aim is to find all "subject" intervals that overlap a "query" interval, which is a common task when analyzing genomics data.
The interface and capabilities of this library are based on the `findOverlaps()` function from the [**IRanges**](https://bioconductor.org/packages/IRanges) R package.
We support overlaps with a maximum gap or minimum overlap, as well as overlaps of different types - matching starts/ends, within/extends, etc.

## Quick start

Given an array of starts/ends for the subject intervals, we build the nested containment list:

```cpp
// The i-th subject interval is defined as '[starts[i], ends[i])'.
// Note that the end is not inclusive.
std::vector<int> starts { 5, 10, 20 };
std::vector<int> ends   { 8, 25, 30 };

auto subjects = nclist::build(3, starts.data(), ends.data());
```

Then we check for overlaps with our query interval:

```cpp
nclist::OverlapsAnyWorkspace<int> workspace;
nclist::OverlapsAnyParameters<int> params;
std::vector<int> matches;

// Checking for overlaps to `[6, 16)`.
// 'matches' is filled with the indices of the matching subject interval.
nclist::overlaps_any(subjects, 6, 16, params, workspace, matches);

// Performing another query.
// The workspace is re-used to avoid reallocation of internal structures.
nclist::overlaps_any(subjects, 25, 30, params, workspace, matches);
```

Check out the [reference documentation](https://ltla.github.io/nclist-cpp) for more details.

## Setting parameters

Say we only want to report overlaps where the length of the overlapping subinterval is not below some threshold.
We can do so with the `min_overlaps=` parameter:

```cpp
params.min_overlaps = 10;
nclist::overlaps_any(subjects, 20, 30, params, workspace, matches);
```

We can also detect "overlaps" where the query and subject intervals are separated by a gap that is no greater than some threshold:

```cpp
// 'max_gap = 0' means that contiguous intervals are considered "overlapping".
params.max_gap = 0;
nclist::overlaps_any(subjects, 1, 5, params, workspace, matches);

params.max_gap = 10;
nclist::overlaps_any(subjects, 40, 50, params, workspace, matches);
```

In some cases, we only want to determine if any overlap exists, without concern for the identity of the overlapping subject intervals.
This can be achieved with the `quit_on_first=` parameter, which will return upon finding the first overlapping interval for greater efficiency.

```cpp
params.quit_on_first = true;
nclist::overlaps_any(subjects, 0, 50, params, workspace, matches);
matches.empty();
```

## Overlap types

The `overlaps_any()` function will look for any overlaps between the query and subject intervals.
However, other functions can also be used to report different types of overlaps.
Perhaps we want to consider overlaps where the query interval lies "within" (i.e., is a subinterval of) a subject interval:

```cpp
nclist::OverlapsWithinWorkspace<int> wworkspace;
nclist::OverlapsWithinParameters<int> wparams;
nclist::overlaps_within(subjects, 20, 28, wparams, wworkspace, matches);
```

Or we only care about those subject intervals with the same start position:

```cpp
nclist::OverlapsStartWorkspace<int> wworkspace;
nclist::OverlapsStartParameters<int> wparams;
nclist::overlaps_start(subjects, 10, 50, wparams, wworkspace, matches);
```

And so on.
This functionality is inspired by the `type=` argument in the `findOverlaps()` function from the **IRanges** package.
Note that the interpretation of some parameters (e.g., `max_gap`) depends on the type of overlap,
so be sure to consult the [relevant documentation](https://ltla.github.io/nclist-cpp).

## Position types

This library will work with double-precision coordinates for the interval coordinates:

```cpp
// Again, remember that 'ends' are not inclusive.
std::vector<double> starts { 5.5, 10.1, 20.5 };
std::vector<double> ends   { 8.5, 25.2, 30.3 };
auto subjects = nclist::build(3, starts.data(), ends.data());

nclist::OverlapsAnyWorkspace<int> workspace;
nclist::OverlapsAnyParameters<double> params;
std::vector<int> matches;

nclist::overlaps_any(subjects, 14.4, 29.5, params, workspace, matches);
```

The subject interval indices (used to store the overlap results in `matches`) can also be changed from `int` to other integer types like `std::size_t`.
Larger types may be preferred if there are more intervals than can be represented by `int`, at the cost of some increased memory usage.

## Custom subject intervals

The `build_custom()` function accepts subject interval coordinates in formats other than a C-style array.
For example, we might have stored the coordinates in a `std::deque` for more efficient expansion:

```cpp
std::deque<int> dq_starts { 5, 10, 20 };
std::deque<int> dq_ends   { 8, 25, 30 };
auto dq_subjects = nclist::build_custom(3, dq_starts, dq_ends);
```

A more interesting application involves adjusting the coordinates without allocating a new array.
For example, many genomic intervals are reported with inclusive ends (e.g., in GFF and SAM files) but `build()` expects exclusive ends.
This is accommodated in `build_custom()` by creating a custom class that increments the end position on the fly:

```cpp
struct Incrementer {
    Incrementer(const std::vector<int>& v) : original(v) {}
    int operator[](int i) const { return original[i] + 1; }
    const std::vector<int>& original;
};

auto inc_subjects = nclist::build_custom(3, starts, Incrementer(ends));
```

## Building projects 

### CMake with `FetchContent`

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  nclist
  GIT_REPOSITORY https://github.com/LTLA/nclist
  GIT_TAG master # replaced with a pinned version
)

FetchContent_MakeAvailable(nclist)
```

Then you can link to **nclist** to make the headers available during compilation:

```cmake
# For executables:
target_link_libraries(myexe nclist)

# For libaries
target_link_libraries(mylib INTERFACE nclist)
```

### CMake with `find_package()`

You can install the library by cloning a suitable version of this repository and running the following commands:

```sh
mkdir build && cd build
cmake .. -DNCLIST_TESTS=OFF
cmake --build . --target install
```

Then you can use `find_package()` as usual:

```cmake
find_package(ltla_nclist CONFIG REQUIRED)
target_link_libraries(mylib INTERFACE ltla::nclist)
```

### Manual

If you're not using CMake, the simple approach is to just copy the files in the `include/` subdirectory - 
either directly or with Git submodules - and include their path during compilation with, e.g., GCC's `-I`.
