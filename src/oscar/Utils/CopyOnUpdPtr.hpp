#pragma once

#include <memory>
#include <utility>

namespace osc
{
    template<typename T>
    class CopyOnUpdPtr final {
    private:
        template<typename U, typename... Args>
        friend CopyOnUpdPtr<U> make_cow(Args&&... args);

        explicit CopyOnUpdPtr(std::shared_ptr<T> p) :
            m_Ptr{std::move(p)}
        {
        }
    public:
        CopyOnUpdPtr() = default;

        explicit CopyOnUpdPtr(std::unique_ptr<T> p) :
            m_Ptr{std::move(p)}
        {
        }

        T const* get() const noexcept
        {
            return m_Ptr.get();
        }

        T* upd()
        {
            if (m_Ptr.use_count() > 1)
            {
                m_Ptr = std::make_shared<T>(*m_Ptr);
            }
            return m_Ptr.get();
        }

        T const& operator*() const noexcept
        {
            return *get();
        }

        T const* operator->() const noexcept
        {
            return get();
        }

        explicit operator bool() const noexcept
        {
            return true;
        }

        friend void swap(CopyOnUpdPtr& a, CopyOnUpdPtr& b) noexcept
        {
            swap(a.m_Ptr, b.m_Ptr);
        }

    private:
        template<typename U, typename V>
        friend bool operator==(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator!=(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator<(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator>(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator<=(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator>=(CopyOnUpdPtr<U> const&, CopyOnUpdPtr<V> const&) noexcept;

        friend struct std::hash<CopyOnUpdPtr<T>>;

        std::shared_ptr<T> m_Ptr;
    };

    template<typename T, typename... Args>
    CopyOnUpdPtr<T> make_cow(Args&&... args)
    {
        return CopyOnUpdPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template<typename U, typename V>
    inline bool operator==(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr == b.m_Ptr;
    }

    template<typename U, typename V>
    inline bool operator!=(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr != b.m_Ptr;
    }

    template<typename U, typename V>
    inline bool operator<(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr < b.m_Ptr;
    }

    template<typename U, typename V>
    inline bool operator>(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr > b.m_Ptr;
    }

    template<typename U, typename V>
    inline bool operator<=(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr <= b.m_Ptr;
    }

    template<typename U, typename V>
    inline bool operator>=(CopyOnUpdPtr<U> const& a, CopyOnUpdPtr<V> const& b) noexcept
    {
        return a.m_Ptr >= b.m_Ptr;
    }
}

namespace std
{
    template<typename T>
    struct hash<osc::CopyOnUpdPtr<T>> final
    {
        size_t operator()(osc::CopyOnUpdPtr<T> const& cow) const
        {
            return std::hash<std::shared_ptr<T>>{}(cow.m_Ptr);
        }
    };
}