#include "ModelWarpingTab.hpp"

#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/TPS3D.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Assertions.hpp>

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// OpenSim extension methods
namespace OpenSim
{
    // this lets OpenSim::ComponentPath work in a std::map
    bool operator<(OpenSim::ComponentPath const& a, OpenSim::ComponentPath const& b)
    {
        return a.toString() < b.toString();
    }
}

// document-level code
//
// i.e. code that the user is loading/editing in the UI
namespace
{
    char const* const c_LandmarksFileExtension = ".landmarks";

    // in-memory representation of a loaded ".landmarks" file
    struct MeshLandmarksFile final {
        std::filesystem::path filesystemLocation;
        std::vector<glm::vec3> landmarks;
    };

    // TPS-related data that can be associated to a mesh in the model
    struct MeshTPSData final {

        explicit MeshTPSData(OpenSim::ComponentPath meshComponentAbsPath_) :
            meshComponentAbsPath{std::move(meshComponentAbsPath_)}
        {
        }

        OpenSim::ComponentPath meshComponentAbsPath;
        std::optional<std::filesystem::path> maybeSourceMeshFilesystemLocation;
        std::optional<MeshLandmarksFile> maybeSourceMeshLandmarksFile;
        std::optional<std::filesystem::path> maybeDestinationMeshFilesystemLocation;
        std::optional<MeshLandmarksFile> maybeDestinationMeshLandmarksFile;
    };

    // returns the absolute filesystem path to the TPS "destination" mesh
    //
    // (otherwise, std::nullopt if the associated TPS mesh cannot be found)
    std::optional<std::filesystem::path> FindTPSMeshAbsFilePath(OpenSim::Model const& model, OpenSim::Mesh const& mesh)
    {
        OSC_THROWING_ASSERT(osc::HasInputFileName(model) && "the model isn't available on-disk (required to locate TPS warps)");

        std::filesystem::path const meshFileName = std::filesystem::path{mesh.get_mesh_file()}.filename();
        std::filesystem::path const modelAbsPath = std::filesystem::absolute({model.getInputFileName()});
        std::filesystem::path const expectedTPSMeshPath = modelAbsPath / "TPS" / "Geometry" / meshFileName;

        return std::filesystem::exists(expectedTPSMeshPath) ? std::optional<std::filesystem::path>{expectedTPSMeshPath} : std::nullopt;
    }

    // returns the supplied path, but with the extension replaced by the provided string
    std::filesystem::path WithExtension(std::filesystem::path const& p, std::filesystem::path const& newExtension)
    {
        std::filesystem::path rv = p;
        rv.replace_extension(newExtension);
        return rv;
    }

    // tries to find+load the `.landmarks` file associated with the given mesh path
    std::optional<MeshLandmarksFile> TryLoadMeshLandmarks(std::filesystem::path const& meshAbsPath)
    {
        std::filesystem::path const landmarksPath = WithExtension(meshAbsPath, c_LandmarksFileExtension);

        if (!std::filesystem::exists(landmarksPath))
        {
            return std::nullopt;  // the .landmarks file doesn't exist
        }

        // else: load the landmarks
        return MeshLandmarksFile{landmarksPath,  osc::LoadLandmarksFromCSVFile(landmarksPath)};
    }

    // returns TPS data, if any, associated with the given in-model mesh
    MeshTPSData FindLandmarkData(
        OpenSim::Model const& model,
        OpenSim::Mesh const& mesh)
    {
        MeshTPSData rv{osc::GetAbsolutePath(mesh)};

        // try locating "source" mesh information
        rv.maybeSourceMeshFilesystemLocation = osc::FindGeometryFileAbsPath(model, mesh);
        rv.maybeSourceMeshLandmarksFile = rv.maybeSourceMeshFilesystemLocation ? TryLoadMeshLandmarks(*rv.maybeSourceMeshFilesystemLocation) : std::nullopt;

        // try locating "destination" mesh information
        rv.maybeDestinationMeshFilesystemLocation = FindTPSMeshAbsFilePath(model, mesh);
        rv.maybeDestinationMeshLandmarksFile = rv.maybeDestinationMeshFilesystemLocation ? TryLoadMeshLandmarks(*rv.maybeDestinationMeshFilesystemLocation) : std::nullopt;

        return rv;
    }

    // returns a mapping of mesh.getAbsolutePath() => TPS mesh data for all meshes in the given model
    std::map<OpenSim::ComponentPath, MeshTPSData> FindLandmarkDataForAllMeshesIn(OpenSim::Model const& model)
    {
        std::map<OpenSim::ComponentPath, MeshTPSData> rv;
        for (OpenSim::Mesh const& mesh : model.getComponentList<OpenSim::Mesh>())
        {
            MeshTPSData data = FindLandmarkData(model, mesh);
            OpenSim::ComponentPath absPath = data.meshComponentAbsPath;
            rv.insert_or_assign(std::move(absPath), std::move(data));
        }
        return rv;
    }

