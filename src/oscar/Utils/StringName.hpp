#pragma once

#include <oscar/Shims/Cpp20/string_view.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <atomic>
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
            explicit StringNameData(std::string_view value_) :
                m_Value{value_}
            {
            }
            StringNameData(StringNameData const&) = delete;
            StringNameData(StringNameData&&) noexcept = delete;
            StringNameData& operator=(StringNameData const&) = delete;
            StringNameData& operator=(StringNameData&&) noexcept = delete;
            ~StringNameData() noexcept = default;

            std::string const& value() const
            {
                return m_Value;
            }

            size_t hash() const
            {
                return m_Hash;
            }

            void incrementOwnerCount()
            {
                m_Owners.fetch_add(1, std::memory_order_relaxed);
            }

            bool decrementOwnerCount()
            {
                return m_Owners.fetch_sub(1, std::memory_order_relaxed) == 1;
            }
        private:
            std::string m_Value;
            size_t m_Hash = std::hash<std::string>{}(m_Value);
            std::atomic<size_t> m_Owners = 1;
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
        explicit StringName(char const*);
        explicit StringName(std::nullptr_t) = delete;

        StringName(StringName const& other) noexcept :
            m_Data{other.m_Data}
        {
            m_Data->incrementOwnerCount();
        }

        StringName(StringName&& tmp) noexcept :
            m_Data{tmp.m_Data}
        {
            m_Data->incrementOwnerCount();
        }

        StringName& operator=(StringName const& other) noexcept
        {
            if (other == *this)
            {
                return *this;
            }

            this->~StringName();
            m_Data = other.m_Data;
            m_Data->incrementOwnerCount();
            return *this;
        }

        StringName& operator=(StringName&& tmp) noexcept
        {
            if (tmp == *this)
            {
                return *this;
            }

            this->~StringName();
            m_Data = tmp.m_Data;
            m_Data->incrementOwnerCount();
            return *this;
        }

        ~StringName() noexcept;

        friend void swap(StringName& a, StringName& b) noexcept
        {
            a.swap(b);
        }

        const_reference at(size_t pos) const
        {
            return m_Data->value().at(pos);
        }

        const_reference operator[](size_t pos) const
        {
            return m_Data->value()[pos];
        }

        const_reference front() const
        {
            return m_Data->value().front();
        }

        const_reference back() const
        {
            return m_Data->value().back();
        }

        value_type const* data() const noexcept
        {
            return m_Data->value().data();
        }

        value_type const* c_str() const noexcept
        {
            return m_Data->value().c_str();
        }

        operator std::string_view() const noexcept
        {
            return m_Data->value();
        }

        operator CStringView() const noexcept
        {
            return m_Data->value();
        }

        const_iterator begin() const noexcept
        {
            return m_Data->value().cbegin();
        }

        const_iterator cbegin() const noexcept
        {
            return m_Data->value().cbegin();
        }

        const_iterator end() const noexcept
        {
            return m_Data->value().cend();
        }

        const_iterator cend() const noexcept
        {
            return m_Data->value().cend();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_Data->value().empty();
        }

        size_type size() const noexcept
        {
            return m_Data->value().size();
        }

        void swap(StringName& other) noexcept
        {
            std::swap(m_Data, other.m_Data);
        }

        friend bool operator==(StringName const&, StringName const&) noexcept = default;

        template<ConvertibleTo<std::string_view> StringLike>
        friend bool operator==(StringName const& lhs, StringLike const& rhs)
        {
            return std::string_view{lhs} == std::string_view{rhs};
        }

        template<ConvertibleTo<std::string_view> StringLike>
        friend auto operator<=>(StringName const& lhs, StringLike const& rhs)
        {
            return ThreeWayComparison(std::string_view{lhs}, std::string_view{rhs});
        }

    private:
        friend struct std::hash<StringName>;
        detail::StringNameData* m_Data;
    };

    std::ostream& operator<<(std::ostream&, StringName const&);
}

template<>
struct std::hash<osc::StringName> final {
    size_t operator()(osc::StringName const& str) const
    {
        return str.m_Data->hash();
    }
};
