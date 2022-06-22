#include "SelectGeometryPopup.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <optional>
#include <vector>
#include <utility>

namespace {
    using Geom_ctor_fn = std::unique_ptr<OpenSim::Geometry>(*)(void);
    constexpr std::array<Geom_ctor_fn const, 7> g_GeomCtors = {
        []() {
            auto ptr = std::make_unique<OpenSim::Brick>();
            ptr->set_half_lengths(SimTK::Vec3{0.1, 0.1, 0.1});
            return std::unique_ptr<OpenSim::Geometry>(std::move(ptr));
        },
        []() {
            auto ptr = std::make_unique<OpenSim::Sphere>();
            ptr->set_radius(0.1);
            return std::unique_ptr<OpenSim::Geometry>{std::move(ptr)};
        },
        []() {
            auto ptr = std::make_unique<OpenSim::Cylinder>();
            ptr->set_radius(0.1);
            ptr->set_half_height(0.1);
            return std::unique_ptr<OpenSim::Geometry>{std::move(ptr)};
        },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::LineGeometry{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Ellipsoid{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Arrow{}}; },
        []() { return std::unique_ptr<OpenSim::Geometry>{new OpenSim::Cone{}}; },
    };
    constexpr std::array<char const* const, 7> g_GeomNames = {
        "Brick",
        "Sphere",
        "Cylinder",
        "LineGeometry",
        "Ellipsoid",
        "Arrow (CARE: may not work in OpenSim's main UI)",
        "Cone",
    };
    static_assert(g_GeomCtors.size() == g_GeomNames.size());


    std::optional<std::filesystem::path> promptOpenVTP()
    {
        std::filesystem::path p = osc::PromptUserForFile("vtp,stl");
        return !p.empty() ? std::optional{p.string()} : std::nullopt;
    }
}

class osc::SelectGeometryPopup::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName) :
        StandardPopup{std::move(popupName)}
    {
    }

    std::unique_ptr<OpenSim::Geometry> drawCheckResult()
    {
        draw();
        return std::exchange(m_Result, nullptr);
    }

    void implDraw() override
    {
        // premade geometry section
        //
        // let user select from a shorter sequence of analytical geometry that can be
        // generated without a mesh file
        {
            ImGui::TextUnformatted("Generated geometry");
            ImGui::SameLine();
            DrawHelpMarker("This is geometry that OpenSim can generate without needing an external mesh file. Useful for basic geometry.");
            ImGui::Separator();
            ImGui::Dummy({0.0f, 2.0f});

            int item = -1;
            if (ImGui::Combo("##premade", &item, g_GeomNames.data(), static_cast<int>(g_GeomNames.size())))
            {
                auto const& ctor = g_GeomCtors[static_cast<size_t>(item)];
                m_Result = ctor();
                m_Search.clear();
                requestClose();
            }
        }

        // mesh file selection
        //
        // let the user select a mesh file that the implementation should load + use
        ImGui::Dummy({0.0f, 3.0f});
        ImGui::TextUnformatted("mesh file");
        ImGui::SameLine();
        DrawHelpMarker("This is geometry that OpenSim loads from external mesh files. Useful for custom geometry (usually, created in some other application, such as ParaView or Blender)");
        ImGui::Separator();
        ImGui::Dummy({0.0f, 2.0f});

        // let the user search through mesh files in pre-established Geometry/ dirs
        osc::InputString("search", m_Search, m_SearchMaxLen);
        ImGui::Dummy({0.0f, 1.0f});

        ImGui::BeginChild(
            "mesh list",
            ImVec2(ImGui::GetContentRegionAvail().x, 256),
            false,
            ImGuiWindowFlags_HorizontalScrollbar);

        if (!m_RecentUserChoices.empty())
        {
            ImGui::TextDisabled("  (recent)");
        }

        for (std::filesystem::path const& p : m_RecentUserChoices)
        {
            auto resp = tryDrawFileChoice(p);
            if (resp)
            {
                m_Result = std::move(resp);
            }
        }

        if (!m_RecentUserChoices.empty())
        {
            ImGui::TextDisabled("  (from Geometry/ dir)");
        }
        for (std::filesystem::path const& p : m_Vtps)
        {
            auto resp = tryDrawFileChoice(p);
            if (resp)
            {
                m_Result = std::move(resp);
            }
        }

        ImGui::EndChild();

        if (ImGui::Button("Open Mesh File"))
        {
            if (auto maybeVTP = promptOpenVTP(); maybeVTP)
            {
                m_Result = onVTPChoiceMade(std::move(maybeVTP).value());
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Open a mesh file on the filesystem");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::Dummy({0.0f, 5.0f});

        if (ImGui::Button("Cancel"))
        {
            m_Search.clear();
            requestClose();
        }
    }

private:
    std::unique_ptr<OpenSim::Mesh> onVTPChoiceMade(std::filesystem::path path)
    {
        auto rv = std::make_unique<OpenSim::Mesh>(path.string());

        // add to recent list
        m_RecentUserChoices.push_back(std::move(path));

        // reset search (for next popup open)
        m_Search.clear();

        requestClose();

        return rv;
    }

    std::unique_ptr<OpenSim::Mesh> tryDrawFileChoice(std::filesystem::path const& p)
    {
        if (p.filename().string().find(m_Search) != std::string::npos)
        {
            if (ImGui::Selectable(p.filename().string().c_str()))
            {
                return onVTPChoiceMade(p.filename());
            }
        }
        return nullptr;
    }

    // vtps found in the user's/installation's `Geometry/` dir
    std::vector<std::filesystem::path> m_Vtps = GetAllFilesInDirRecursively(App::resource("geometry"));

    // recent file choices by the user
    std::vector<std::filesystem::path> m_RecentUserChoices;

    // the user's current search filter
    std::string m_Search;

    // maximum length of the search string
    static inline constexpr int m_SearchMaxLen = 128;

    // return value (what the user wants)
    std::unique_ptr<OpenSim::Geometry> m_Result = nullptr;
};

// public API

osc::SelectGeometryPopup::SelectGeometryPopup(std::string_view popupName) :
    m_Impl{std::make_unique<Impl>(std::move(popupName))}
{
}
osc::SelectGeometryPopup::SelectGeometryPopup(SelectGeometryPopup&&) noexcept = default;
osc::SelectGeometryPopup& osc::SelectGeometryPopup::operator=(SelectGeometryPopup&&) noexcept = default;
osc::SelectGeometryPopup::~SelectGeometryPopup() noexcept = default;

void osc::SelectGeometryPopup::open()
{
    m_Impl->open();
}

void osc::SelectGeometryPopup::close()
{
    m_Impl->close();
}

std::unique_ptr<OpenSim::Geometry> osc::SelectGeometryPopup::draw()
{
    return m_Impl->drawCheckResult();
}
