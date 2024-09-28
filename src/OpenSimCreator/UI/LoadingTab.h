#pragma once

#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>

namespace osc
{
    class LoadingTab final : public Tab {
    public:
        LoadingTab(const ParentPtr<IMainUIStateAPI>&, std::filesystem::path);

    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
