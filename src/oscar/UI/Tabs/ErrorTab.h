#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <exception>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ErrorTab final : public Tab {
    public:
        ErrorTab(const ParentPtr<ITabHost>&, const std::exception&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
