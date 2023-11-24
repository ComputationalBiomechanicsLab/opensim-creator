#include <OpenSimCreator/UI/Widgets/AddComponentPopup.hpp>

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.hpp>
#include <OpenSimCreator/UI/Middleware/PopupAPI.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Common/Component.h>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Widgets/Popup.hpp>
#include <oscar/Utils/ScopeGuard.hpp>

#include <memory>

using osc::AddComponentPopup;
using osc::OpenSimCreatorApp;
using osc::Popup;
using osc::PopupAPI;
using osc::ScopeGuard;
using osc::UndoableModelStatePair;

namespace
{
    class NullPopupAPI : public PopupAPI {
    private:
        void implPushPopup(std::unique_ptr<Popup>) final {}
    };
}

TEST(AddComponentPopup, CanOpenAndDrawAllRegisteredComponentsInTheAddComponentPopup)
{
    OpenSimCreatorApp app;

    for (auto const& entry : osc::GetAllRegisteredComponents())
    {
        try
        {
            osc::ImGuiInit();
            ScopeGuard g{[]() { osc::ImGuiShutdown(); }};
            osc::ImGuiNewFrame();
            NullPopupAPI api;
            auto model = std::make_shared<UndoableModelStatePair>();
            AddComponentPopup popup{"popupname", &api, model, entry.instantiate()};
            popup.open();
            popup.beginPopup();
            popup.onDraw();
            popup.endPopup();
            osc::ImGuiRender();
        }
        catch (std::exception const& ex)
        {
            FAIL() << entry.name() << ": " << ex.what();
        }
    }
}
