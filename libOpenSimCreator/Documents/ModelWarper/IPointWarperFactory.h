#pragma once

#include <libOpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <libOpenSimCreator/Documents/ModelWarper/IWarpDetailProvider.h>
#include <libOpenSimCreator/Documents/ModelWarper/IPointWarper.h>
#include <libOpenSimCreator/Documents/ModelWarper/IValidateable.h>

#include <liboscar/Maths/Vec3.h>

#include <memory>
#include <span>

namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // an interface to an object that can create an `IPointWarper`
    class IPointWarperFactory :
        public ICloneable<IPointWarperFactory>,
        public IWarpDetailProvider,
        public IValidateable {
    protected:
        IPointWarperFactory() = default;
        IPointWarperFactory(const IPointWarperFactory&) = default;
        IPointWarperFactory(IPointWarperFactory&&) noexcept = default;
        IPointWarperFactory& operator=(const IPointWarperFactory&) = default;
        IPointWarperFactory& operator=(IPointWarperFactory&&) noexcept = default;
    public:
        virtual ~IPointWarperFactory() = default;

        std::unique_ptr<IPointWarper> tryCreatePointWarper(const WarpableModel& document) const { return implTryCreatePointWarper(document); }
    private:
        virtual std::unique_ptr<IPointWarper> implTryCreatePointWarper(const WarpableModel&) const = 0;
    };
}
