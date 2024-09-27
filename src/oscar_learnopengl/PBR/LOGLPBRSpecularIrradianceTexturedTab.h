#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLPBRSpecularIrradianceTexturedTab final : public Tab {
    public:
        static CStringView id();

        explicit LOGLPBRSpecularIrradianceTexturedTab(const ParentPtr<ITabHost>&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
