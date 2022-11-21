#include "ModelWarpingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_events.h>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // one set of source/destination landmarks for a particular mesh in the model
    struct MeshTPS3DLandmarks final {
        OpenSim::ComponentPath meshComponentAbsPath;
        std::filesystem::path meshPath;
        std::filesystem::path landmarksPath;
        std::vector<glm::vec3> landmarkPositions;
    };

    std::filesystem::path GetMeshAbsFilePath(OpenSim::Mesh const& mesh)
    {
        return {};  // TODO
    }

    std::filesystem::path GetPathToAssociatedLandmarksFile(std::filesystem::path const& meshFilePath)
    {
        std::filesystem::path rv = meshFilePath;
        rv.replace_extension(".landmarks");
        return rv;
    }

    std::vector<glm::vec3> LoadLandmarkPositions(std::filesystem::path const& landmarksFilePath)
    {
        return {};  // TODO
    }

    // returns filename(no extension) => source mesh+landmark associations for a given model
    //
    // assumes that for each landmark-ed mesh in the model, there exists a file called
    // `mesh.landmarks` "ajacent" to it in the filesystem
    std::map<std::string, MeshTPS3DLandmarks> FindSourceLandmarkData(OpenSim::Model const& model)
    {
        for (OpenSim::Mesh const& mesh : model.getComponentList<OpenSim::Mesh>())
        {
            OpenSim::ComponentPath const componentAbsPath = mesh.getAbsolutePath();
            std::filesystem::path const meshPath = GetMeshAbsFilePath(mesh);
            std::filesystem::path const landmarksPath = GetPathToAssociatedLandmarksFile(meshPath);
            if (!std::filesystem::exists(landmarksPath))
            {
                continue;  // skip this: it doesn't have an associated landmarks file
            }

            std::vector<glm::vec3> const landmarksData = LoadLandmarkPositions(landmarksPath);
        }
        return {};
    }

    // returns filename(no extension) => destination mesh+landmark associations for a given model
    //
    // assumes that for each landmark-ed mesh in the model, there exists files called
    // `mesh.meshextension` and `mesh.landmarks` in a folder called `destination/` in the
    // filesystem
    std::map<std::string, MeshTPS3DLandmarks> FindDestinationLandmarkData(OpenSim::Model const&)
    {
        return {};
    }

    // model warping input to the TPS algorithm
    class ModelWarpingInput final {
    public:
        ModelWarpingInput() :
            m_Model{},
            m_SourceLandmarkData{},
            m_DestinationLandmarkData{}
        {
        }

        explicit ModelWarpingInput(std::filesystem::path const& osimPath) :
            m_Model{osimPath},
            m_SourceLandmarkData{FindSourceLandmarkData(m_Model.getModel())},
            m_DestinationLandmarkData{FindDestinationLandmarkData(m_Model.getModel())}
        {
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model.getModel();
        }

        size_t getNumSourceLandmarks() const
        {
            return m_SourceLandmarkData.size();
        }

        MeshTPS3DLandmarks const& getSourceLandmark(size_t i) const
        {
            auto it = m_SourceLandmarkData.cbegin();
            std::advance(it, i);
            return it->second;
        }

        size_t getNumDestinationLandmarks() const
        {
            return m_DestinationLandmarkData.size();
        }

        MeshTPS3DLandmarks const& getDestinationLandmark(size_t i) const
        {
            auto it = m_DestinationLandmarkData.cbegin();
            std::advance(it, i);
            return it->second;
        }

    private:
        osc::UndoableModelStatePair m_Model;
        std::map<std::string, MeshTPS3DLandmarks> m_SourceLandmarkData;
        std::map<std::string, MeshTPS3DLandmarks> m_DestinationLandmarkData;
    };

    // top-level state for the whole tab UI
    struct ModelWarpingTabState final {
        ModelWarpingInput warpingInput;
    };
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

    // tab stuff
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " ModelWarping";
    TabHost* m_Parent;

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
