#include <OpenSimCreator/UI/ModelEditor/AddComponentPopup.h>

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <OpenSimCreator/UI/IPopupAPI.h>

#include <OpenSim/Common/Component.h>
#include <gtest/gtest.h>
#include <oscar/UI/ui_context.h>
#include <oscar/Utils/ScopeGuard.h>

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
    ui::context::init(app);
    for (const auto& entry : GetAllRegisteredComponents()) {
        try {
            ui::context::on_start_new_frame(app);
            NullPopupAPI api;
            auto model = std::make_shared<UndoableModelStatePair>();
            AddComponentPopup popup{"popupname", &api, model, entry.instantiate()};
            popup.open();
            popup.begin_popup();
            popup.on_draw();
            popup.end_popup();
            ui::context::render();
        }
        catch (const std::exception& ex) {
            FAIL() << entry.name() << ": " << ex.what();
        }
    }
    ui::context::shutdown();
}
