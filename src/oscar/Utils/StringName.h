#pragma once

#include <oscar/Utils/CStringView.h>

#include <atomic>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace osc
{
    namespace detail
    {
        class StringNameData final {
        public:
            explicit StringNameData(std::string_view view) :
                value_{view}
            {}
            StringNameData(StringNameData const&) = delete;
            StringNameData(StringNameData&&) noexcept = delete;
            StringNameData& operator=(const StringNameData&) = delete;
            StringNameData& operator=(StringNameData&&) noexcept = delete;
            ~StringNameData() noexcept = default;

            const std::string& value() const { return value_; }

            size_t hash() const { return hash_; }

            void increment_owner_count()
            {
                num_owners_.fetch_add(1, std::memory_order_relaxed);
            }

            bool decrement_owner_count()
            {
                return num_owners_.fetch_sub(1, std::memory_order_relaxed) == 1;
            }
        private:
            std::string value_;
            size_t hash_ = std::hash<std::string>{}(value_);
            std::atomic<size_t> num_owners_ = 1;
        };
    }

    // immutable, globally unique string with fast hashing+equality
    class StringName final {
    public:
        using traits_type = std::string::traits_type;
        using value_type = std::string::value_type;
        using size_type = std::string::size_type;
        using difference_type = std::string::difference_type;
        using reference = std::string::const_reference;
        using const_reference = std::string::const_reference;
        using iterator = std::string::const_iterator;
        using const_iterator = std::string::const_iterator;
        using reverse_iterator = std::string::const_reverse_iterator;
        using const_reverse_iterator = std::string::const_reverse_iterator;

        explicit StringName();
        explicit StringName(std::string&&);
        explicit StringName(std::string_view);
        explicit StringName(const char* s) : StringName{std::string_view{s}} {}
        explicit StringName(std::nullptr_t) = delete;

        StringName(const StringName& other) noexcept :
            data_{other.data_}
        {
            data_->increment_owner_count();
        }

        StringName(StringName&& tmp) noexcept :
            data_{tmp.data_}
        {
            data_->increment_owner_count();
        }

        StringName& operator=(const StringName& other) noexcept
        {
            if (other == *this) {
                return *this;
            }

            this->~StringName();
            data_ = other.data_;
            data_->increment_owner_count();
            return *this;
        }

        StringName& operator=(StringName&& strname) noexcept
        {
            if (strname == *this) {
                return *this;
            }

            this->~StringName();
            data_ = strname.data_;
            data_->increment_owner_count();
            return *this;
        }

        ~StringName() noexcept;

        friend void swap(StringName& a, StringName& b) noexcept
        {
            a.swap(b);
        }

        const_reference at(size_t pos) const
        {
            return data_->value().at(pos);
        }

        const_reference operator[](size_t pos) const
        {
            return data_->value()[pos];
        }

        const_reference front() const
        {
            return data_->value().front();
        }

        const_reference back() const
        {
            return data_->value().back();
        }

        const value_type* data() const noexcept
        {
            return data_->value().data();
        }

        const value_type* c_str() const noexcept
        {
            return data_->value().c_str();
        }

        operator std::string_view() const noexcept
        {
            return data_->value();
        }

        operator CStringView() const noexcept
        {
            return data_->value();
        }

        const_iterator begin() const noexcept
        {
            return data_->value().cbegin();
        }

        const_iterator cbegin() const noexcept
        {
            return data_->value().cbegin();
        }

        const_iterator end() const noexcept
        {
            return data_->value().cend();
        }

        const_iterator cend() const noexcept
        {
            return data_->value().cend();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return data_->value().empty();
        }

        size_type size() const noexcept
        {
            return data_->value().size();
        }

        void swap(StringName& other) noexcept
        {
            std::swap(data_, other.data_);
        }

        friend bool operator==(const StringName&, const StringName&) noexcept = default;

        template<std::convertible_to<std::string_view> StringLike>
        friend bool operator==(const StringName& lhs, const StringLike& rhs)
        {
            return std::string_view{lhs} == std::string_view{rhs};
        }

        template<std::convertible_to<std::string_view> StringLike>
        friend auto operator<=>(const StringName& lhs, const StringLike& rhs)
        {
            return std::string_view{lhs} <=> std::string_view{rhs};
        }

    private:
        friend struct std::hash<StringName>;
        detail::StringNameData* data_;
    };

    std::ostream& operator<<(std::ostream&, const StringName&);
}

template<>
struct std::hash<osc::StringName> final {
    size_t operator()(const osc::StringName& str) const
    {
        return str.data_->hash();
    }
};
