#pragma once

#include <libopensimcreator/Documents/ModelWarper/ICloneable.h>
#include <libopensimcreator/Documents/ModelWarper/IWarpDetailProvider.h>
#include <libopensimcreator/Documents/ModelWarper/IFrameWarper.h>
#include <libopensimcreator/Documents/ModelWarper/IValidateable.h>

namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // an interface to an object that can create an `IFrameWarper`
    class IFrameWarperFactory :
        public ICloneable<IFrameWarperFactory>,
        public IWarpDetailProvider,
        public IValidateable {
    protected:
        IFrameWarperFactory() = default;
        IFrameWarperFactory(const IFrameWarperFactory&) = default;
        IFrameWarperFactory& operator=(const IFrameWarperFactory&) = default;
        IFrameWarperFactory& operator=(IFrameWarperFactory&&) noexcept = default;
    public:
        virtual ~IFrameWarperFactory() noexcept = default;

        std::unique_ptr<IFrameWarper> tryCreateFrameWarper(const WarpableModel& document) const
        {
            return implTryCreateFrameWarper(document);
        }
    private:
        virtual std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(const WarpableModel& document) const = 0;
    };
}
