#include "ModelWarpingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TPS3D.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Assertions.hpp"

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
#include <utility>
#include <vector>

// add less-than to OpenSim::ComponentPath
namespace OpenSim
{
    // this lets OpenSim::ComponentPath work in a std::map
    bool operator<(OpenSim::ComponentPath const& a, OpenSim::ComponentPath const& b)
    {
        return a.toString() < b.toString();
    }
}

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
        MeshTPSData rv{mesh.getAbsolutePath()};

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

    // wrapper class over a fully-initialized, immutable, OpenSim model
    //
    // (this editor doesn't allow model edits)
    class ImmutableInitializedModel final {
    public:
        ImmutableInitializedModel() :
            m_Model{}
        {
            osc::InitializeModel(m_Model);
            osc::InitializeState(m_Model);
        }

        ImmutableInitializedModel(std::filesystem::path const& osimPath) :
            m_Model{osimPath.string()}
        {
            osc::InitializeModel(m_Model);
            osc::InitializeState(m_Model);
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model;
        }

    private:
        OpenSim::Model m_Model;
    };

    // top-level document class that represents the model being warped
    class ModelWarpingDocument final {
    public:
        ModelWarpingDocument() :
            m_Model{},
            m_WarpingData{FindLandmarkDataForAllMeshesIn(m_Model.getModel())}
        {
        }

        ModelWarpingDocument(std::filesystem::path const& osimPath) :
            m_Model{osimPath},
            m_WarpingData{FindLandmarkDataForAllMeshesIn(m_Model.getModel())}
        {
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model.getModel();
        }

    private:
        ImmutableInitializedModel m_Model;
        std::map<OpenSim::ComponentPath, MeshTPSData> m_WarpingData;
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

class osc::ModelWarpingTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {
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

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " ModelWarping";
    TabHost* m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<ModelWarpingTabState> m_State = std::make_shared<ModelWarpingTabState>();
};


// public API (PIMPL)

osc::ModelWarpingTab::ModelWarpingTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
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

osc::TabHost* osc::ModelWarpingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ModelWarpingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ModelWarpingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ModelWarpingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ModelWarpingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ModelWarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ModelWarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}
