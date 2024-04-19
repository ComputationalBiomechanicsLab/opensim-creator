#pragma once

#include <oscar/Utils/Concepts.h>

#include <functional>
#include <memory>
#include <utility>

namespace osc
{
    template<typename T>
    class CopyOnUpdPtr final {
    private:
        template<typename U, typename... Args>
        friend CopyOnUpdPtr<U> make_cow(Args&&...);

        explicit CopyOnUpdPtr(std::shared_ptr<T> p) :
            m_Ptr{std::move(p)}
        {
        }
    public:

        const T* get() const
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

        const T& operator*() const
        {
            return *get();
        }

        const T* operator->() const
        {
            return get();
        }

        friend void swap(CopyOnUpdPtr& a, CopyOnUpdPtr& b) noexcept
        {
            swap(a.m_Ptr, b.m_Ptr);
        }

        template<typename U>
        friend bool operator==(const CopyOnUpdPtr& lhs, const CopyOnUpdPtr<U>& rhs)
        {
            return lhs.m_Ptr == rhs.m_Ptr;
        }

        template<typename U>
        friend auto operator<=>(const CopyOnUpdPtr& lhs, const CopyOnUpdPtr<U>& rhs)
        {
            return lhs.m_Ptr <=> rhs.m_Ptr;
        }

    private:
        friend struct std::hash<CopyOnUpdPtr>;

        std::shared_ptr<T> m_Ptr;
    };

    template<typename T, typename... Args>
    CopyOnUpdPtr<T> make_cow(Args&&... args)
    {
        return CopyOnUpdPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }
}

template<typename T>
struct std::hash<osc::CopyOnUpdPtr<T>> final {
    size_t operator()(const osc::CopyOnUpdPtr<T>& cow) const
    {
        return std::hash<std::shared_ptr<T>>{}(cow.m_Ptr);
    }
};
