#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IWarpDetailProvider.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

namespace osc::mow { class ModelWarpDocument; }

namespace osc::mow
{
    // an interface to an object that can create an `IFrameWarper`
    class IFrameWarperFactory :
        public ICloneable<IFrameWarperFactory>,
        public IWarpDetailProvider,
        public IValidateable {
    protected:
        IFrameWarperFactory() = default;
        IFrameWarperFactory(IFrameWarperFactory const&) = default;
        IFrameWarperFactory& operator=(IFrameWarperFactory const&) = default;
        IFrameWarperFactory& operator=(IFrameWarperFactory&&) noexcept = default;
    public:
        virtual ~IFrameWarperFactory() noexcept = default;

        std::unique_ptr<IFrameWarper> tryCreateFrameWarper(ModelWarpDocument const& document) const
        {
            return implTryCreateFrameWarper(document);
        }
    private:
        virtual std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(ModelWarpDocument const& document) const = 0;
    };
}
