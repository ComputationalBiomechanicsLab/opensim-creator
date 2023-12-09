#pragma once

#include <memory>

namespace osc::mow
{
    class IWarpableFrameDefinition {
    protected:
        IWarpableFrameDefinition() = default;
        IWarpableFrameDefinition(IWarpableFrameDefinition const&) = default;
        IWarpableFrameDefinition(IWarpableFrameDefinition&&) noexcept = default;
        IWarpableFrameDefinition& operator=(IWarpableFrameDefinition const&) = default;
        IWarpableFrameDefinition& operator=(IWarpableFrameDefinition&&) noexcept = default;
    public:
        virtual ~IWarpableFrameDefinition() noexcept = default;

        std::unique_ptr<IWarpableFrameDefinition> clone() const
        {
            return implClone();
        }

        // TODO: figure out what the virtual interface would need in order to satisfy both
        //       the requirements of either using point-based definitions (preferred) or
        //       least-squares definitions (Julia's approach)
    private:
        virtual std::unique_ptr<IWarpableFrameDefinition> implClone() const = 0;
    };
}
