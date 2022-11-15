#pragma once

#include <utility>

namespace osc
{
    class TypeErasedValueHolder {
    protected:
        TypeErasedValueHolder() = default;
        TypeErasedValueHolder(TypeErasedValueHolder const&) = default;
        TypeErasedValueHolder(TypeErasedValueHolder&&) noexcept = default;
        TypeErasedValueHolder& operator=(TypeErasedValueHolder const&) = default;
        TypeErasedValueHolder& operator=(TypeErasedValueHolder&&) noexcept = default;
    public:
        virtual ~TypeErasedValueHolder() noexcept = default;
    };

    template<class T>
    class ValueHolder : public TypeErasedValueHolder {
    public:
        template<class... Args>
        ValueHolder(Args&&... args) :
            m_Value{std::forward<Args>(args)...}
        {
        }

        T const& get() const noexcept
        {
            return m_Value;
        }

        T& upd() noexcept
        {
            return m_Value;
        }

    private:
        T m_Value;
    };
}