    // a single "warp target" in the model
    //
    // this is something in the model that needs to be warped by the TPS algorithm, along with
    // its (assumed) mesh assignment
    struct ModelWarpTarget final {

        ModelWarpTarget(
            OpenSim::ComponentPath componentAbsPath_,
            std::optional<OpenSim::ComponentPath> maybeConnectedMeshWarpPath_ = std::nullopt) :
            componentAbsPath{std::move(componentAbsPath_)},
            maybeConnectedMeshWarpPath{std::move(maybeConnectedMeshWarpPath_)}
        {
        }

        OpenSim::ComponentPath componentAbsPath;
        std::optional<OpenSim::ComponentPath> maybeConnectedMeshWarpPath;
    };

    // returns all warp targets (+assumed assignments) in the model
    std::map<OpenSim::ComponentPath, ModelWarpTarget> FindAllWarpTargetsIn(OpenSim::Model const& model)
    {
        std::map<OpenSim::ComponentPath, ModelWarpTarget> rv;
        for (OpenSim::PhysicalFrame const& frame : model.getComponentList<OpenSim::PhysicalFrame>())
        {
            OpenSim::ComponentPath const absPath = osc::GetAbsolutePath(frame);
            rv.insert_or_assign(absPath, ModelWarpTarget{absPath, std::nullopt});
        }
        return rv;
    }

    // wrapper class over a fully-initialized, immutable, OpenSim model
    //
    // (this editor doesn't allow model edits)
    class ImmutableInitializedModel final {
    public:
        ImmutableInitializedModel() :
            m_Model{std::make_unique<OpenSim::Model>()}
        {
            osc::InitializeModel(*m_Model);
            osc::InitializeState(*m_Model);
        }

        ImmutableInitializedModel(std::filesystem::path const& osimPath) :
            m_Model{std::make_unique<OpenSim::Model>(osimPath.string())}
        {
            osc::InitializeModel(*m_Model);
            osc::InitializeState(*m_Model);
        }

        OpenSim::Model const& getModel() const
        {
            return *m_Model;
        }

    private:
        std::unique_ptr<OpenSim::Model> m_Model;
    };

    // top-level document class that represents the model being warped
    class ModelWarpingDocument final {
    public:
        ModelWarpingDocument() = default;

        ModelWarpingDocument(std::filesystem::path const& osimPath) :
            m_Model{osimPath}
        {
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model.getModel();
        }

        std::map<OpenSim::ComponentPath, MeshTPSData> const& getWarpingData() const
        {
            return m_WarpingData;
        }

        std::map<OpenSim::ComponentPath, ModelWarpTarget> const& getWarpTargetData() const
        {
            return m_WarpTargets;
        }

    private:
        ImmutableInitializedModel m_Model;
        std::map<OpenSim::ComponentPath, MeshTPSData> m_WarpingData = FindLandmarkDataForAllMeshesIn(m_Model.getModel());
        std::map<OpenSim::ComponentPath, ModelWarpTarget> m_WarpTargets = FindAllWarpTargetsIn(m_Model.getModel());
    };

    // top-level state for the whole tab UI
    struct ModelWarpingTabState final {
        ModelWarpingDocument document;
    };

    // action: prompt the user for an osim file to open
    void ActionOpenOsim(ModelWarpingTabState& state)
    {
        std::optional<std::filesystem::path> const maybeOsimPath = osc::PromptUserForFile("osim");

        if (!maybeOsimPath)
        {
            return;  // user probably cancelled out of the prompt
        }

        state.document = ModelWarpingDocument{*maybeOsimPath};
    }
}

