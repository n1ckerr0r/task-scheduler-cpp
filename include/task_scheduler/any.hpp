#pragma once

#include <exception>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>

namespace task_scheduler {

class Any {
public:
  class BadAnyCast : public std::exception {
  public:
    explicit BadAnyCast(const char *message) : message_(message) {}

    [[nodiscard]] auto what() const noexcept -> const char * override {
      return message_.c_str();
    }

  private:
    std::string message_;
  };

  Any() = default;

  Any(const Any &other)
      : content_(other.content_ ? other.content_->Copy() : nullptr) {}

  auto operator=(const Any &other) -> Any & {
    if (this != &other) {
      content_.reset(other.content_ ? other.content_->Copy() : nullptr);
    }
    return *this;
  }

  Any(Any &&other) noexcept = default;
  auto operator=(Any &&other) noexcept -> Any & = default;

  template <typename T> auto operator=(T value) -> Any & {
    content_ = std::make_unique<ValueHolder<T>>(std::move(value));
    return *this;
  }

  [[nodiscard]] auto HasValue() const -> bool { return content_ != nullptr; }

  template <typename T> auto cast() -> T & {
    EnsureValue();
    if (typeid(T) != content_->GetType()) {
      throw BadAnyCast("Stored value has a different type");
    }
    return static_cast<ValueHolder<T> *>(content_.get())->value_;
  }

  template <typename T> [[nodiscard]] auto cast() const -> const T & {
    EnsureValue();
    if (typeid(T) != content_->GetType()) {
      throw BadAnyCast("Stored value has a different type");
    }
    return static_cast<const ValueHolder<T> *>(content_.get())->value_;
  }

  void swap(Any &other) noexcept { std::swap(content_, other.content_); }

private:
  class BaseHolder {
  public:
    virtual ~BaseHolder() = default;

    [[nodiscard]] virtual auto Copy() const -> BaseHolder * = 0;
    [[nodiscard]] virtual auto GetType() const -> const std::type_info & = 0;
  };

  template <typename T> class ValueHolder final : public BaseHolder {
  public:
    explicit ValueHolder(T value) : value_(std::move(value)) {}

    [[nodiscard]] auto Copy() const -> BaseHolder * override {
      return new ValueHolder(value_);
    }

    [[nodiscard]] auto GetType() const -> const std::type_info & override {
      return typeid(T);
    }

    T value_;
  };

  void EnsureValue() const {
    if (!HasValue()) {
      throw BadAnyCast("Any does not contain a value");
    }
  }

  std::unique_ptr<BaseHolder> content_;
};

} // namespace task_scheduler
