#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

namespace osc
{
    template<typename T>
    struct CowData final {
        template<typename... Args>
        CowData(Args&&... args) : data{std::forward<Args>(args)...} {}

        std::atomic<int64_t> owners = 1;
        T data;
    };

    template<typename T>
    class Cow final {
    public:
        template<typename... Args>
        Cow(Args&&... args) :
            m_Data{new CowData<T>{std::forward<Args>(args)...}}
        {
        }

        Cow(Cow const& other) noexcept :
            m_Data{other.m_Data}
        {
            m_Data->owners.fetch_add(1, std::memory_order_relaxed);
        }

        Cow(Cow&& tmp) noexcept :
            m_Data{std::exchange(tmp.m_Data, nullptr)}
        {
        }

        Cow& operator=(Cow const& tmp) noexcept
        {
            Cow cpy{tmp};
            swap(cpy, *this);
            return *this;
        }

        Cow& operator=(Cow&& tmp) noexcept
        {
            using std::swap;

            swap(m_Data, tmp.m_Data);
            return *this;
        }

        ~Cow() noexcept
        {
            if (m_Data && m_Data->owners.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                delete m_Data;
            }
        }

        T const* get() const noexcept
        {
            return &m_Data->data;
        }

        T* upd()
        {
            if (m_Data->owners == 1)
            {
                return &m_Data->data;
            }
            else
            {
                Cow copy{m_Data->data};
                swap(copy, *this);
                return &m_Data->data;
            }
        }

        T const& operator*() const noexcept
        {
            return *get();
        }

        T const* operator->() const noexcept
        {
            return get();
        }

        int64_t use_count() const noexcept
        {
            return m_Data->owners;
        }

        operator bool() const noexcept
        {
            return true;
        }

        friend void swap(Cow& a, Cow& b) noexcept
        {
            CowData<T>* tmp = a.m_Data;
            a.m_Data = b.m_Data;
            b.m_Data = tmp;
        }

    private:
        template<typename U, typename V>
        friend bool operator==(Cow<U> const&, Cow<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator!=(Cow<U> const&, Cow<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator<(Cow<U> const&, Cow<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator>(Cow<U> const&, Cow<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator<=(Cow<U> const&, Cow<V> const&) noexcept;

        template<typename U, typename V>
        friend bool operator>=(Cow<U> const&, Cow<V> const&) noexcept;

        CowData<T>* m_Data = nullptr;
    };

    template<typename T, typename... Args>
    Cow<T> make_cow(Args&&... args)
    {
        return Cow<T>(std::forward<Args>(args)...);
    }

    template<typename U, typename V>
    inline bool operator==(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data == b.m_Data;
    }

    template<typename U, typename V>
    inline bool operator!=(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data != b.m_Data;
    }

    template<typename U, typename V>
    inline bool operator<(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data < b.m_Data;
    }

    template<typename U, typename V>
    inline bool operator>(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data > b.m_Data;
    }

    template<typename U, typename V>
    inline bool operator<=(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data <= b.m_Data;
    }

    template<typename U, typename V>
    inline bool operator>=(Cow<U> const& a, Cow<V> const& b) noexcept
    {
        return a.m_Data >= b.m_Data;
    }
}