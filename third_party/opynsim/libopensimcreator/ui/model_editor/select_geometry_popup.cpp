#include "select_geometry_popup.h"

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/filesystem_helpers.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    using Geom_ctor_fn = std::unique_ptr<OpenSim::Geometry>();

    constexpr auto c_geom_ctors = std::to_array<Geom_ctor_fn*>({
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            auto ptr = std::make_unique<OpenSim::Brick>();
            ptr->set_half_lengths(SimTK::Vec3{0.1, 0.1, 0.1});
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            auto ptr = std::make_unique<OpenSim::Sphere>();
            ptr->set_radius(0.1);
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            auto ptr = std::make_unique<OpenSim::Cylinder>();
            ptr->set_radius(0.1);
            ptr->set_half_height(0.1);
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            return std::make_unique<OpenSim::LineGeometry>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            return std::make_unique<OpenSim::Ellipsoid>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            return std::make_unique<OpenSim::Arrow>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry> {
            return std::make_unique<OpenSim::Cone>();
        }},
    });

    constexpr auto c_geom_names = std::to_array<CStringView>({
        "Brick",
        "Sphere",
        "Cylinder",
        "LineGeometry",
        "Ellipsoid",
        "Arrow (CARE: may not work in OpenSim's main UI)",
        "Cone",
    });
    static_assert(c_geom_ctors.size() == c_geom_names.size());

    std::unique_ptr<OpenSim::Mesh> load_geometry_file(const std::filesystem::path& p)
    {
        return std::make_unique<OpenSim::Mesh>(p.string());
    }
}

class osc::SelectGeometryPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        SelectGeometryPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::optional<std::filesystem::path> geometry_dir,
        std::function<void(std::unique_ptr<OpenSim::Geometry>)> on_selection) :

        PopupPrivate{owner, parent, popup_name},
        on_selection_{std::move(on_selection)},
        geometry_files_{
            geometry_dir ? find_files_recursive(*geometry_dir)
                         : std::vector<std::filesystem::path>{}}
    {}

    void draw_content()
    {
        // premade geometry section
        //
        // let user select from a shorter sequence of analytical geometry that can be
        // generated without a mesh file
        {
            ui::draw_text("Generated geometry");
            ui::same_line();
            ui::draw_help_marker(
                "This is geometry that OpenSim can generate without needing an external mesh file. Useful for basic geometry.");
            ui::draw_separator();
            ui::draw_vertical_spacer(2.0f / 15.0f);

            size_t item = 0;
            if (ui::draw_combobox("##premade", &item, c_geom_names)) {
                const auto& ctor = c_geom_ctors.at(item);
                result_ = ctor();
            }
        }

        // mesh file selection
        //
        // let the user select a mesh file that the implementation should load + use
        ui::draw_vertical_spacer(3.0f / 15.0f);
        ui::draw_text("mesh file");
        ui::same_line();
        ui::draw_help_marker(
            "This is geometry that OpenSim loads from external mesh files. Useful for custom geometry (usually, created in some other application, such as ParaView or Blender)");
        ui::draw_separator();
        ui::draw_vertical_spacer(2.0f / 15.0f);

        // let the user search through mesh files in pre-established Geometry/ dirs
        ui::draw_string_input("search", search_);
        ui::draw_vertical_spacer(1.0f / 15.0f);

        ui::begin_child_panel(
            "mesh list",
            Vector2{ui::get_content_region_available().x(), 256},
            ui::ChildPanelFlags{},
            ui::PanelFlag::HorizontalScrollbar);

        if (not recent_user_choices_.empty()) {
            ui::draw_text_disabled("  (recent)");
        }

        for (const std::filesystem::path& p : recent_user_choices_) {
            auto resp = try_draw_file_choice(p);
            if (resp) {
                result_ = std::move(resp);
            }
        }

        if (not recent_user_choices_.empty()) {
            ui::draw_text_disabled("  (from Geometry/ dir)");
        }
        for (const std::filesystem::path& p : geometry_files_) {
            auto resp = try_draw_file_choice(p);
            if (resp) {
                result_ = std::move(resp);
            }
        }

        ui::end_child_panel();

        if (ui::draw_button("Open Mesh File")) {
            prompt_user_for_geometry_file();
        }
        ui::draw_tooltip_if_item_hovered("Open Mesh File", "Open a mesh file on the filesystem");

        ui::draw_vertical_spacer(5.0f / 15.0f);

        if (ui::draw_button("Cancel")) {
            search_.clear();
            request_close();
        }

        if (result_) {
            on_selection_(std::move(result_));
            search_.clear();
            request_close();
        }
    }

private:
    void prompt_user_for_geometry_file()
    {
        App::upd().prompt_user_to_select_file_async(
            [this, this_lifetime = this->lifetime_watcher()](const FileDialogResponse& response) {
                if (this_lifetime.expired()) {
                    return;  // `this` has expired
                }

                if (response.size() != 1) {
                    return;  // user cancelled, or somehow selected multiple files?
                }

                on_selection_(on_mesh_file_chosen(response.front()));
            },
            opyn::get_supported_sim_tk_mesh_formats_as_filters());
    }

    std::unique_ptr<OpenSim::Mesh> on_mesh_file_chosen(std::filesystem::path path)
    {
        auto rv = load_geometry_file(path);

        // add to recent list
        recent_user_choices_.push_back(std::move(path));

        // reset search (for next popup open)
        search_.clear();

        request_close();

        return rv;
    }

    std::unique_ptr<OpenSim::Mesh> try_draw_file_choice(const std::filesystem::path& p)
    {
        if (p.filename().string().contains(search_)) {
            if (ui::draw_selectable(p.filename().string())) {
                return on_mesh_file_chosen(p.filename());
            }
        }
        return nullptr;
    }

    // holding space for result
    std::unique_ptr<OpenSim::Geometry> result_;

    // callback that's called with the geometry
    std::function<void(std::unique_ptr<OpenSim::Geometry>)> on_selection_;

    // geometry files found in the user's/installation's `Geometry/` dir
    std::vector<std::filesystem::path> geometry_files_;

    // recent file choices by the user
    std::vector<std::filesystem::path> recent_user_choices_;

    // the user's current search filter
    std::string search_;
};

osc::SelectGeometryPopup::SelectGeometryPopup(
    Widget* parent,
    std::string_view popup_name,
    std::optional<std::filesystem::path> geometry_dir,
    std::function<void(std::unique_ptr<OpenSim::Geometry>)> on_selection) :

    Popup{std::make_unique<Impl>(
        *this, parent, popup_name, std::move(geometry_dir), std::move(on_selection))}
{}
void osc::SelectGeometryPopup::impl_draw_content()
{
    private_data().draw_content();
}
