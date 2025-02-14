#include "AddComponentPopup.h"

#include <libopensimcreator/ComponentRegistry/ComponentRegistry.h>
#include <libopensimcreator/ComponentRegistry/ComponentRegistryEntry.h>
#include <libopensimcreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

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
            AddComponentPopup popup{&parent, "popupname", model, entry.instantiate()};
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