// ui code
//
// i.e. code that's relevant for rendering panels, inputs, etc.
namespace
{
    // draws the main-menu's `file` menu
    class ModelWarpingTabFileMenu final {
    public:
        explicit ModelWarpingTabFileMenu(std::shared_ptr<ModelWarpingTabState> state_) :
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr);
        }

        void draw()
        {
            if (ImGui::BeginMenu("File"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }
    private:
        void drawContent()
        {
            if (ImGui::MenuItem(ICON_FA_FILE_IMPORT " Import .osim"))
            {
                ActionOpenOsim(*m_State);
            }
        }

        std::shared_ptr<ModelWarpingTabState> m_State;
    };

    // draws all items in the main menu
    class ModelWarpingTabMainMenu final {
    public:
        explicit ModelWarpingTabMainMenu(std::shared_ptr<ModelWarpingTabState> state_) :
            m_FileMenu{std::move(state_)}
        {
        }

        void draw()
        {
            m_FileMenu.draw();
            m_AboutMenu.draw();
        }

    private:
        ModelWarpingTabFileMenu m_FileMenu;
        osc::MainMenuAboutTab m_AboutMenu;
    };

    class ModelWarpingDocumentDebuggerPanel final : public osc::StandardPanel {
    public:
        ModelWarpingDocumentDebuggerPanel(
            std::string_view panelName_,
            std::shared_ptr<ModelWarpingTabState> state_) :

            StandardPanel{std::move(panelName_)},
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr);
        }

    private:
        void implDrawContent() final
        {
            drawButtons();
            drawModelInfoSection();
            drawWarpingInfoSection();
            drawWarpTargetSection();
        }

        void drawButtons()
        {
            if (ImGui::Button(ICON_FA_FILE_IMPORT " Import .osim"))
            {
                ActionOpenOsim(*m_State);
            }
        }

        void drawModelInfoSection() const
        {
            ModelWarpingDocument const& doc = m_State->document;

            ImGui::NewLine();
            ImGui::Text("Model Info");
            ImGui::SameLine();
            osc::DrawHelpMarker("Top-level information extracted from the osim itself");
            ImGui::Separator();

            if (osc::HasInputFileName(doc.getModel()))
            {
                ImGui::Text("    file = %s", osc::TryFindInputFile(doc.getModel()).filename().string().c_str());
            }
            else
            {
                ImGui::Text("    file = (no backing file)");
            }
        }

        void drawWarpingInfoSection() const
        {
            ImGui::NewLine();
            ImGui::Text("Mesh Warps Info");
            ImGui::SameLine();
            osc::DrawHelpMarker("Warping information associated to each mesh in the model");
            ImGui::Separator();

            if (!m_State->document.getWarpingData().empty())
            {
                drawWarpingInfoTable();
            }
            else
            {
                ImGui::TextDisabled("    (no mesh warping information available)");
            }
        }

        void drawWarpingInfoTable() const
        {
            if (ImGui::BeginTable("##WarpingInfo", 5))
            {
                ImGui::TableSetupColumn("Component Name");
                ImGui::TableSetupColumn("Source Mesh File");
                ImGui::TableSetupColumn("Source Mesh Landmarks");
                ImGui::TableSetupColumn("Destination Mesh File");
                ImGui::TableSetupColumn("Destination Mesh Landmarks");
                ImGui::TableHeadersRow();

                for (std::pair<OpenSim::ComponentPath const, MeshTPSData> const& p : m_State->document.getWarpingData())
                {
                    ImGui::TableNextRow();
                    drawWarpingInfoTableRowContent(p);
                }

                ImGui::EndTable();
            }
        }

        void drawWarpingInfoTableRowContent(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            ImGui::TableSetColumnIndex(0);
            drawComponentNameCell(p);
            ImGui::TableSetColumnIndex(1);
            drawSourceMeshCell(p);
            ImGui::TableSetColumnIndex(2);
            drawSourceLandmarksCell(p);
            ImGui::TableSetColumnIndex(3);
            drawDestinationMeshCell(p);
            ImGui::TableSetColumnIndex(4);
            drawDestinationLandmarksCell(p);
        }

        void drawComponentNameCell(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            std::string const name = p.first.getComponentName();
            ImGui::Text("%s", name.c_str());
            osc::DrawTooltipIfItemHovered(name, p.first.toString());  // show abspath on hover
        }

        void drawSourceMeshCell(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            std::optional<std::filesystem::path> const& maybeMeshLocation = p.second.maybeSourceMeshFilesystemLocation;
            if (!maybeMeshLocation)
            {
                drawMissingMessage();
                return;
            }
            std::filesystem::path const& meshLocation = *maybeMeshLocation;

            std::string const filename = meshLocation.filename().string();
            ImGui::Text("%s", filename.c_str());
            osc::DrawTooltipIfItemHovered(filename, meshLocation.string());
        }

        void drawSourceLandmarksCell(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            std::optional<MeshLandmarksFile> const& maybeLocation = p.second.maybeSourceMeshLandmarksFile;
            if (!maybeLocation)
            {
                drawMissingMessage();
                return;
            }
            MeshLandmarksFile const& location = *maybeLocation;

            ImGui::TextUnformatted("source landmarks exist");
        }

        void drawDestinationMeshCell(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            std::optional<std::filesystem::path> const& maybeLocation = p.second.maybeDestinationMeshFilesystemLocation;
            if (!maybeLocation)
            {
                drawMissingMessage();
                return;
            }
            std::filesystem::path const& location = *maybeLocation;

            ImGui::TextUnformatted("destination mesh exists");
        }

        void drawDestinationLandmarksCell(std::pair<OpenSim::ComponentPath, MeshTPSData> const& p) const
        {
            std::optional<MeshLandmarksFile> const& maybeLocation = p.second.maybeDestinationMeshLandmarksFile;
            if (!maybeLocation)
            {
                drawMissingMessage();
                return;
            }
            MeshLandmarksFile const& location = *maybeLocation;

            ImGui::TextUnformatted("destination landmarks exist");
        }

        void drawMissingMessage() const
        {
            ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.0f, 0.0f, 1.0f});
            ImGui::TextUnformatted("(missing)");
            ImGui::PopStyleColor();
        }

        void drawWarpTargetSection() const
        {
            ImGui::NewLine();
            ImGui::TextUnformatted("Warp Target Info");
            ImGui::SameLine();
            osc::DrawHelpMarker("How each mesh warp is applied (or not) to applicable components in the model");
            ImGui::Separator();

            if (!m_State->document.getWarpTargetData().empty())
            {
                drawWarpTargetTable();
            }
            else
            {
                ImGui::TextDisabled("    (no warp target information available)");
            }
        }

        void drawWarpTargetTable() const
        {
            if (ImGui::BeginTable("##WarpTargetInfo", 2))
            {
                ImGui::TableSetupColumn("Component Name");
                ImGui::TableSetupColumn("Connected Mesh Warp");
                ImGui::TableHeadersRow();

                for (std::pair<OpenSim::ComponentPath const, ModelWarpTarget> const& p : m_State->document.getWarpTargetData())
                {
                    ImGui::TableNextRow();
                    drawWarpTargetTableRowContent(p);
                }

                ImGui::EndTable();
            }
        }

        void drawWarpTargetTableRowContent(std::pair<OpenSim::ComponentPath, ModelWarpTarget> const& p) const
        {
            ImGui::TableSetColumnIndex(0);
            drawWarpTargetComponentNameCell(p);
            ImGui::TableSetColumnIndex(1);
            drawWarpTargetConnectedMesh(p);
        }

        void drawWarpTargetComponentNameCell(std::pair<OpenSim::ComponentPath, ModelWarpTarget> const& p) const
        {
            std::string const name = p.first.getComponentName();
            ImGui::Text("%s", name.c_str());
            osc::DrawTooltipIfItemHovered(name, p.first.toString());  // show abspath on hover
        }

        void drawWarpTargetConnectedMesh(std::pair<OpenSim::ComponentPath, ModelWarpTarget> const& p) const
        {
            if (!p.second.maybeConnectedMeshWarpPath)
            {
                drawMissingMessage();
                return;
            }
            OpenSim::ComponentPath const& path = *p.second.maybeConnectedMeshWarpPath;
        }

        std::shared_ptr<ModelWarpingTabState> m_State;
    };
}

