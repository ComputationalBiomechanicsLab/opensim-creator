#pragma once

#include <memory>

namespace osc
{
    template<typename T>
    class ICloneable {
    protected:
        ICloneable() = default;
        ICloneable(ICloneable const&) = default;
        ICloneable(ICloneable&&) noexcept = default;
        ICloneable& operator=(ICloneable const&) = default;
        ICloneable& operator=(ICloneable&&) noexcept = default;
    public:
        virtual ~ICloneable() noexcept = default;

        std::unique_ptr<T> clone() const { return implClone(); }
    private:
        virtual std::unique_ptr<T> implClone() const = 0;
    };
}
