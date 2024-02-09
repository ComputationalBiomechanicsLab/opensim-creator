#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.hpp>

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.hpp>
#include <OpenSimCreator/UI/IPopupAPI.hpp>

#include <OpenSim/Common/Component.h>
#include <gtest/gtest.h>
#include <oscar/UI/ui_context.hpp>
#include <oscar/Utils/ScopeGuard.hpp>

#include <exception>
#include <memory>

using namespace osc;
namespace ui = osc::ui;

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
            ui::context::Init();
            ScopeGuard g{[]() { ui::context::Shutdown(); }};
            ui::context::NewFrame();
            NullPopupAPI api;
            auto model = std::make_shared<UndoableModelStatePair>();
            AddComponentPopup popup{"popupname", &api, model, entry.instantiate()};
            popup.open();
            popup.beginPopup();
            popup.onDraw();
            popup.endPopup();
            ui::context::Render();
        }
        catch (std::exception const& ex)
        {
            FAIL() << entry.name() << ": " << ex.what();
        }
    }
}