class osc::ModelWarpingTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " ModelWarping";
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        // set the size+pos (central) of the main menu
        {
            Rect const mainMenuRect = calcMenuRect();
            glm::vec2 const mainMenuDims = Dimensions(mainMenuRect);
            ImGui::SetNextWindowPos(mainMenuRect.p1);
            ImGui::SetNextWindowSize({mainMenuDims.x, -1.0f});
            ImGui::SetNextWindowSizeConstraints(mainMenuDims, mainMenuDims);
        }

        if (ImGui::Begin("Input Screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            drawMenuContent();
        }
        ImGui::End();

        m_DebuggerPanel.draw();
    }

    void drawMenuContent()
    {
        ImGui::Text("hi");
    }

    Rect calcMenuRect()
    {
        glm::vec2 constexpr menuMaxDims = {640.0f, 512.0f};

        Rect const tabRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 const menuDims = osc::Min(osc::Dimensions(tabRect), menuMaxDims);
        glm::vec2 const menuTopLeft = tabRect.p1 + 0.5f*(Dimensions(tabRect) - menuDims);

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

private:
    UID m_TabID;

    // top-level state that all panels can potentially access
    std::shared_ptr<ModelWarpingTabState> m_State = std::make_shared<ModelWarpingTabState>();

    // UI widgets etc.
    ModelWarpingTabMainMenu m_MainMenu{m_State};
    ModelWarpingDocumentDebuggerPanel m_DebuggerPanel{"Debugger", m_State};
};


// public API (PIMPL)

osc::CStringView osc::ModelWarpingTab::id() noexcept
{
    return "OpenSim/Experimental/ModelWarping";
}

osc::ModelWarpingTab::ModelWarpingTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ModelWarpingTab::~ModelWarpingTab() noexcept = default;

osc::UID osc::ModelWarpingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ModelWarpingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ModelWarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ModelWarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}
