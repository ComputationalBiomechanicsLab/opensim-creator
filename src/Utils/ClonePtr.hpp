#pragma once

#include <type_traits>
#include <memory>
#include <utility>
#include <iosfwd>

namespace osc
{
    // a "smart pointer" that behaves exactly like a unique_ptr but
    // supports copy construction/assignment by calling `T::clone()`
    template<typename T, typename Deleter = std::default_delete<T>>
    class ClonePtr {
    public:
        typedef T* pointer;
        typedef T element_type;

        ClonePtr() noexcept : m_Value{nullptr}
        {
        }
        ClonePtr(std::nullptr_t) noexcept : m_Value{nullptr}
        {
        }
        explicit ClonePtr(T* ptr) noexcept : m_Value{ptr}
        {
            // takes ownership
        }
        ClonePtr(std::unique_ptr<T, Deleter> ptr) noexcept : m_Value{std::move(ptr)}
        {
            // takes ownership
        }
        explicit ClonePtr(T const& ref) : m_Value{ref.clone()}
        {
            // clones
        }
        ClonePtr(ClonePtr const& src) : m_Value{src.m_Value ? src.m_Value->clone() : nullptr}
        {
            // clones
        }

        ClonePtr(ClonePtr&& tmp) = default;

        template<typename U, typename E>
        ClonePtr(ClonePtr<U, E>&& tmp) : m_Value{std::move(tmp)}
        {
        }

        ~ClonePtr() noexcept = default;

        ClonePtr& operator=(ClonePtr const& other)
        {
            if (!other.m_Value)
            {
                m_Value.reset();
            }
            else if (&other != this)
            {
                m_Value = other.m_Value->clone();
            }
            return *this;
        }

        ClonePtr& operator=(T const& other)
        {
            if (m_Value)
            {
                *m_Value = other;
            }
            else
            {
                m_Value = std::make_unique<T>(other);
            }
            return *this;
        }

        ClonePtr& operator=(ClonePtr&&) = default;

        template<typename U, typename E>
        ClonePtr& operator=(ClonePtr<U, E>&& tmp) noexcept
        {
            m_Value = std::move(tmp.m_Value);
            return *this;
        }

        ClonePtr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        T* release() noexcept
        {
            return m_Value.release();
        }
        void reset(T* ptr = nullptr)
        {
            m_Value.reset(ptr);
        }
        void swap(ClonePtr& other)
        {
            m_Value.swap(other.m_Value);
        }
        T* get() const noexcept
        {
            return m_Value.get();
        }
        Deleter& get_deleter() noexcept
        {
            return m_Value.get_deleter();
        }
        Deleter const& get_deleter() const noexcept
        {
            return m_Value.get_deleter();
        }
        explicit operator bool() const noexcept
        {
            return m_Value;
        }
        T& operator*() const noexcept
        {
            return *m_Value;
        }
        T* operator->() const noexcept
        {
            return m_Value.get();
        }

    private:
        std::unique_ptr<T, Deleter> m_Value;
    };

    template<typename T1, typename D1, typename T2, typename D2>
    bool operator==(ClonePtr<T1, D1> const& x, ClonePtr<T2, D2> const& y)
    {
        return x.get() == y.get();
    }

    template<typename T, typename D>
    std::ostream& operator<<(std::ostream& o, ClonePtr<T, D> const& p)
    {
        return o << p.get();
    }
}

namespace std {
    template<typename T, typename D>
    void swap(osc::ClonePtr<T, D>& lhs, osc::ClonePtr<T, D>& rhs)
    {
        lhs.swap(rhs);
    }

    template<typename T, typename D>
    struct hash<osc::ClonePtr<T, D>> {
        size_t operator()(osc::ClonePtr<T, D> const& p)
        {
            return std::hash(p.get());
        }
    };
}
