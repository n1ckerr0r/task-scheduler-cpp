#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
#include <any.h>

class TTaskScheduler {
private:
    struct BaseTask {
        virtual ~BaseTask() = default;
        virtual Any GetResult() = 0;
        virtual size_t GetId() = 0;
        virtual bool isExecute() = 0;
        virtual void invoke() = 0;
    };

    template <typename Func, typename... Args>
    struct Task : public BaseTask {
        Task(Func function, size_t id, Args... args)
                : function_(function), args_(std::make_tuple(args...)), id_(id), is_executed_(false) {}

        Any GetResult() override {
            return result_;
        }

        size_t GetId() override {
            return id_;
        }

        bool isExecute() override {
            return is_executed_;
        }

        void invoke() override {
            result_ = std::apply(function_, args_);
        }

        std::tuple<Args...> args_;
        Func function_;
        size_t id_;
        Any result_;
        bool is_executed_;
    };

    template <typename T>
    struct FutureResult {
    public:
        FutureResult(std::shared_ptr<BaseTask> ptr) : ptr_(ptr) {}

        operator T() {
            return  ptr_->GetResult().cast<T>();
        }

    private:
        std::shared_ptr<BaseTask> ptr_;
    };

public:
    template <typename Func, typename... Args>
    auto add(Func func, Args... args) {
        auto task = std::make_shared<Task<Func, Args...>>(func, next_id_++, args...);
        return addUtil(task);
    }

    template <typename T>
    FutureResult<T> getFutureResult(size_t id) {
        graph_[id].push_back(next_id_);
        is_top_sorted_ = false;
        return FutureResult<T>(tasks_[id]);
    }

    template <typename T>
    T getResult(size_t id) {
        auto task = tasks_[id];
        if (!task->isExecute()) {
            executeAll();
        }

        return task->GetResult().cast<T>();
    }

    void executeAll() {
        if (!is_top_sorted_) {
            topSort();
        }
        for (int i = top_sort_order_.size() - 1; i >= 0; --i) {
            if (!tasks_[top_sort_order_[i]]->isExecute()) {
                tasks_[top_sort_order_[i]]->invoke();
            }
        }
    }

private:
    void DFS(size_t vertex, std::vector<bool>& visited) {
        visited[vertex] = true;
        for (auto u: graph_[vertex]) {
            if (!visited[u])
                DFS(u, visited);
        }
        top_sort_order_.push_back(vertex);
    }

    void topSort() {
        std::vector<bool> visited(next_id_);
        top_sort_order_.clear();
        for (size_t i = 0; i < next_id_; ++i) {
            if (!visited[i]) {
                DFS(i, visited);
            }
        }
        is_top_sorted_ = true;
    }

    size_t addUtil(std::shared_ptr<BaseTask> task) {
        graph_.push_back(std::vector<size_t>{});
        top_sort_order_.push_back(next_id_ - 1);
        tasks_.push_back(task);
        return (next_id_ - 1);
    }

    std::vector<std::shared_ptr<BaseTask>> tasks_;
    std::vector<std::vector<size_t>> graph_;
    std::vector<size_t> top_sort_order_;
    size_t next_id_ = 0;
    bool is_top_sorted_ = false;
};
