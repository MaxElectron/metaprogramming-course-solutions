#pragma once
#include <compare>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

struct ILogger {
  virtual void log(unsigned int call_count) = 0;
  virtual ~ILogger() = default;
  virtual ILogger *clone() const = 0;
};

template <typename Logger> struct LoggerAdapter : ILogger {
  Logger logger_;

  template <typename L>
  explicit LoggerAdapter(L &&logger) : logger_(std::forward<L>(logger)) {}

  void log(unsigned int call_count) override { logger_(call_count); }

  ILogger *clone() const override {
    if constexpr (std::is_copy_constructible_v<Logger>) {
      return new LoggerAdapter(logger_);
    } else {
      return nullptr;
    }
  }
};

struct DynLogger : ILogger {
  std::unique_ptr<ILogger> logger_ptr_;

  template <typename L>
  DynLogger(L &&logger_holder)
      : logger_ptr_(std::make_unique<LoggerAdapter<L>>(
            std::forward<L>(logger_holder))) {}

  DynLogger(const DynLogger &other)
      : logger_ptr_(other.logger_ptr_ ? other.logger_ptr_->clone() : nullptr) {}

  DynLogger &operator=(const DynLogger &other) {
    if (this == &other) {
      return *this;
    }

    DynLogger temp(other);
    std::swap(temp.logger_ptr_, logger_ptr_);
    return *this;
  }

  DynLogger(DynLogger &&) = default;
  DynLogger &operator=(DynLogger &&) = default;

  void log(unsigned int call_count) override {
    if (logger_ptr_) {
      logger_ptr_->log(call_count);
    }
  }

  ILogger *clone() const override {
    return logger_ptr_ ? new DynLogger(*this) : nullptr;
  }
};

template <class T, class Allocator = std::allocator<std::byte>> class Spy {
  struct Tracker {
    T *operator->() { return &spy_ref_->obj_; }
    ~Tracker() { spy_ref_->on_access_end(); }
    Spy *spy_ref_;
    Tracker(Spy &spy) : spy_ref_(&spy) {}
  };

public:
  explicit Spy(const Allocator &alloc) : alloc_(alloc) {}

  Spy() = default;

  explicit Spy(T obj, const Allocator &alloc = Allocator())
      : alloc_(alloc), obj_(std::move(obj)) {}

  Spy(const Spy &other)
    requires(std::copy_constructible<T>)
      : alloc_(other.alloc_), obj_(other.obj_) {
    if (other.logger_) {
      logger_.reset(other.logger_->clone());
    }
  }

  Spy &operator=(const Spy &other)
    requires(std::copyable<T>)
  {
    if (this == &other) {
      return *this;
    }

    Spy temp(other);
    std::swap(temp.alloc_, alloc_);
    std::swap(logger_, temp.logger_);
    std::swap(obj_, temp.obj_);
    return *this;
  }

  Spy(Spy &&other)
    requires(std::move_constructible<T>)
      : alloc_(std::move(other.alloc_)), obj_(std::move(other.obj_)),
        logger_(std::move(other.logger_)) {}

  Spy &operator=(Spy &&other)
    requires(std::movable<T>)
  {
    if (this == &other) {
      return *this;
    }
    Spy temp(std::move(other));
    std::swap(temp.alloc_, alloc_);
    std::swap(logger_, temp.logger_);
    std::swap(obj_, temp.obj_);
    return *this;
  }

  auto operator<=>(const Spy &other) const
    requires(std::three_way_comparable<T>)
  {
    return obj_ <=> other.obj_;
  }

  bool operator==(const Spy &other) const
    requires(std::equality_comparable<T>)
  {
    return obj_ == other.obj_;
  }

  T &operator*() { return obj_; }

  const T &operator*() const { return obj_; }

  Tracker operator->() {
    ++access_cnt_;
    ++tracker_cnt_;
    return Tracker(*this);
  }

  void setLogger() { logger_.reset(); }

  template <std::invocable<unsigned int> Logger>
    requires((!std::copy_constructible<T> ||
              std::is_copy_constructible_v<std::remove_reference_t<Logger>>) &&
             (!std::move_constructible<T> ||
              std::is_move_constructible_v<std::remove_reference_t<Logger>>) &&
             std::is_nothrow_destructible_v<std::remove_reference_t<Logger>>)
  void setLogger(Logger &&logger) {
    logger_ = std::make_unique<DynLogger>(std::forward<Logger>(logger));
  }

private:
  friend struct Tracker;

  unsigned int access_cnt_ = 0;
  unsigned int tracker_cnt_ = 0;
  Allocator alloc_;
  std::unique_ptr<ILogger> logger_;
  T obj_;

  void on_access_end() {
    if (tracker_cnt_ == 0) {
      return;
    }
    --tracker_cnt_;
    if (tracker_cnt_ > 0) {
      return;
    }
    log_if_avail();
    access_cnt_ = 0;
  }

  void log_if_avail() {
    if (logger_) {
      logger_->log(access_cnt_);
    }
  }
};