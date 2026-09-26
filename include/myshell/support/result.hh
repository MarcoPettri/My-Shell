#pragma once

#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace myshell {

/// @brief Lightweight result type modelling either success (T) or failure (E).
///
/// Invariants:
/// - A default-constructed Result<void, E> is success.
/// - [[nodiscard]] on all factory functions and the type itself.
///
/// Thread-safety: not thread-safe (value semantics — copy/move as needed).
template<typename T, typename E>
class [[nodiscard]] Result {
public:
    // ------------------------------------------------------------------ //
    //  Construction
    // ------------------------------------------------------------------ //

    /// @brief Construct a successful result holding `value`.
    static Result ok(T value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        Result r;
        r.storage_.template emplace<0>(std::move(value));
        return r;
    }

    /// @brief Construct an error result holding `error`.
    static Result err(E error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        Result r;
        r.storage_.template emplace<1>(std::move(error));
        return r;
    }

    // ------------------------------------------------------------------ //
    //  Observers
    // ------------------------------------------------------------------ //

    /// @brief Returns true if this result holds a success value.
    [[nodiscard]] bool is_ok() const noexcept {
        return storage_.index() == 0;
    }
    [[nodiscard]] bool ok() const noexcept {
        return is_ok();
    }

    /// @brief Returns true if this result holds an error.
    [[nodiscard]] bool is_err() const noexcept {
        return storage_.index() == 1;
    }

    /// @brief Boolean conversion — true when ok.
    explicit operator bool() const noexcept {
        return is_ok();
    }

    // ------------------------------------------------------------------ //
    //  Value access (UB / assert if wrong state)
    // ------------------------------------------------------------------ //

    [[nodiscard]] T& value() & {
        assert(is_ok() && "Result::value() called on error result");
        return std::get<0>(storage_);
    }

    [[nodiscard]] const T& value() const& {
        assert(is_ok() && "Result::value() called on error result");
        return std::get<0>(storage_);
    }

    [[nodiscard]] T&& value() && {
        assert(is_ok() && "Result::value() called on error result");
        return std::get<0>(std::move(storage_));
    }

    [[nodiscard]] E& error() & {
        assert(is_err() && "Result::error() called on ok result");
        return std::get<1>(storage_);
    }

    [[nodiscard]] const E& error() const& {
        assert(is_err() && "Result::error() called on ok result");
        return std::get<1>(storage_);
    }

    [[nodiscard]] E&& error() && {
        assert(is_err() && "Result::error() called on ok result");
        return std::get<1>(std::move(storage_));
    }

    /// @brief Return value or default if error.
    [[nodiscard]] T value_or(T&& def) const& {
        return is_ok() ? std::get<0>(storage_) : std::forward<T>(def);
    }

private:
    Result() = default;
    std::variant<T, E> storage_;
};

// ------------------------------------------------------------------ //
//  Specialisation for void success
// ------------------------------------------------------------------ //

template<typename E>
class [[nodiscard]] Result<void, E> {
public:
    static Result ok() noexcept {
        Result r;
        r.has_error_ = false;
        return r;
    }

    static Result err(E error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        Result r;
        r.has_error_ = true;
        r.error_ = std::move(error);
        return r;
    }

    [[nodiscard]] bool is_ok() const noexcept {
        return !has_error_;
    }
    [[nodiscard]] bool is_err() const noexcept {
        return has_error_;
    }
    explicit operator bool() const noexcept {
        return is_ok();
    }

    [[nodiscard]] E& error() & {
        assert(is_err());
        return error_;
    }
    [[nodiscard]] const E& error() const& {
        assert(is_err());
        return error_;
    }
    [[nodiscard]] E&& error() && {
        assert(is_err());
        return std::move(error_);
    }

private:
    Result() = default;
    bool has_error_{false};
    E error_{};
};

namespace support {
template<typename T, typename E>
using Result = myshell::Result<T, E>;
}  // namespace support

}  // namespace myshell
