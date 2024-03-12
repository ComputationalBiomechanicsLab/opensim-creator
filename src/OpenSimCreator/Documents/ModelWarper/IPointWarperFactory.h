#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IWarpDetailProvider.h>
#include <OpenSimCreator/Documents/ModelWarper/IPointWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

#include <oscar/Maths/Vec3.h>

#include <memory>
#include <span>

namespace osc::mow { class ModelWarpDocument; }

namespace osc::mow
{
    // an interface to an object that can create an `IPointWarper`
    class IPointWarperFactory :
        public ICloneable<IPointWarperFactory>,
        public IWarpDetailProvider,
        public IValidateable {
    protected:
        IPointWarperFactory() = default;
        IPointWarperFactory(IPointWarperFactory const&) = default;
        IPointWarperFactory(IPointWarperFactory&&) noexcept = default;
        IPointWarperFactory& operator=(IPointWarperFactory const&) = default;
        IPointWarperFactory& operator=(IPointWarperFactory&&) noexcept = default;
    public:
        virtual ~IPointWarperFactory() = default;

        std::unique_ptr<IPointWarper> tryCreatePointWarper(ModelWarpDocument const& document) const { return implTryCreatePointWarper(document); }
    private:
        virtual std::unique_ptr<IPointWarper> implTryCreatePointWarper(ModelWarpDocument const&) const = 0;
    };
}
