#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLHelloTriangleTab final : public Tab {
    public:
        static CStringView id();

        explicit LOGLHelloTriangleTab(const ParentPtr<ITabHost>&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
