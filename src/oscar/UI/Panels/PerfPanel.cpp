#include "PerfPanel.h"

#include <oscar/Platform/App.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/Perf.h>

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <memory>
#include <ranges>
#include <string_view>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

class osc::PerfPanel::Impl final : public StandardPanelImpl {
public:

    explicit Impl(std::string_view panel_name) :
        StandardPanelImpl{panel_name}
    {}

private:
    void impl_draw_content() final
    {
        ui::Columns(2);
        ui::TextUnformatted("FPS");
        ui::NextColumn();
        ui::Text("%.0f", static_cast<double>(ui::GetIO().Framerate));
        ui::NextColumn();
        ui::Columns();

        {
            bool waiting = App::get().is_main_loop_waiting();
            if (ui::Checkbox("waiting", &waiting)) {
                App::upd().set_main_loop_waiting(waiting);
            }
        }
        {
            bool vsync = App::get().is_vsync_enabled();
            if (ui::Checkbox("VSYNC", &vsync)) {
                App::upd().set_vsync(vsync);
            }
        }
        if (ui::Button("clear measurements")) {
            ClearAllPerfMeasurements();
        }
        ui::Checkbox("pause", &is_paused_);

        std::vector<PerfMeasurement> measurements;
        if (not is_paused_) {
            measurements = GetAllPerfMeasurements();
            rgs::sort(measurements, rgs::less{}, &PerfMeasurement::getLabel);
        }

        const ImGuiTableFlags flags =
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInner;

        if (ui::BeginTable("measurements", 6, flags)) {
            ui::TableSetupColumn("Label");
            ui::TableSetupColumn("Source File");
            ui::TableSetupColumn("Num Calls");
            ui::TableSetupColumn("Last Duration");
            ui::TableSetupColumn("Average Duration");
            ui::TableSetupColumn("Total Duration");
            ui::TableHeadersRow();

            for (const PerfMeasurement& pm : measurements) {

                if (pm.getCallCount() <= 0) {
                    continue;
                }

                int column = 0;
                ui::TableNextRow();
                ui::TableSetColumnIndex(column++);
                ui::TextUnformatted(pm.getLabel());
                ui::TableSetColumnIndex(column++);
                ui::Text("%s:%u", pm.getFilename().c_str(), pm.getLine());
                ui::TableSetColumnIndex(column++);
                ui::Text("%zu", pm.getCallCount());
                ui::TableSetColumnIndex(column++);
                ui::Text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getLastDuration()).count()));
                ui::TableSetColumnIndex(column++);
                ui::Text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getAvgDuration()).count()));
                ui::TableSetColumnIndex(column++);
                ui::Text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getTotalDuration()).count()));
            }

            ui::EndTable();
        }
    }

    bool is_paused_ = false;
};

osc::PerfPanel::PerfPanel(std::string_view panel_name) :
    impl_{std::make_unique<Impl>(panel_name)}
{}

osc::PerfPanel::PerfPanel(PerfPanel&&) noexcept = default;
osc::PerfPanel& osc::PerfPanel::operator=(PerfPanel&&) noexcept = default;
osc::PerfPanel::~PerfPanel() noexcept = default;

CStringView osc::PerfPanel::impl_get_name() const
{
    return impl_->name();
}

bool osc::PerfPanel::impl_is_open() const
{
    return impl_->is_open();
}

void osc::PerfPanel::impl_open()
{
    return impl_->open();
}

void osc::PerfPanel::impl_close()
{
    impl_->close();
}

void osc::PerfPanel::impl_on_draw()
{
    impl_->on_draw();
}
