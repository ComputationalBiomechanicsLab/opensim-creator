#include "ModelWarperTabInitialPopup.h"

#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Popups/StandardPopup.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::ModelWarperTabInitialPopup::Impl final : public StandardPopup {
public:
    using StandardPopup::StandardPopup;
private:
    void impl_draw_content() final
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

osc::ModelWarperTabInitialPopup::ModelWarperTabInitialPopup(std::string_view popupName) :
    m_Impl{std::make_unique<Impl>(popupName)}
{}
osc::ModelWarperTabInitialPopup::ModelWarperTabInitialPopup(ModelWarperTabInitialPopup&&) noexcept = default;
ModelWarperTabInitialPopup& osc::ModelWarperTabInitialPopup::operator=(ModelWarperTabInitialPopup&&) noexcept = default;
osc::ModelWarperTabInitialPopup::~ModelWarperTabInitialPopup() noexcept = default;

bool osc::ModelWarperTabInitialPopup::impl_is_open() const { return m_Impl->is_open(); }
void osc::ModelWarperTabInitialPopup::impl_open() { m_Impl->open(); }
void osc::ModelWarperTabInitialPopup::impl_close() { m_Impl->close(); }
bool osc::ModelWarperTabInitialPopup::impl_begin_popup() { return m_Impl->begin_popup(); }
void osc::ModelWarperTabInitialPopup::impl_on_draw() { m_Impl->on_draw(); }
void osc::ModelWarperTabInitialPopup::impl_end_popup() { m_Impl->end_popup(); }
