#pragma once

#include <liboscar/utilities/hash_helpers.h>

#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace osc
{
    // Implementation details: downstream code shouldn't use this.
    namespace detail
    {
        // Satisfied if `R` has a size/extent greater than or equal to `Max`.
        template<typename R, size_t N>
        concept CompileTimeSizeGreaterThanOrEqualTo =
            (
                requires { std::remove_cvref_t<R>::extent; } and
                std::remove_cvref_t<R>::extent >= N and
                std::remove_cvref_t<R>::extent != std::dynamic_extent
            )
            or
            (
                requires { typename std::tuple_size <std::remove_cvref_t<R>>; } and
                std::tuple_size<std::remove_cvref_t<R>>::value >= N
            );

        // Satisfied if `R` has a size/extent equal to `N`.
        template<typename R, size_t N>
        concept CompileTimeSizeEqualTo =
            (
                requires { std::remove_cvref_t<R>::extent; } and
                std::remove_cvref_t<R>::extent == N and
                std::remove_cvref_t<R>::extent != std::dynamic_extent
            )
            or
            (
                requires { typename std::tuple_size<std::remove_cvref_t<R>>; } and
                std::tuple_size<std::remove_cvref_t<R>>::value == N
            );

        template<typename T, size_t N>
        requires (N != std::dynamic_extent)
        constexpr size_t compile_time_size(std::span<T, N>) { return N; }

        // Satisfied if `T` can be written to a `std::ostream`
        template<typename T>
        concept OutputStreamable = requires (std::ostream& out, const T& v) { out << v; };
    }

    // Represents a contiguous range of `N` instances of `T`.
    template<typename T, size_t N>
    requires (N > 0)
    class Vector final {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        // Constructs a `Vector` with value-initialized `T`s.
        constexpr Vector() = default;

        constexpr Vector(const Vector&) = default;
        constexpr Vector(Vector&&) = default;

        // Constructs a `Vector` with `value` assigned to all elements.
        explicit constexpr Vector(const T& value) :
            Vector(value, std::make_index_sequence<N>{})
        {}

        // Constructs a `Vector` element-by-element.
        template<typename... Elements>
        requires (
            (sizeof...(Elements) == N) and
            (std::constructible_from<T, Elements> and ...)
        )
        explicit (not (std::convertible_to<Elements, T> and ...))
        constexpr Vector(Elements&&... elements) :
            data_{static_cast<T>(std::forward<Elements>(elements))...}
        {}

        // Constructs a `Vector` from the first `N` elements of the given fixed-size contiguous range.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeGreaterThanOrEqualTo<R, N> and
            std::constructible_from<T, typename std::remove_cvref_t<R>::value_type>
        )
        explicit (not std::convertible_to<typename std::remove_cvref_t<R>::value_type, T>)
        constexpr Vector(R&& r) :
            Vector{std::forward<R>(r), std::make_index_sequence<N>{}}
        {}

        // Constructs a `Vector` by extending a smaller one.
        template<typename U, size_t M, typename... Elements>
        requires (
            M < N and
            ((M + sizeof...(Elements)) == N) and
            std::constructible_from<T, U> and
            (std::constructible_from<T, Elements> and ...)
        )
        constexpr Vector(const Vector<U, M>& v, Elements&&... elements) :
            Vector{v, std::make_index_sequence<M>{}, std::forward<Elements>(elements)...}
        {}

        ~Vector() noexcept = default;

        constexpr Vector& operator=(const Vector&) = default;
        constexpr Vector& operator=(Vector&&) = default;

        // Assigns the first `N` elements of a given fixed-size contiguous range to `*this`.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeGreaterThanOrEqualTo<R, N> and
            std::assignable_from<T&, typename std::remove_cvref_t<R>::value_type>
        )
        constexpr Vector& operator=(R&& r)
        {
            copy_or_move_assign(std::forward<R>(r), std::make_index_sequence<N>{});
            return *this;
        }

        // Returns the number of elements in `*this`.
        constexpr size_type size() const { return N; }

        // Returns a pointer to the underlying array serving as element storage.
        constexpr pointer       data()       { return data_.data(); }
        constexpr const_pointer data() const { return data_.data(); }

        // Returns an iterator to the first element of `*this`.
        constexpr iterator       begin()       { return data(); }
        constexpr const_iterator begin() const { return data(); }

        // Returns an iterator past the last element of `*this`.
        constexpr iterator       end()       { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }

        // Returns a reference to the element at specified position `pos`.
        constexpr reference       operator[](size_type pos)       { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        // Returns `(*this)[0]`
        constexpr reference       x()       requires (N > 0) { return begin()[0]; }
        constexpr const_reference x() const requires (N > 0) { return begin()[0]; }

        // Returns `(*this)[1]`
        constexpr reference       y()       requires (N > 1) { return begin()[1]; }
        constexpr const_reference y() const requires (N > 1) { return begin()[1]; }

        // Returns `(*this)[2]`
        constexpr reference       z()       requires (N > 2) { return begin()[2]; }
        constexpr const_reference z() const requires (N > 2) { return begin()[2]; }

        // Returns `(*this)[3]`
        constexpr reference       w()       requires (N > 3) { return begin()[3]; }
        constexpr const_reference w() const requires (N > 3) { return begin()[3]; }

        friend constexpr bool operator== (const Vector&, const Vector&) = default;
        friend constexpr auto operator<=>(const Vector&, const Vector&) = default;

        constexpr operator std::span<      T, N>()       { return {data(), size()}; }
        constexpr operator std::span<const T, N>() const { return {data(), size()}; }

        // Adds `rhs` to each element of `*this`.
        template<typename U>
        requires (requires (T& lhs, const U& rhs) { lhs += rhs; })
        constexpr Vector& operator+=(const U& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] += rhs), ...);  // fold over +=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Adds each element of `rhs` to each element of `*this` pairwise.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeEqualTo<R, N> and
            requires (T& lhs, const R& rhs) { lhs += rhs[0]; }
        )
        constexpr Vector& operator+=(const R& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] += rhs[Is]), ...);  // fold over +=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Subtracts `rhs` from each element of `*this`.
        template<typename U>
        requires (requires (T& lhs, const U& rhs) { lhs -= rhs; })
        constexpr Vector& operator-=(const U& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] -= rhs), ...);  // fold over -=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Subtracts each element of `rhs` from each element of `*this` pairwise.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeEqualTo<R, N> and
            requires (T& lhs, const R& rhs) { lhs -= rhs[0]; }
        )
        constexpr Vector& operator-=(const R& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] -= rhs[Is]), ...);  // fold over -=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Multiplies each element of `*this` by `rhs`.
        template<typename U>
        requires (requires (T& lhs, const U& rhs) { lhs *= rhs; })
        constexpr Vector& operator*=(const U& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] *= rhs), ...);  // fold over *=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Multiplies each element of `rhs` with each element of `*this` pairwise.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeEqualTo<R, N> and
            requires (T& lhs, const R& rhs) { lhs *= rhs[0]; }
        )
        constexpr Vector& operator*=(const R& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] *= rhs[Is]), ...);  // fold over *=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Divides each element of `*this` by `rhs`.
        template<typename U>
        requires (requires (T& lhs, const U& rhs) { lhs /= rhs; })
        constexpr Vector& operator/=(const U& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] /= rhs), ...);  // fold over /=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Divides each element of `rhs` by each element of `*this` pairwise.
        template<std::ranges::contiguous_range R>
        requires (
            detail::CompileTimeSizeEqualTo<R, N> and
            requires (T& lhs, const R& rhs) { lhs /= rhs[0]; }
        )
        constexpr Vector& operator/=(const R& rhs)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((data_[Is] /= rhs[Is]), ...);  // fold over /=
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Increments each element of `*this` and then returns `*this`.
        constexpr Vector& operator++()
        requires (requires(T& t) { ++t; })
        {
            [this]<size_t... Is>(std::index_sequence<Is...>)
            {
                (++data_[Is], ...);  // fold over ++
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Decrements each element of `*this` and then returns `*this`.
        constexpr Vector& operator--()
        requires (requires(T& t) { --t; })
        {
            [this]<size_t... Is>(std::index_sequence<Is...>)
            {
                (--data_[Is], ...);  // fold over --
            }(std::make_index_sequence<N>{});
            return *this;
        }

        // Creates a copy of `*this` increments `*this`, and then returns the copy (postfix).
        constexpr Vector operator++(int)
        requires (requires(T& t) { ++t; })
        {
            Vector copy{*this};
            ++(*this);
            return copy;
        }

        // Creates a copy of `*this` decrements `*this`, and then returns the copy (postfix).
        constexpr Vector operator--(int)
        requires (requires(T& t) { --t; })
        {
            Vector copy{*this};
            --(*this);
            return copy;
        }

        // Returns a copy of `*this`, but with `value` used to construct the element at `pos`.
        template<typename U>
        requires std::constructible_from<T, U>
        constexpr Vector with_element(size_type pos, U value) const
        {
            Vector copy{*this};
            copy[pos] = static_cast<T>(value);
            return copy;
        }

    private:
        // Helper constructor that assigns `value` to all elements in `data_`
        template<size_t... Is>
        constexpr Vector(const T& value, std::index_sequence<Is...>) :
            data_{ (static_cast<void>(Is), value)... }
        {}

        // Helper function that forwards an element in a range.
        template<class R, class U>
        static constexpr decltype(auto) forward_range_element(U&& x)
        {
            if constexpr (std::is_rvalue_reference_v<R&&>) {
                return std::move(x);
            }
            else {
                return std::forward<U>(x);
            }
        }

        // Helper constructor that perfectly forwards range elements into `data_`
        template<std::ranges::contiguous_range R, size_t... Is>
        constexpr Vector(R&& r, std::index_sequence<Is...>) :
            data_{ static_cast<T>(forward_range_element<R>(r[Is]))... }
        {}

        // Helper constructor that copies from a smaller `Vector` and extends it with `elements`
        template<typename U, size_t M, size_t... Is, typename... Elements>
        constexpr Vector(const Vector<U, M>& v, std::index_sequence<Is...>, Elements&&... elements) :
            data_{static_cast<T>(v[Is])..., static_cast<T>(std::forward<Elements>(elements))...}
        {}

        template<std::ranges::contiguous_range R, size_t... Is>
        constexpr void copy_or_move_assign(R&& r, std::index_sequence<Is...>)
        {
            ((data_[Is] = static_cast<T>(forward_range_element<R>(r[Is]))), ...);
        }

        std::array<T, N> data_{};
    };

    // Returns a copy of `vec` with each element copied and promoted via unary `+`.
    template<typename T, size_t N>
    requires (requires (const Vector<T, N>& v) { +v[0]; })
    constexpr auto operator+(const Vector<T, N>& vec)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(+vec[0])>, N>{+vec[Is]...};  // fold over unary `+`
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `vec` with each element copied and promoted via unary `-`.
    template<typename T, size_t N>
    requires (requires (const Vector<T, N>& v) { -v[0]; })
    constexpr auto operator-(const Vector<T, N>& vec)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(-vec[0])>, N>{-vec[Is]...};  // fold over unary `-`
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `vec` with each element copied and logically NOTed (!)
    template<typename T, size_t N>
    requires (requires (const Vector<T, N>& v) { !v[0]; })
    constexpr auto operator!(const Vector<T, N>& vec)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(!vec[0])>, N>{!vec[Is]...};  // fold over unary `!`
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `lhs` with `rhs` added to each element.
    template<typename T, size_t N, typename U>
    requires (
        requires (const Vector<T, N>& v, const U& rhs) { v[0] + rhs; }
    )
    constexpr auto operator+(const Vector<T, N>& lhs, const U& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] + rhs)>, N>{(lhs[Is] + rhs)...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `rhs` with `lhs` added to each element.
    template<typename T, typename U, size_t N>
    requires (
        requires (const T& lhs, const Vector<U, N>& rhs) { lhs + rhs[0]; }
    )
    constexpr auto operator+(const T& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs + rhs[0])>, N>{(lhs + rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a `Vector` containing `lhs[i] + rhs[i]` for each `i` in `[0, N)`.
    template<typename T, typename U, size_t N>
    requires (requires (const Vector<T, N>& lhs, const Vector<U, N>& rhs) { lhs[0] + rhs[0]; })
    constexpr auto operator+(const Vector<T, N>& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] + rhs[0])>, N>{(lhs[Is] + rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `lhs` with `rhs` subtracted from each element.
    template<typename T, size_t N, typename U>
    requires (requires (const Vector<T, N>& v, const U& rhs) { v[0] - rhs; })
    constexpr auto operator-(const Vector<T, N>& lhs, const U& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] - rhs)>, N>{(lhs[Is] - rhs)...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `rhs` with `lhs` subtracted from each element.
    template<typename T, typename U, size_t N>
    requires (requires (const T& lhs, const Vector<U, N>& rhs) { lhs - rhs[0]; })
    constexpr auto operator-(const T& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs - rhs[0])>, N>{(lhs - rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a `Vector` containing `lhs[i] - rhs[i]` for each `i` in `[0, N)`.
    template<typename T, typename U, size_t N>
    requires (requires (const Vector<T, N>& lhs, const Vector<U, N>& rhs) { lhs[0] - rhs[0]; })
    constexpr auto operator-(const Vector<T, N>& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] - rhs[0])>, N>{(lhs[Is] - rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `lhs` with each element multiplied by `rhs`.
    template<typename T, size_t N, typename U>
    requires (requires (const Vector<T, N>& v, const U& rhs) { v[0] * rhs; })
    constexpr auto operator*(const Vector<T, N>& lhs, const U& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] * rhs)>, N>{(lhs[Is] * rhs)...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `rhs` with each element multiplied by `lhs`.
    template<typename T, typename U, size_t N>
    requires (requires (const T& lhs, const Vector<U, N>& rhs) { U(lhs * rhs[0]); })
    constexpr auto operator*(const T& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs * rhs[0])>, N>{(lhs * rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a `Vector` containing `lhs[i] * rhs[i]` for each `i` in `[0, N)`.
    template<typename T, typename U, size_t N>
    requires (requires (const Vector<T, N>& lhs, const Vector<U, N>& rhs) { lhs[0] * rhs[0]; })
    constexpr auto operator*(const Vector<T, N>& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] * rhs[0])>, N>{(lhs[Is] * rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `lhs` with each element divided by `rhs`.
    template<typename T, size_t N, typename U>
    requires (requires (const Vector<T, N>& v, const U& rhs) { v[0] / rhs; })
    constexpr auto operator/(const Vector<T, N>& lhs, const U& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] / rhs)>, N>{(lhs[Is] / rhs)...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a copy of `rhs` with each element divided by `lhs`.
    template<typename T, typename U, size_t N>
    requires (requires (const T& lhs, const Vector<U, N>& rhs) { lhs / rhs[0]; })
    constexpr auto operator/(const T& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs / rhs[0])>, N>{(lhs / rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Returns a `Vector` containing `lhs[i] / rhs[i]` for each `i` in `[0, N)`.
    template<typename T, typename U, size_t N>
    requires (requires (const Vector<T, N>& lhs, const Vector<U, N>& rhs) { lhs[0] / rhs[0]; })
    constexpr auto operator/(const Vector<T, N>& lhs, const Vector<U, N>& rhs)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return Vector<std::remove_cvref_t<decltype(lhs[0] / rhs[0])>, N>{(lhs[Is] / rhs[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // Formats the vector to the given output stream as `VectorN(el, el, el)`
    template<detail::OutputStreamable T, size_t N>
    std::ostream& operator<<(std::ostream& out, const Vector<T, N>& v)
    {
        out << "Vector" << N << '(';
        std::string_view delimiter;
        for (const T& el : v) {
            out << delimiter << el;
            delimiter = ", ";
        }
        out << ')';
        return out;
    }

    // Returns the `I`th element of `v` (structured binding support)
    template<size_t I, typename T, size_t N>
    constexpr const T& get(const Vector<T, N>& v) { return v[I]; }

    // Returns the `I`th element of `v` (structured binding support)
    template<size_t I, typename T, size_t N>
    constexpr T& get(Vector<T, N>& v) { return v[I]; }

    // Returns the `I`th element of `v` (structured binding support)
    template<size_t I, typename T, size_t N>
    constexpr T&& get(Vector<T, N>&& v) { return std::move(v[I]); }

    // Returns the `I`th element of `v` (structured binding support)
    template<size_t I, typename T, size_t N>
    constexpr const T&& get(const Vector<T, N>&& v) { return std::move(v[I]); }
}

// Returns the number of elements in a `Vector` at compile-time (`std::tuple_size_v` and structured binding support)
template<typename T, size_t N>
struct std::tuple_size<osc::Vector<T, N>> : std::integral_constant<size_t, N> {};

// Returns the type of the element of a vector (`std::get` and structured binding support)
template<size_t I, typename T, size_t N>
requires (I < N)
struct std::tuple_element<I, osc::Vector<T, N>> { using type = T; };

template<typename T, size_t N>
struct std::hash<osc::Vector<T, N>> final {
    size_t operator()(const osc::Vector<T, N>& vec) const noexcept(noexcept(osc::hash_range(vec)))
    {
        return osc::hash_range(vec);
    }
};

// `Vector` aliases
namespace osc
{
    using Vector2    = Vector<float,     2>;
    using Vector2f   = Vector<float,     2>;
    using Vector2d   = Vector<double,    2>;
    using Vector2i   = Vector<int,       2>;
    using Vector2z   = Vector<ptrdiff_t, 2>;
    using Vector2uz  = Vector<size_t,    2>;
    using Vector2u32 = Vector<uint32_t,  2>;

    using Vector3    = Vector<float,     3>;
    using Vector3f   = Vector<float,     3>;
    using Vector3d   = Vector<double,    3>;
    using Vector3i   = Vector<int,       3>;
    using Vector3z   = Vector<ptrdiff_t, 3>;
    using Vector3uz  = Vector<size_t,    3>;
    using Vector3u32 = Vector<uint32_t,  3>;

    using Vector4    = Vector<float,     4>;
    using Vector4f   = Vector<float,     4>;
    using Vector4d   = Vector<double,    4>;
    using Vector4i   = Vector<int,       4>;
    using Vector4z   = Vector<ptrdiff_t, 4>;
    using Vector4uz  = Vector<size_t,    4>;
    using Vector4u32 = Vector<uint32_t,  4>;
}
