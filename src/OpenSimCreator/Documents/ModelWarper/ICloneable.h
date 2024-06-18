#pragma once

#include <memory>

namespace osc::mow
{
    // an interface to an object that can be cloned at runtime
    template<typename T>
    class ICloneable {
    protected:
        ICloneable() = default;
        ICloneable(const ICloneable&) = default;
        ICloneable(ICloneable&&) noexcept = default;
        ICloneable& operator=(const ICloneable&) = default;
        ICloneable& operator=(ICloneable&&) noexcept = default;
    public:
        virtual ~ICloneable() noexcept = default;

        std::unique_ptr<T> clone() const { return implClone(); }
    private:
        virtual std::unique_ptr<T> implClone() const = 0;
    };
}
