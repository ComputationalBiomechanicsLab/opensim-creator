#include "AddComponentPopup.h"

#include <libOpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <libOpenSimCreator/ComponentRegistry/ComponentRegistryEntry.h>
#include <libOpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libOpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <libOpenSimCreator/Platform/OpenSimCreatorApp.h>

#include <gtest/gtest.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/ScopeGuard.h>
#include <OpenSim/Common/Component.h>

#include <exception>
#include <memory>

using namespace osc;
namespace ui = osc::ui;

TEST(AddComponentPopup, CanOpenAndDrawAllRegisteredComponentsInTheAddComponentPopup)
{
    OpenSimCreatorApp app;
    ui::context::init(app);
    for (const auto& entry : GetAllRegisteredComponents()) {
        try {
            ui::context::on_start_new_frame(app);
            Widget parent;
            auto model = std::make_shared<UndoableModelStatePair>();
            AddComponentPopup popup{"popupname", parent, model, entry.instantiate()};
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
    ui::context::shutdown(App::upd());
}
