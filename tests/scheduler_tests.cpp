#include "task_scheduler/any.hpp"
#include "task_scheduler/scheduler.hpp"

#include <cassert>
#include <stdexcept>

namespace {

constexpr int kSourceValue = 10;
constexpr int kAdditionValue = 5;
constexpr int kUnrelatedResult = 99;
constexpr int kCachedResult = 42;
constexpr int kAssignedIntegerValue = 7;
constexpr int kExecuteAllResult = 123;

struct Counter {
  int calls = 0;
  int offset = 1;

  auto IncrementAndReturn(int value) -> int {
    ++calls;
    return value;
  }

  [[nodiscard]] auto AddConst(int value) const -> int { return value + offset; }
};

void TestGetResultExecutesOnlyDependencies() {
  task_scheduler::Scheduler scheduler;
  int unrelated_calls = 0;

  const auto source_task_id =
      scheduler.add([]() -> int { return kSourceValue; });
  const auto dependent_task_id = scheduler.add(
      [](int value) -> int { return value + kAdditionValue; },
      scheduler.getFutureResult<int>(source_task_id));
  scheduler.add([&unrelated_calls]() -> int {
    ++unrelated_calls;
    return kUnrelatedResult;
  });

  assert(scheduler.getResult<int>(dependent_task_id) ==
         (kSourceValue + kAdditionValue));
  assert(unrelated_calls == 0);
}

void TestTaskResultIsCached() {
  task_scheduler::Scheduler scheduler;
  Counter counter;

  const auto cached_task_id =
      scheduler.add(&Counter::IncrementAndReturn, &counter, kCachedResult);

  assert(scheduler.getResult<int>(cached_task_id) == kCachedResult);
  assert(scheduler.getResult<int>(cached_task_id) == kCachedResult);
  assert(counter.calls == 1);
}

void TestInvalidTaskIdThrows() {
  task_scheduler::Scheduler scheduler;
  bool caught = false;

  try {
    static_cast<void>(scheduler.getResult<int>(0));
  } catch (const std::out_of_range &) {
    caught = true;
  }

  assert(caught);
}

void TestAnyEmptyCastThrows() {
  task_scheduler::Any value;
  bool caught = false;

  try {
    static_cast<void>(value.cast<int>());
  } catch (const task_scheduler::Any::BadAnyCast &) {
    caught = true;
  }

  assert(caught);
}

void TestAnyWrongTypeThrows() {
  task_scheduler::Any value;
  value = kAssignedIntegerValue;
  bool caught = false;

  try {
    static_cast<void>(value.cast<float>());
  } catch (const task_scheduler::Any::BadAnyCast &) {
    caught = true;
  }

  assert(caught);
}

void TestGetFutureResultRejectsInvalidTaskId() {
  task_scheduler::Scheduler scheduler;
  bool caught = false;

  try {
    static_cast<void>(scheduler.getFutureResult<int>(0));
  } catch (const std::out_of_range &) {
    caught = true;
  }

  assert(caught);
}

void TestExecuteAllExecutesTaskOnce() {
  task_scheduler::Scheduler scheduler;
  int invocation_count = 0;

  const auto task_id = scheduler.add([&invocation_count]() -> int {
    ++invocation_count;
    return kExecuteAllResult;
  });

  scheduler.executeAll();
  scheduler.executeAll();

  assert(scheduler.getResult<int>(task_id) == kExecuteAllResult);
  assert(invocation_count == 1);
}

void TestMemberFunctionPointerOnConstObject() {
  task_scheduler::Scheduler scheduler;
  const Counter counter{};

  const auto task_id = scheduler.add(&Counter::AddConst, counter, 9);

  assert(scheduler.getResult<int>(task_id) == 10);
}

} // namespace

auto main() -> int {
  // Each test is intentionally tiny: the process exits with a non-zero code if
  // any assert fails, so CTest treats it as a failed test run.
  try {
    TestGetResultExecutesOnlyDependencies();
    TestTaskResultIsCached();
    TestInvalidTaskIdThrows();
    TestAnyEmptyCastThrows();
    TestAnyWrongTypeThrows();
    TestGetFutureResultRejectsInvalidTaskId();
    TestExecuteAllExecutesTaskOnce();
    TestMemberFunctionPointerOnConstObject();
  } catch (const std::exception &) {
    return 1;
  }
  return 0;
}
