#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.hpp>

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.hpp>
#include <OpenSimCreator/UI/IPopupAPI.hpp>

#include <OpenSim/Common/Component.h>
#include <gtest/gtest.h>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Widgets/IPopup.hpp>
#include <oscar/Utils/ScopeGuard.hpp>

#include <memory>

using osc::AddComponentPopup;
using osc::GetAllRegisteredComponents;
using osc::ImGuiInit;
using osc::ImGuiNewFrame;
using osc::ImGuiRender;
using osc::ImGuiShutdown;
using osc::IPopup;
using osc::IPopupAPI;
using osc::OpenSimCreatorApp;
using osc::ScopeGuard;
using osc::UndoableModelStatePair;

namespace
{
    class NullPopupAPI : public IPopupAPI {
    private:
        void implPushPopup(std::unique_ptr<IPopup>) final {}
    };
}

TEST(AddComponentPopup, CanOpenAndDrawAllRegisteredComponentsInTheAddComponentPopup)
{
    OpenSimCreatorApp app;

    for (auto const& entry : GetAllRegisteredComponents())
    {
        try
        {
            ImGuiInit();
            ScopeGuard g{[]() { ImGuiShutdown(); }};
            ImGuiNewFrame();
            NullPopupAPI api;
            auto model = std::make_shared<UndoableModelStatePair>();
            AddComponentPopup popup{"popupname", &api, model, entry.instantiate()};
            popup.open();
            popup.beginPopup();
            popup.onDraw();
            popup.endPopup();
            ImGuiRender();
        }
        catch (std::exception const& ex)
        {
            FAIL() << entry.name() << ": " << ex.what();
        }
    }
}
