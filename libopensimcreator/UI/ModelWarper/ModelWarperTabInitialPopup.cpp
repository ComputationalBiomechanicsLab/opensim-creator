#include "ModelWarperTabInitialPopup.h"

#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/Popups/Popup.h>
#include <liboscar/UI/Popups/PopupPrivate.h>
#include <liboscar/UI/oscimgui.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::ModelWarperTabInitialPopup::Impl final : public PopupPrivate {
public:
    using PopupPrivate::PopupPrivate;

    void draw_content()
    {
        ui::draw_text_centered(OSC_ICON_MAGIC " This feature is experimental " OSC_ICON_MAGIC);
        ui::start_new_line();
        ui::draw_text_wrapped("The model warping UI is still work-in-progress. Which means that some datafiles may change over time.\n\nIf you would like a basic overview of how the model warping UI (and the associated mesh warping UI) work, please consult the documentation:");
        ui::start_new_line();
        const auto docsURL = OpenSimCreatorApp::get().docs_url();
        if (ui::draw_text_link(docsURL)) {
            open_url_in_os_default_web_browser(docsURL);
        }
        ui::start_new_line();
        if (ui::draw_button_centered("Close")) {
            request_close();
        }
    }
};

osc::ModelWarperTabInitialPopup::ModelWarperTabInitialPopup(
    Widget* parent,
    std::string_view popupName) :
    Popup{std::make_unique<Impl>(*this, parent, popupName)}
{}
void osc::ModelWarperTabInitialPopup::impl_draw_content() { private_data().draw_content(); }
