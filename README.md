# Task Scheduler C++

Header-only C++23 library for building and executing small dependency graphs of tasks.

> **Note:** this is a pet project, created for learning and experimentation. It is not intended for production use.

The library lets you:
- add tasks with zero, one, or two arguments;
- reference one task result from another task;
- compute only the subgraph that is actually needed;
- execute the whole graph on demand;
- cache completed results;
- validate task ids at runtime.

The main public type is `task_scheduler::Scheduler` from `task_scheduler/scheduler.hpp`.

## Public headers

Preferred public includes:

```cpp
#include <task_scheduler/scheduler.hpp>
#include <task_scheduler/any.hpp>
```

## Why `Any` exists here

Different tasks may return different result types:
- one task returns `int`;
- another returns `float`;
- another returns a user-defined type.

The scheduler stores all tasks in one container, so it also needs one common way to store all task results. `task_scheduler::Any` solves exactly that problem: it erases the concrete result type while the task is stored, then restores the expected type when `getResult<T>()` is called.

## Project layout

- `include/task_scheduler/scheduler.hpp`: public scheduler API
- `include/task_scheduler/any.hpp`: public type-erasure helper
- `tests/scheduler_tests.cpp`: regression tests
- `cmake/task-scheduler-config.cmake.in`: CMake package config template

## Requirements

- C++23 compiler
- CMake 3.15 or newer if you use the provided build files

## Using the library

This project is header-only. There is no static or shared library to build for normal use.

### Option 1: add the repository with CMake

```cmake
add_subdirectory(external/task-scheduler-cpp)

target_link_libraries(your_target PRIVATE task_scheduler::scheduler)
```

The target only propagates include directories and compile features.

### Option 2: install and use `find_package`

Install:

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix /your/prefix
```

Then in another project:

```cmake
find_package(task-scheduler CONFIG REQUIRED)

target_link_libraries(your_target PRIVATE task_scheduler::scheduler)
```

### Option 3: copy headers directly

Add `include/` to your include path and include:

```cpp
#include <task_scheduler/scheduler.hpp>
```

## Minimal example

```cpp
#include <iostream>
#include <task_scheduler/scheduler.hpp>

auto main() -> int {
  task_scheduler::Scheduler scheduler;

  const auto base_task_id = scheduler.add([]() -> int { return 21; });
  const auto doubled_task_id = scheduler.add(
      [](int value) -> int { return value * 2; },
      scheduler.getFutureResult<int>(base_task_id));

  std::cout << scheduler.getResult<int>(doubled_task_id) << '\n';
  return 0;
}
```

## Real example

```cpp
#include <cmath>
#include <iostream>
#include <task_scheduler/scheduler.hpp>

struct ShiftValue {
  [[nodiscard]] auto add(float value) const -> float { return value + shift; }
  float shift;
};

auto main() -> int {
  constexpr float kCoefficientA = 1.0F;
  constexpr float kCoefficientB = -2.0F;
  constexpr float kCoefficientC = 0.0F;
  constexpr float kNegativeFour = -4.0F;
  constexpr float kTwo = 2.0F;

  task_scheduler::Scheduler scheduler;
  ShiftValue shift_value{.shift = 3.0F};

  const auto discriminant_tail_task_id =
      scheduler.add([](float lhs, float rhs) -> float {
        return kNegativeFour * lhs * rhs;
      }, kCoefficientA, kCoefficientC);

  const auto discriminant_task_id = scheduler.add(
      [](float coefficient, float value) -> float {
        return (coefficient * coefficient) + value;
      },
      kCoefficientB,
      scheduler.getFutureResult<float>(discriminant_tail_task_id));

  const auto first_numerator_task_id = scheduler.add(
      [](float coefficient, float discriminant) -> float {
        return -coefficient + std::sqrt(discriminant);
      },
      kCoefficientB, scheduler.getFutureResult<float>(discriminant_task_id));

  const auto second_numerator_task_id = scheduler.add(
      [](float coefficient, float discriminant) -> float {
        return -coefficient - std::sqrt(discriminant);
      },
      kCoefficientB, scheduler.getFutureResult<float>(discriminant_task_id));

  const auto first_root_task_id = scheduler.add(
      [](float coefficient, float value) -> float {
        return value / (kTwo * coefficient);
      },
      kCoefficientA,
      scheduler.getFutureResult<float>(first_numerator_task_id));

  const auto second_root_task_id = scheduler.add(
      [](float coefficient, float value) -> float {
        return value / (kTwo * coefficient);
      },
      kCoefficientA,
      scheduler.getFutureResult<float>(second_numerator_task_id));

  const auto shifted_root_task_id =
      scheduler.add(&ShiftValue::add, shift_value,
                    scheduler.getFutureResult<float>(second_root_task_id));

  scheduler.executeAll();

  std::cout << "x1 = " << scheduler.getResult<float>(first_root_task_id) << '\n';
  std::cout << "x2 = " << scheduler.getResult<float>(second_root_task_id) << '\n';
  std::cout << "x3 = " << scheduler.getResult<float>(shifted_root_task_id) << '\n';
  return 0;
}
```

## API overview

### `task_scheduler::Scheduler`

Main scheduler type.

### `add`

```cpp
const auto task_id = scheduler.add(callable, args...);
```

Adds a task and returns its numeric identifier.

Supported callables:
- lambdas;
- free functions;
- functors;
- member function pointers.

Arguments may be plain values or `getFutureResult<T>(other_task_id)`.

### `getFutureResult<T>`

```cpp
const auto future = scheduler.getFutureResult<int>(task_id);
```

Creates a lightweight placeholder for another task result. No computation happens yet.

### `getResult<T>`

```cpp
const auto value = scheduler.getResult<int>(task_id);
```

Computes only the requested task and the dependency chain required for it. Already-computed results are reused.

### `executeAll`

```cpp
scheduler.executeAll();
```

Computes every registered task exactly once.

### `size`

```cpp
const auto count = scheduler.size();
```

Returns the number of registered tasks.

## Error behavior

The library throws:

- `std::out_of_range`: task id does not exist
- `std::logic_error`: cyclic dependency detected during execution
- `task_scheduler::Any::BadAnyCast`: stored result type does not match the requested type, or the value is empty

## Production characteristics

This version is suitable as a small embedded library because it is:
- header-only;
- deterministic;
- lazily evaluated;
- cached after first successful execution;
- packaged for `add_subdirectory` and `find_package`;
- covered by regression tests;
- integrated with `clang-tidy` in CMake.

## Build and test this repository

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install layout

After installation the package provides:

- headers under `<prefix>/include/task-scheduler`
- CMake package files under `<prefix>/lib*/cmake/task-scheduler`
