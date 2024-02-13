#pragma once

#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <vector>

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
        std::vector<WarpDetail> details() const { return implWarpDetails(); }
    private:
        virtual std::vector<WarpDetail> implWarpDetails() const = 0;
    };
}
