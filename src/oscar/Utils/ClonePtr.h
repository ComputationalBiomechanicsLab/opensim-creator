#pragma once

#include <iosfwd>
#include <memory>
#include <type_traits>
#include <utility>

namespace osc
{
    // a "smart pointer" that behaves exactly like a unique_ptr but
    // supports copy construction/assignment by calling `T::clone()`
    template<typename T, typename Deleter = std::default_delete<T>>
    class ClonePtr final {
    public:
        typedef T* pointer;
        typedef T element_type;

        ClonePtr() : m_Value{nullptr}
        {
        }
        ClonePtr(std::nullptr_t) : m_Value{nullptr}
        {
        }
        explicit ClonePtr(T* ptr) noexcept : m_Value{ptr}
        {
            // takes ownership
        }
        explicit ClonePtr(std::unique_ptr<T, Deleter> ptr) : m_Value{std::move(ptr)}
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

        ClonePtr(ClonePtr&&) noexcept = default;

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
                m_Value = std::unique_ptr<T>{other.m_Value->clone()};
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

        ClonePtr& operator=(ClonePtr&&) noexcept = default;

        template<typename U, typename E>
        ClonePtr& operator=(ClonePtr<U, E>&& tmp) noexcept
        {
            m_Value = std::move(tmp.m_Value);
            return *this;
        }

        ClonePtr& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }

        T* release()
        {
            return m_Value.release();
        }
        void reset(T* ptr = nullptr)
        {
            m_Value.reset(ptr);
        }
        void swap(ClonePtr& other) noexcept
        {
            m_Value.swap(other.m_Value);
        }
        T* get() const
        {
            return m_Value.get();
        }
        Deleter& get_deleter()
        {
            return m_Value.get_deleter();
        }
        Deleter const& get_deleter() const
        {
            return m_Value.get_deleter();
        }
        explicit operator bool() const
        {
            return m_Value;
        }
        T& operator*() const
        {
            return *m_Value;
        }
        T* operator->() const
        {
            return m_Value.get();
        }

        friend void swap(ClonePtr& lhs, ClonePtr& rhs) noexcept
        {
            lhs.swap(rhs);
        }

        template<typename T2, typename D2>
        friend bool operator==(ClonePtr const& lhs, ClonePtr<T2, D2> const& rhs)
        {
            return lhs.get() == rhs.get();
        }

        friend std::ostream& operator<<(std::ostream& o, ClonePtr const& p)
        {
            return o << p.get();
        }

    private:
        std::unique_ptr<T, Deleter> m_Value;
    };
}

template<typename T, typename D>
struct std::hash<osc::ClonePtr<T, D>> final {
    size_t operator()(osc::ClonePtr<T, D> const& p)
    {
        return std::hash(p.get());
    }
};
