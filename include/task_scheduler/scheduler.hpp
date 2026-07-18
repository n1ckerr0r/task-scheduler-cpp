#pragma once

#include "task_scheduler/any.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace task_scheduler {

class Scheduler {
private:
  enum class VisitState : std::uint8_t { kNotVisited, kVisiting, kDone };

  struct BaseTask {
    virtual ~BaseTask() = default;

    [[nodiscard]] virtual auto GetResult() const -> const Any & = 0;
    [[nodiscard]] virtual auto Dependencies() const
        -> const std::vector<size_t> & = 0;
    [[nodiscard]] virtual auto IsExecuted() const -> bool = 0;
    virtual void Invoke() = 0;
  };

public:
  template <typename T> class FutureResult {
  public:
    FutureResult(Scheduler *owner, size_t task_id)
        : owner_(owner), task_id_(task_id) {}

    operator T() const { return owner_->getResult<T>(task_id_); }

    [[nodiscard]] auto GetId() const -> size_t { return task_id_; }

  private:
    Scheduler *owner_;
    size_t task_id_;
  };

private:
  template <typename Func, typename... Args> struct Task final : BaseTask {
    Task(Func function, Args... args)
        : function_(std::move(function)), args_(args...) {
      CollectDependencies(args...);
    }

    [[nodiscard]] auto GetResult() const -> const Any & override {
      return result_;
    }

    [[nodiscard]] auto Dependencies() const -> const std::vector<size_t> &
        override {
      return dependencies_;
    }

    [[nodiscard]] auto IsExecuted() const -> bool override {
      return is_executed_;
    }

    void Invoke() override {
      result_ = std::apply(function_, args_);
      is_executed_ = true;
    }

  private:
    void CollectDependencies() {}

    template <typename First, typename... Rest>
    void CollectDependencies(const First &first, const Rest &...rest) {
      AppendDependency(first);
      CollectDependencies(rest...);
    }

    template <typename T> void AppendDependency(const FutureResult<T> &future) {
      dependencies_.push_back(future.GetId());
    }

    template <typename T> void AppendDependency(const T &value) {
      (void)value;
    }

    Func function_;
    std::tuple<Args...> args_;
    std::vector<size_t> dependencies_;
    Any result_;
    bool is_executed_ = false;
  };

public:
  template <typename Func, typename... Args>
  auto add(Func func, Args... args) -> size_t {
    tasks_.push_back(std::make_shared<Task<Func, Args...>>(std::move(func),
                                                           std::move(args)...));
    task_states_.push_back(VisitState::kNotVisited);
    return next_id_++;
  }

  template <typename T> auto getFutureResult(size_t task_id) -> FutureResult<T> {
    ValidateTaskId(task_id);
    return FutureResult<T>(this, task_id);
  }

  template <typename T> auto getResult(size_t task_id) -> T {
    ValidateTaskId(task_id);
    ComputeTask(task_id);
    return tasks_[task_id]->GetResult().cast<T>();
  }

  void executeAll() {
    for (size_t task_id = 0; task_id < tasks_.size(); ++task_id) {
      ComputeTask(task_id);
    }
  }

  [[nodiscard]] auto size() const -> size_t { return tasks_.size(); }

private:
  void ValidateTaskId(size_t task_id) const {
    if (task_id >= tasks_.size()) {
      throw std::out_of_range("Task id is out of range");
    }
  }

  void ComputeTask(size_t task_id) {
    auto &state = task_states_[task_id];
    if (state == VisitState::kDone || tasks_[task_id]->IsExecuted()) {
      state = VisitState::kDone;
      return;
    }
    if (state == VisitState::kVisiting) {
      throw std::logic_error("Cycle detected in task dependencies");
    }

    state = VisitState::kVisiting;
    for (size_t dependency_id : tasks_[task_id]->Dependencies()) {
      ComputeTask(dependency_id);
    }

    if (!tasks_[task_id]->IsExecuted()) {
      tasks_[task_id]->Invoke();
    }
    state = VisitState::kDone;
  }

  std::vector<std::shared_ptr<BaseTask>> tasks_;
  std::vector<VisitState> task_states_;
  size_t next_id_ = 0;
};

} // namespace task_scheduler
