#include <iterator>
#include <vector>
#include <ranges>
#include <concepts>
#include <cstdlib>
#include <algorithm>

// "Как же я ненавижу компьютеры, Ребят... Это все было ошибкой" --Санду

// Span Class Template
template <class T, std::size_t extent = std::dynamic_extent> class Span {
public:
  // Type Aliases
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;

  // Size Access
  constexpr std::size_t Size() const {
    if constexpr (extent == std::dynamic_extent) {
      return dynamic_size_;
    } else {
      return extent;
    }
  }

  // Data Access
  constexpr T *Data() const { return data_ptr_; }

  // Subranges
  template <std::size_t Count>
  constexpr Span<element_type, Count> Last() const {
    MPC_VERIFY(Count <= Size());
    return {end() - Count};
  }

  Span<element_type, std::dynamic_extent> Last(size_type count) const {
    MPC_VERIFY(count <= Size());
    return {end() - count, count};
  }

  template <std::size_t Count>
  constexpr Span<element_type, Count> First() const {
    MPC_VERIFY(Count <= Size());
    return {data_ptr_};
  }

  // Destructor
  ~Span() = default;

  Span<element_type, std::dynamic_extent> First(size_type count) const {
    MPC_VERIFY(count <= Size());
    return {data_ptr_, count};
  }

  // Constructors
  template <class R>
    requires std::ranges::sized_range<R>
  constexpr Span(R &&range) : data_ptr_(std::ranges::data(range)) {
    if constexpr (extent == std::dynamic_extent) {
      dynamic_size_ = std::ranges::size(range);
    } else {
      MPC_VERIFY(std::ranges::size(range) == extent);
    }
  }

  constexpr Span(std::array<T, extent> &arr) noexcept
      : data_ptr_(arr.data()), dynamic_size_{} {}

  template <class It>
    requires std::random_access_iterator<It>
  Span(It first, std::size_t count)
      : data_ptr_(std::to_address(first)), dynamic_size_{count} {}

  Span(const T *data, std::size_t count)
      : data_ptr_(const_cast<T *>(data)), dynamic_size_{count} {}

  Span(T *data) : data_ptr_(data), dynamic_size_{} {}

  Span &operator=(const Span &other) noexcept = default;
  constexpr Span(const Span &other) noexcept = default;

  constexpr Span() noexcept
    requires(extent == 0 || extent == std::dynamic_extent)
      : data_ptr_(nullptr), dynamic_size_{0} {}

  // Element Access
  reference operator[](std::size_t i) const {
    MPC_VERIFY(i < Size());
    return *(data_ptr_ + i);
  }

  reverse_iterator rend() const  { return reverse_iterator(begin()); }
  reverse_iterator rbegin() const { return reverse_iterator(end()); }

  // Accessors
  reference Back() const {
    MPC_VERIFY(Size() > 0);
    return *(end() - 1);
  }
  reference Front() const {
    MPC_VERIFY(Size() > 0);
    return *begin();
  }

  // Iterators
  T *end() const { return data_ptr_ + Size(); }
  T *begin() const { return data_ptr_; }

  // Members
 private:
  std::conditional_t<extent == std::dynamic_extent, size_type, char[0]>
      dynamic_size_;
  T *data_ptr_;
};

// Class Template Deduction Guides
template <class It>
Span(It, std::size_t)
    -> Span<std::remove_reference_t<std::iter_reference_t<It>>>;

template <typename R>
  requires std::ranges::sized_range<R>
Span(R &&) -> Span<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

template <typename T> Span(const std::vector<T> &vec) -> Span<T>;