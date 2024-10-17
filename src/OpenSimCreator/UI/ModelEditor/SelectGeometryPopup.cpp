#include "SelectGeometryPopup.h"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/FilesystemHelpers.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    using Geom_ctor_fn = std::unique_ptr<OpenSim::Geometry>();

    constexpr auto c_GeomCtors = std::to_array<Geom_ctor_fn*>(
    {
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            auto ptr = std::make_unique<OpenSim::Brick>();
            ptr->set_half_lengths(SimTK::Vec3{0.1, 0.1, 0.1});
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            auto ptr = std::make_unique<OpenSim::Sphere>();
            ptr->set_radius(0.1);
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            auto ptr = std::make_unique<OpenSim::Cylinder>();
            ptr->set_radius(0.1);
            ptr->set_half_height(0.1);
            return ptr;
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            return std::make_unique<OpenSim::LineGeometry>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            return std::make_unique<OpenSim::Ellipsoid>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            return std::make_unique<OpenSim::Arrow>();
        }},
        std::decay_t<Geom_ctor_fn>{[]() -> std::unique_ptr<OpenSim::Geometry>
        {
            return std::make_unique<OpenSim::Cone>();
        }},
    });

    constexpr auto c_GeomNames = std::to_array<CStringView>(
    {
        "Brick",
        "Sphere",
        "Cylinder",
        "LineGeometry",
        "Ellipsoid",
        "Arrow (CARE: may not work in OpenSim's main UI)",
        "Cone",
    });
    static_assert(c_GeomCtors.size() == c_GeomNames.size());

    std::optional<std::filesystem::path> PromptUserForGeometryFile()
    {
        return prompt_user_to_select_file(GetSupportedSimTKMeshFormats());
    }

    std::unique_ptr<OpenSim::Mesh> LoadGeometryFile(const std::filesystem::path& p)
    {
        return std::make_unique<OpenSim::Mesh>(p.string());
    }
}

class osc::SelectGeometryPopup::Impl final : public StandardPopup {
public:
    Impl(std::string_view popupName,
         const std::filesystem::path& geometryDir,
         std::function<void(std::unique_ptr<OpenSim::Geometry>)> onSelection) :

        StandardPopup{popupName},
        m_OnSelection{std::move(onSelection)},
        m_GeometryFiles{find_files_recursive(geometryDir)}
    {
    }

private:

    void impl_draw_content() final
    {
        // premade geometry section
        //
        // let user select from a shorter sequence of analytical geometry that can be
        // generated without a mesh file
        {
            ui::draw_text_unformatted("Generated geometry");
            ui::same_line();
            ui::draw_help_marker("This is geometry that OpenSim can generate without needing an external mesh file. Useful for basic geometry.");
            ui::draw_separator();
            ui::draw_dummy({0.0f, 2.0f});

            size_t item = 0;
            if (ui::draw_combobox("##premade", &item, c_GeomNames))
            {
                const auto& ctor = c_GeomCtors.at(item);
                m_Result = ctor();
            }
        }

        // mesh file selection
        //
        // let the user select a mesh file that the implementation should load + use
        ui::draw_dummy({0.0f, 3.0f});
        ui::draw_text_unformatted("mesh file");
        ui::same_line();
        ui::draw_help_marker("This is geometry that OpenSim loads from external mesh files. Useful for custom geometry (usually, created in some other application, such as ParaView or Blender)");
        ui::draw_separator();
        ui::draw_dummy({0.0f, 2.0f});

        // let the user search through mesh files in pre-established Geometry/ dirs
        ui::draw_string_input("search", m_Search);
        ui::draw_dummy({0.0f, 1.0f});

        ui::begin_child_panel(
            "mesh list",
            Vec2{ui::get_content_region_available().x, 256},
            ui::ChildPanelFlags{},
            ui::WindowFlag::HorizontalScrollbar);

        if (!m_RecentUserChoices.empty())
        {
            ui::draw_text_disabled("  (recent)");
        }

        for (const std::filesystem::path& p : m_RecentUserChoices)
        {
            auto resp = tryDrawFileChoice(p);
            if (resp)
            {
                m_Result = std::move(resp);
            }
        }

        if (!m_RecentUserChoices.empty())
        {
            ui::draw_text_disabled("  (from Geometry/ dir)");
        }
        for (const std::filesystem::path& p : m_GeometryFiles)
        {
            auto resp = tryDrawFileChoice(p);
            if (resp)
            {
                m_Result = std::move(resp);
            }
        }

        ui::end_child_panel();

        if (ui::draw_button("Open Mesh File"))
        {
            if (auto maybeMeshFile = PromptUserForGeometryFile())
            {
                m_Result = onMeshFileChosen(std::move(maybeMeshFile).value());
            }
        }
        ui::draw_tooltip_if_item_hovered("Open Mesh File", "Open a mesh file on the filesystem");

        ui::draw_dummy({0.0f, 5.0f});

        if (ui::draw_button("Cancel"))
        {
            m_Search.clear();
            request_close();
        }

        if (m_Result)
        {
            m_OnSelection(std::move(m_Result));
            m_Search.clear();
            request_close();
        }
    }

    std::unique_ptr<OpenSim::Mesh> onMeshFileChosen(std::filesystem::path path)
    {
        auto rv = LoadGeometryFile(path);

        // add to recent list
        m_RecentUserChoices.push_back(std::move(path));

        // reset search (for next popup open)
        m_Search.clear();

        request_close();

        return rv;
    }

    std::unique_ptr<OpenSim::Mesh> tryDrawFileChoice(const std::filesystem::path& p)
    {
        if (p.filename().string().find(m_Search) != std::string::npos)
        {
            if (ui::draw_selectable(p.filename().string()))
            {
                return onMeshFileChosen(p.filename());
            }
        }
        return nullptr;
    }

    // holding space for result
    std::unique_ptr<OpenSim::Geometry> m_Result;

    // callback that's called with the geometry
    std::function<void(std::unique_ptr<OpenSim::Geometry>)> m_OnSelection;

    // geometry files found in the user's/installation's `Geometry/` dir
    std::vector<std::filesystem::path> m_GeometryFiles;

    // recent file choices by the user
    std::vector<std::filesystem::path> m_RecentUserChoices;

    // the user's current search filter
    std::string m_Search;
};


osc::SelectGeometryPopup::SelectGeometryPopup(
    std::string_view popupName,
    const std::filesystem::path& geometryDir,
    std::function<void(std::unique_ptr<OpenSim::Geometry>)> onSelection) :

    m_Impl{std::make_unique<Impl>(popupName, geometryDir, std::move(onSelection))}
{}
osc::SelectGeometryPopup::SelectGeometryPopup(SelectGeometryPopup&&) noexcept = default;
osc::SelectGeometryPopup& osc::SelectGeometryPopup::operator=(SelectGeometryPopup&&) noexcept = default;
osc::SelectGeometryPopup::~SelectGeometryPopup() noexcept = default;

bool osc::SelectGeometryPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::SelectGeometryPopup::impl_open()
{
    m_Impl->open();
}

void osc::SelectGeometryPopup::impl_close()
{
    m_Impl->close();
}

bool osc::SelectGeometryPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::SelectGeometryPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::SelectGeometryPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
