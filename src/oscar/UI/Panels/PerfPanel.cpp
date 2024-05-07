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
        ui::set_num_columns(2);
        ui::draw_text_unformatted("FPS");
        ui::next_column();
        ui::draw_text("%.0f", static_cast<double>(ui::get_io().Framerate));
        ui::next_column();
        ui::set_num_columns();

        {
            bool waiting = App::get().is_main_loop_waiting();
            if (ui::draw_checkbox("waiting", &waiting)) {
                App::upd().set_main_loop_waiting(waiting);
            }
        }
        {
            bool vsync = App::get().is_vsync_enabled();
            if (ui::draw_checkbox("VSYNC", &vsync)) {
                App::upd().set_vsync(vsync);
            }
        }
        if (ui::draw_button("clear measurements")) {
            ClearAllPerfMeasurements();
        }
        ui::draw_checkbox("pause", &is_paused_);

        std::vector<PerfMeasurement> measurements;
        if (not is_paused_) {
            measurements = GetAllPerfMeasurements();
            rgs::sort(measurements, rgs::less{}, &PerfMeasurement::getLabel);
        }

        const ImGuiTableFlags flags =
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInner;

        if (ui::begin_table("measurements", 6, flags)) {
            ui::table_setup_column("Label");
            ui::table_setup_column("Source File");
            ui::table_setup_column("Num Calls");
            ui::table_setup_column("Last Duration");
            ui::table_setup_column("Average Duration");
            ui::table_setup_column("Total Duration");
            ui::table_headers_row();

            for (const PerfMeasurement& measurement : measurements) {

                if (measurement.getCallCount() <= 0) {
                    continue;
                }

                int column = 0;
                ui::table_next_row();
                ui::table_set_column_index(column++);
                ui::draw_text_unformatted(measurement.getLabel());
                ui::table_set_column_index(column++);
                ui::draw_text("%s:%u", measurement.getFilename().c_str(), measurement.getLine());
                ui::table_set_column_index(column++);
                ui::draw_text("%zu", measurement.getCallCount());
                ui::table_set_column_index(column++);
                ui::draw_text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(measurement.getLastDuration()).count()));
                ui::table_set_column_index(column++);
                ui::draw_text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(measurement.getAvgDuration()).count()));
                ui::table_set_column_index(column++);
                ui::draw_text("%" PRId64 " us", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(measurement.getTotalDuration()).count()));
            }

            ui::end_table();
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
