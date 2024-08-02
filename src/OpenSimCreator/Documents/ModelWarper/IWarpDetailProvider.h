#pragma once

#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <vector>

namespace osc::mow
{
    // an interface to an object that can provide user-facing warp details at runtime
    class IWarpDetailProvider {
    protected:
        IWarpDetailProvider() = default;
        IWarpDetailProvider(const IWarpDetailProvider&) = default;
        IWarpDetailProvider(IWarpDetailProvider&&) noexcept = default;
        IWarpDetailProvider& operator=(const IWarpDetailProvider&) = default;
        IWarpDetailProvider& operator=(IWarpDetailProvider&&) noexcept = default;
    public:
        virtual ~IWarpDetailProvider() noexcept = default;

        // returns a sequence of potentially-useful-to-know details about the
        // the object (e.g. parameters it's using, filesystem paths it's
        // referring to - anything that could help a user debug the system)
        std::vector<WarpDetail> details() const { return implWarpDetails(); }
    private:

        // by default, returns nothing (i.e. no details)
        //
        // overriders should return a sequence of potentially-useful-to-know details
        // about the object
        virtual std::vector<WarpDetail> implWarpDetails() const { return {}; }
    };
}
