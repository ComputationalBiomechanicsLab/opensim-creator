#pragma once

#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <functional>

namespace osc::mow
{
    class IDetailListable {
    protected:
        IDetailListable() = default;
        IDetailListable(IDetailListable const&) = default;
        IDetailListable(IDetailListable&&) noexcept = default;
        IDetailListable& operator=(IDetailListable const&) = default;
        IDetailListable& operator=(IDetailListable&&) noexcept = default;
    public:
        virtual ~IDetailListable() noexcept = default;

        void forEachDetail(std::function<void(WarpDetail)> const& callback) const { implForEachDetail(callback); }

    private:
        virtual void implForEachDetail(std::function<void(WarpDetail)> const&) const = 0;
    };
}
