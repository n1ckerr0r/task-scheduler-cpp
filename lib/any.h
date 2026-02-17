#pragma once

#include <iostream>
#include <memory>
#include <typeinfo>
#include <type_traits>

class Any {
public:
    Any() : content_(nullptr) {}

    Any(const Any& other)
            : content_(other.content_ ? other.content_->copy() : nullptr)
            {}

    Any& operator=(const Any& other) {
        Any(other).swap(*this);
        return *this;
    }

    Any(Any&& other) noexcept
            : content_(std::move(other.content_)) {}

    Any& operator=(Any&& other) noexcept {
        Any(std::move(other)).swap(*this);
        return *this;
    }

    template<typename T>
    Any& operator=(const T& value) {
        content_ = std::make_unique<TaskHolder<T>>(value);
        return *this;
    }

    template<typename T>
    T& cast() {
        if (typeid(T) != content_->getType()) {
            throw BadAnyCast("Task has been created with another type");
        }
        return static_cast<TaskHolder<T>*>(content_.get())->value_;
    }

    template<typename T>
    const T& cast() const {
        if (typeid(T) != content_->getType()) {
            throw BadAnyCast("Task has been created with another type");
        }
        return static_cast<TaskHolder<T>*>(content_.get())->value_;
    }

    void swap(Any& other) {
        std::swap(content_, other.content_);
    }

private:
    class BaseTaskHolder {
    public:
        virtual ~BaseTaskHolder() = default;

        virtual const std::type_info& getType() const = 0;

        virtual BaseTaskHolder* copy() const = 0;
    };

    template<typename T>
    class TaskHolder : public BaseTaskHolder {
    public:
        TaskHolder(T value) : value_(value) {}

        const std::type_info& getType() const override {
            return typeid(T);
        }

        BaseTaskHolder* copy() const override {
            return new TaskHolder(value_);
        }

        T value_;
    };

    class BadAnyCast : public std::exception {
    public:
        BadAnyCast(const char* msg) : message(msg) {}

        const char* what() const throw() {
            return message.c_str();
        }

    private:
        std::string message;
    };

    std::unique_ptr<BaseTaskHolder> content_;
};

