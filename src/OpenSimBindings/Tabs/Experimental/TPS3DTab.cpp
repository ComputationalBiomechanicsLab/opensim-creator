#include "TPS3DTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Formats/CSV.hpp"
#include "src/Formats/OBJ.hpp"
#include "src/Formats/STL.hpp"
#include "src/Graphics/CachedSceneRenderer.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/CollisionTests.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/TPS3D.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/UndoRedo.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/RedoButton.hpp"
#include "src/Widgets/StandardPanel.hpp"
#include "src/Widgets/UndoButton.hpp"
#include "src/Widgets/UndoRedoPanel.hpp"
#include "src/Widgets/Panel.hpp"

#include <glm/mat3x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>
#include <Simbody.h>

#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <string>
#include <string_view>
#include <sstream>
#include <iostream>
#include <limits>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// generic graphics algorithms
//
// (these have nothing to do with TPS, but are used to help render the UI)
namespace
{
    // returns the 3D position of the intersection between the user's mouse and the mesh, if any
    std::optional<osc::RayCollision> RaycastMesh(
        osc::PolarPerspectiveCamera const& camera,
        osc::Mesh const& mesh,
        osc::Rect const& renderRect,
        glm::vec2 mousePos)
    {
        osc::Line const ray = camera.unprojectTopLeftPosToWorldRay(
            mousePos - renderRect.p1,
            osc::Dimensions(renderRect)
        );

        return osc::GetClosestWorldspaceRayCollision(
            mesh,
            osc::Transform{},
            ray
        );
    }

    // returns scene rendering parameters for an generic panel
    osc::SceneRendererParams CalcRenderParams(
        osc::PolarPerspectiveCamera const& camera,
        glm::vec2 renderDims)
    {
        osc::SceneRendererParams rv;
        rv.drawFloor = false;
        rv.backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        rv.dimensions = renderDims;
        rv.viewMatrix = camera.getViewMtx();
        rv.projectionMatrix = camera.getProjMtx(osc::AspectRatio(renderDims));
        rv.samples = osc::App::get().getMSXAASamplesRecommended();
        rv.lightDirection = osc::RecommendedLightDirection(camera);
        return rv;
    }
}

// TPS document datastructure
//
// this covers the datastructures that the user is dynamically editing
namespace
{
    // the "input" part of the document (i.e. source or destination mesh)
    struct TPSDocumentInput final {

        explicit TPSDocumentInput(osc::Mesh const& mesh_) :
            Mesh{mesh_}
        {
        }

        osc::Mesh Mesh;
        std::unordered_map<std::string, glm::vec3> Landmarks;
    };

    // the whole TPS document that the user edits
    struct TPSDocument final {
        TPSDocumentInput Source{osc::GenUntexturedUVSphere(16, 16)};
        TPSDocumentInput Destination{osc::GenUntexturedSimbodyCylinder(16)};
        float BlendingFactor = 1.0f;
    };

    // an enum used to identify one of the two inputs (source/destination) of the TPS
    // document at runtime
    enum class TPSDocumentInputIdentifier {
        Source,
        Destination,
    };

    // an enum used to identify what type of part of the input is described
    enum class TPSDocumentInputElementType {
        Landmark,
        Mesh,
    };

    // an associative identifier to specific element in a TPS document
    //
    // (handy for selection logic etc.)
    struct TPSDocumentElementID final {
        TPSDocumentElementID(
            TPSDocumentInputIdentifier whichInput_,
            TPSDocumentInputElementType elementType_,
            std::string elementID_) :

            whichInput{std::move(whichInput_)},
            elementType{std::move(elementType_)},
            elementID{std::move(elementID_)}
        {
        }

        TPSDocumentInputIdentifier whichInput;
        TPSDocumentInputElementType elementType;
        std::string elementID;
    };

    // (necessary for storage in associative datastructures)
    bool operator==(TPSDocumentElementID const& a, TPSDocumentElementID const& b)
    {
        return
            a.whichInput == b.whichInput &&
            a.elementType == b.elementType &&
            a.elementID == b.elementID;
    }
}

namespace std
{
    // (necessary for storage in associative datastructures)
    template<>
    struct hash<TPSDocumentElementID> {
        size_t operator()(TPSDocumentElementID const& el) const
        {
            return osc::HashOf(el.whichInput, el.elementType, el.elementID);
        }
    };
}

namespace
{
    // returns the specified input
    TPSDocumentInput const& GetInput(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        switch (which)
        {
        case TPSDocumentInputIdentifier::Source:
            return doc.Source;
        case TPSDocumentInputIdentifier::Destination:
            return doc.Destination;
        default:
            throw std::runtime_error{"invalid document identifier provided to a getter"};
        }
    }

    // returns the specified input (mutable)
    TPSDocumentInput& UpdInput(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        switch (which)
        {
        case TPSDocumentInputIdentifier::Source:
            return doc.Source;
        case TPSDocumentInputIdentifier::Destination:
            return doc.Destination;
        default:
            throw std::runtime_error{"invalid document identifier provided to a getter"};
        }
    }

    // returns all paired landmarks in the document argument
    std::vector<osc::LandmarkPair3D> GetLandmarkPairs(TPSDocument const& doc)
    {
        std::vector<osc::LandmarkPair3D> rv;
        rv.reserve(doc.Source.Landmarks.size());  // probably a good guess
        for (auto const& [k, srcPos] : doc.Source.Landmarks)
        {
            auto const it = doc.Destination.Landmarks.find(k);
            if (it != doc.Destination.Landmarks.end())
            {
                rv.emplace_back(srcPos, it->second);
            }
        }
        return rv;
    }

    // action: try to undo the last change
    void ActionUndo(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.undo();
    }

    // action: try to redo the last undone change
    void ActionRedo(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.redo();
    }

    // action: add a landmark to the source mesh and return its ID
    std::string ActionAddLandmarkTo(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentInputIdentifier which,
        glm::vec3 const& pos)
    {
        TPSDocumentInput& input = UpdInput(doc.updScratch(), which);
        std::string const id = std::to_string(input.Landmarks.size());

        input.Landmarks[id] = pos;
        doc.commitScratch("added landmark");

        return id;
    }

    // action: prompt the user to browse for a different source mesh
    void ActionBrowseForNewMesh(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeMeshPath = osc::PromptUserForFile("vtp,obj");
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }

        TPSDocumentInput& input = UpdInput(doc.updScratch(), which);
        input.Mesh = osc::LoadMeshViaSimTK(*maybeMeshPath);
        doc.commitScratch("changed mesh");
    }

    // action load landmarks from a headerless CSV file into source/destination
    void ActionLoadLandmarksCSV(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath = osc::PromptUserForFile("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select anything
        }

        std::vector<glm::vec3> const landmarks = osc::LoadLandmarksFromCSVFile(*maybeCSVPath);

        if (landmarks.empty())
        {
            return;  // file was empty, or had invalid data
        }

        TPSDocumentInput& input = UpdInput(doc.updScratch(), which);
        for (glm::vec3 const& landmark : landmarks)
        {
            std::string const key = std::to_string(input.Landmarks.size());
            input.Landmarks[key] = landmark;
        }
        doc.commitScratch("loaded landmarks");
    }

    // action: set the TPS blending factor for the result, but don't save it to undo/redo storage
    void ActionSetBlendFactor(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        doc.updScratch().BlendingFactor = factor;
    }

    // action: set the TPS blending factor and save a commit of the change
    void ActionSetBlendFactorAndSave(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        ActionSetBlendFactor(doc, factor);
        doc.commitScratch("changed blend factor");
    }

    // action: create a "fresh" TPS document
    void ActionCreateNewDocument(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch() = TPSDocument{};
        doc.commitScratch("created new document");
    }

    // action: clear all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch().Source.Landmarks.clear();
        doc.updScratch().Destination.Landmarks.clear();
        doc.commitScratch("cleared all landmarks");
    }

    // action: delete the specified landmarks
    void ActionDeleteSceneElementsByID(
        osc::UndoRedoT<TPSDocument>& doc,
        std::unordered_set<TPSDocumentElementID> const& elementIDs)
    {
        if (elementIDs.empty())
        {
            return;
        }

        TPSDocument& scratch = doc.updScratch();
        for (TPSDocumentElementID const& id : elementIDs)
        {
            TPSDocumentInput& input = UpdInput(scratch, id.whichInput);
            if (id.elementType == TPSDocumentInputElementType::Landmark)
            {
                input.Landmarks.erase(id.elementID);
            }
        }

        doc.commitScratch("deleted elements");
    }

    // action: save all source/destination landmarks to a simple headerless CSV file (matches loading)
    void ActionSaveLandmarksToCSV(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outfile{*maybeCSVPath};

        if (!outfile)
        {
            return;  // couldn't open file for writing
        }

        osc::CSVWriter writer{outfile};
        std::vector<std::string> cols(3);

        TPSDocumentInput const& input = GetInput(doc, which);
        for (auto const& [k, pos] : input.Landmarks)
        {
            cols.at(0) = std::to_string(pos.x);
            cols.at(1) = std::to_string(pos.y);
            cols.at(2) = std::to_string(pos.z);
            writer.writeRow(cols);
        }
    }

    // action: save all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSaveLandmarksToPairedCSV(TPSDocument const& doc)
    {
        std::vector<osc::LandmarkPair3D> const pairs = GetLandmarkPairs(doc);

        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outfile{*maybeCSVPath};

        if (!outfile)
        {
            return;  // couldn't open file for writing
        }

        osc::CSVWriter writer{outfile};

        std::vector<std::string> cols =
        {
            "source.x",
            "source.y",
            "source.z",
            "dest.x",
            "dest.y",
            "dest.z",
        };

        writer.writeRow(cols);  // write header
        for (osc::LandmarkPair3D const& p : pairs)
        {
            cols.at(0) = std::to_string(p.Src.x);
            cols.at(1) = std::to_string(p.Src.y);
            cols.at(2) = std::to_string(p.Src.z);

            cols.at(0) = std::to_string(p.Dest.x);
            cols.at(1) = std::to_string(p.Dest.y);
            cols.at(2) = std::to_string(p.Dest.z);
            writer.writeRow(cols);
        }
    }

    // action: prompt the user to save the result (transformed) mesh to an obj file
    void ActionTrySaveMeshToObj(osc::Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeOBJFile =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");

        if (!maybeOBJFile)
        {
            return;  // user didn't select a save location
        }

        std::ios_base::openmode const flags =
            std::ios_base::out |
            std::ios_base::trunc;

        std::ofstream outfile{*maybeOBJFile, flags};

        if (!outfile)
        {
            return;  // couldn't open for writing
        }

        osc::ObjWriter writer{outfile};

        // ignore normals, because warping might have screwed them
        writer.write(mesh, osc::ObjWriterFlags_IgnoreNormals);
    }

    // action: prompt the user to save the result (transformed) mesh to an stl file
    void ActionTrySaveMeshToStl(osc::Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeSTLPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");

        if (!maybeSTLPath)
        {
            return;  // user didn't select a save location
        }

        std::ios_base::openmode const flags =
            std::ios_base::binary |
            std::ios_base::out |
            std::ios_base::trunc;

        std::ofstream outfile{*maybeSTLPath, flags};

        if (!outfile)
        {
            return;  // couldn't open for writing
        }

        osc::StlWriter writer{outfile};

        writer.write(mesh);
    }
}

// generic result cache helper class
namespace
{
    // a cache that only recomputes the transformed mesh if the document
    // has changed
    //
    // (e.g. when a user adds a new landmark or changes the blending factor)
    class TPSResultCache final {
    public:

        // lookup, or recompute, the transformed mesh
        osc::Mesh const& lookup(TPSDocument const& doc)
        {
            updateResultMesh(doc);
            return m_CachedResultMesh;
        }

    private:
        // returns `true` if the cached result mesh was updated
        bool updateResultMesh(TPSDocument const& doc)
        {
            bool const updatedCoefficients = updateCoefficients(doc);
            bool const updatedMesh = updateInputMesh(doc);

            if (updatedCoefficients || updatedMesh)
            {
                m_CachedResultMesh = ApplyThinPlateWarpToMesh(m_CachedCoefficients, m_CachedSourceMesh);
                return true;
            }
            else
            {
                return false;
            }
        }

        // returns `true` if cached coefficients were updated
        bool updateCoefficients(TPSDocument const& doc)
        {
            if (!updateInputs(doc))
            {
                // cache: the inputs have not been updated, so the coefficients will not change
                return false;
            }

            osc::TPSCoefficients3D newCoefficients = osc::CalcCoefficients(m_CachedInputs);

            if (newCoefficients != m_CachedCoefficients)
            {
                m_CachedCoefficients = std::move(newCoefficients);
                return true;
            }
            else
            {
                return false;  // no change in the coefficients
            }
        }

        // returns `true` if `m_CachedSourceMesh` is updated
        bool updateInputMesh(TPSDocument const& doc)
        {
            if (m_CachedSourceMesh != doc.Source.Mesh)
            {
                m_CachedSourceMesh = doc.Source.Mesh;
                return true;
            }
            else
            {
                return false;
            }
        }

        // returns `true` if cached inputs were updated; otherwise, returns the cached inputs
        bool updateInputs(TPSDocument const& doc)
        {
            osc::TPSCoefficientSolverInputs3D newInputs
            {
                GetLandmarkPairs(doc),
                doc.BlendingFactor,
            };

            if (newInputs != m_CachedInputs)
            {
                m_CachedInputs = std::move(newInputs);
                return true;
            }
            else
            {
                return false;
            }
        }

        osc::TPSCoefficientSolverInputs3D m_CachedInputs;
        osc::TPSCoefficients3D m_CachedCoefficients;
        osc::Mesh m_CachedSourceMesh;
        osc::Mesh m_CachedResultMesh;
    };
}

// TPS UI code
//
// UI code that is specific to the TPS3D UI
namespace
{
    // (forward decl. for a struct that is injected into all panels in the UI)
    struct TPSTabSharedState;

    // a typedef for a function that can construct a UI panel
    //
    // (i.e. this is what's called when a user turns a panel on)
    using TPSUIPanelConstructor = std::function<std::shared_ptr<osc::Panel>(std::shared_ptr<TPSTabSharedState>)>;

    // a panel that a user can toggle at runtime
    //
    // (can be initially toggled off. In which case, `Instance == nullopt`)
    struct TPSUIPanelInfo final {

        TPSUIPanelInfo(
            std::string_view name_,
            TPSUIPanelConstructor constructor_,
            bool isEnabledByDefault_) :

            Name{std::move(name_)},
            ConstructorFunc{std::move(constructor_)},
            IsEnabledByDefault{std::move(isEnabledByDefault_)},
            Instance{std::nullopt}
        {
        }

        std::string Name;
        TPSUIPanelConstructor ConstructorFunc;
        bool IsEnabledByDefault;
        std::optional<std::shared_ptr<osc::Panel>> Instance;
    };

    // returns all available user-toggleable panels (forward decl.)
    std::vector<TPSUIPanelInfo> GetAvailablePanels();

    // holds information about the user's current mouse hover
    struct TPSTabHover final {

        explicit TPSTabHover(glm::vec3 const& worldspaceLocation_) :
            MaybeSceneElementID{std::nullopt},
            WorldspaceLocation{worldspaceLocation_}
        {
        }

        TPSTabHover(
            TPSDocumentElementID sceneElementID_,
            glm::vec3 const& worldspaceLocation_) :

            MaybeSceneElementID{std::move(sceneElementID_)},
            WorldspaceLocation{worldspaceLocation_}
        {
        }

        std::optional<TPSDocumentElementID> MaybeSceneElementID;
        glm::vec3 WorldspaceLocation;
    };

    // holds information about the user's current selection
    struct TPSTabSelection final {

        void clear()
        {
            m_SelectedSceneElements.clear();
        }

        void select(TPSDocumentElementID el)
        {
            m_SelectedSceneElements.insert(std::move(el));
        }

        bool contains(TPSDocumentElementID const& el) const
        {
            return m_SelectedSceneElements.find(el) != m_SelectedSceneElements.end();
        }

        std::unordered_set<TPSDocumentElementID> const& getUnderlyingSet() const
        {
            return m_SelectedSceneElements;
        }

    private:
        std::unordered_set<TPSDocumentElementID> m_SelectedSceneElements;
    };

    // top-level tab state
    //
    // (shared by all panels)
    struct TPSTabSharedState final {

        explicit TPSTabSharedState(osc::UID tabID_, osc::TabHost* parent_) :
            m_TabID{std::move(tabID_)},
            m_Parent{std::move(parent_)}
        {
        }

        osc::Mesh const& getTransformedMesh()
        {
            return ResultCache.lookup(EditedDocument->getScratch());
        }

        TPSDocumentInput& updInputDocument(TPSDocumentInputIdentifier identifier)
        {
            return UpdInput(EditedDocument->updScratch(), identifier);
        }

        TPSDocumentInput const& getInputDocument(TPSDocumentInputIdentifier identifier) const
        {
            return GetInput(EditedDocument->getScratch(), identifier);
        }

        osc::Mesh const& getInputMesh(TPSDocumentInputIdentifier identifier) const
        {
            return getInputDocument(identifier).Mesh;
        }

        void requestCloseTab()
        {
            m_Parent->closeTab(m_TabID);
        }

        // the document the user is editing
        std::shared_ptr<osc::UndoRedoT<TPSDocument>> EditedDocument = std::make_shared<osc::UndoRedoT<TPSDocument>>();

        // cached TPS3D algorithm result (to prevent recomputing it each frame)
        TPSResultCache ResultCache;

        // `true` if the user wants the cameras to be linked
        bool LinkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool OnlyLinkRotation = false;

        // shared linked camera
        osc::PolarPerspectiveCamera LinkedCameraBase = CreateCameraFocusedOn(EditedDocument->getScratch().Source.Mesh.getBounds());

        // wireframe material, used to draw scene elements in a wireframe style
        osc::Material WireframeMaterial = osc::CreateWireframeOverlayMaterial(osc::App::config(), osc::App::singleton<osc::ShaderCache>());

        // shared sphere mesh (used by rendering code)
        std::shared_ptr<osc::Mesh const> LandmarkSphere = osc::App::singleton<osc::MeshCache>().getSphereMesh();

        // color of any in-scene landmark spheres
        glm::vec4 LandmarkColor = {1.0f, 0.0f, 0.0f, 1.0f};

        // current user selection
        TPSTabSelection UserSelection;

        // current user hover: reset per-frame
        std::optional<TPSTabHover> CurrentHover;

        // available/active panels that the user can toggle via the `window` menu
        std::vector<TPSUIPanelInfo> Panels = GetAvailablePanels();

    private:
        osc::UID m_TabID;
        osc::TabHost* m_Parent;
    };

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSTabSharedState const& sharedState,
        osc::Mesh const& tpsSourceOrDestinationMesh,
        bool wireframeMode,
        std::vector<osc::SceneDecoration>& out)
    {
        out.reserve(out.size() + 5);  // likely guess

        // draw the mesh
        out.emplace_back(tpsSourceOrDestinationMesh);

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            osc::SceneDecoration& dec = out.emplace_back(tpsSourceOrDestinationMesh);
            dec.maybeMaterial = sharedState.WireframeMaterial;
        }

        // add grid decorations
        DrawXZGrid(osc::App::singleton<osc::MeshCache>(), out);
        DrawXZFloorLines(osc::App::singleton<osc::MeshCache>(), out, 100.0f);
    }

    // generic base class for the panels shown in the TPS3D tab
    class TPS3DTabPanel : public osc::StandardPanel {
    public:
        using osc::StandardPanel::StandardPanel;

    private:
        void implBeforeImGuiBegin() override final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void implAfterImGuiBegin() override final
        {
            ImGui::PopStyleVar();
        }
    };

    // an "input" panel (i.e. source or destination mesh, before warping)
    class TPS3DInputPanel final : public TPS3DTabPanel {
    public:
        TPS3DInputPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSTabSharedState> state_,
            TPSDocumentInputIdentifier documentIdentifier_) :

            TPS3DTabPanel{std::move(panelName_), ImGuiDockNodeFlags_PassthruCentralNode},
            m_State{std::move(state_)},
            m_DocumentIdentifier{std::move(documentIdentifier_)}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid sharedState state");
        }

    private:
        // draws all of the panel's content
        void implDrawContent() override
        {
            // compute top-level UI variables (render rect, mouse pos, etc.)
            osc::Rect const contentRect = osc::ContentRegionAvailScreenRect();
            glm::vec2 const contentRectDims = osc::Dimensions(contentRect);
            glm::vec2 const mousePos = ImGui::GetMousePos();
            osc::Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, osc::Dimensions(contentRect));

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            std::optional<osc::RayCollision> const meshCollision = m_LastTextureHittestResult.isHovered ?
                osc::GetClosestWorldspaceRayCollision(m_State->getInputMesh(m_DocumentIdentifier), osc::Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<TPSTabHover> landmarkCollision = m_LastTextureHittestResult.isHovered ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // hover state: update central hover state
            if (landmarkCollision)
            {
                // update central state to tell it that there's a new hover
                m_State->CurrentHover = landmarkCollision;
            }
            else if (meshCollision)
            {
                m_State->CurrentHover.emplace(meshCollision->position);
            }

            // ensure the camera is updated *before* rendering; otherwise, it'll be one frame late
            updateCamera();

            // render: draw the scene into the content rect and hittest it
            osc::RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
            osc::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = osc::HittestLastImguiItem();

            // handle any events due to hovering over, clicking, etc.
            handleInputAndHoverEvents(m_LastTextureHittestResult, meshCollision, landmarkCollision);

            // draw any 2D ImGui overlays
            drawOverlays(m_LastTextureHittestResult.rect);
        }

        void updateCamera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            if (m_State->LinkCameras && m_Camera != m_State->LinkedCameraBase)
            {
                if (m_State->OnlyLinkRotation)
                {
                    m_Camera.phi = m_State->LinkedCameraBase.phi;
                    m_Camera.theta = m_State->LinkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->LinkedCameraBase;
                }
            }

            // if the user interacts with the render, update the camera as necessary
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiUserInput(osc::Dimensions(m_LastTextureHittestResult.rect), m_Camera))
                {
                    m_State->LinkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<TPSTabHover> getMouseLandmarkCollisions(osc::Line const& cameraRay) const
        {
            std::optional<TPSTabHover> rv;
            for (auto const& [id, pos] : m_State->getInputDocument(m_DocumentIdentifier).Landmarks)
            {
                std::optional<osc::RayCollision> const coll = osc::GetRayCollisionSphere(cameraRay, osc::Sphere{pos, m_LandmarkRadius});
                if (coll)
                {
                    if (!rv || glm::length(rv->WorldspaceLocation - cameraRay.origin) > coll->distance)
                    {
                        TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, id};
                        rv.emplace(std::move(fullID), pos);
                    }
                }
            }
            return rv;
        }

        void handleInputAndHoverEvents(
            osc::ImGuiItemHittestResult const& htResult,
            std::optional<osc::RayCollision> const& meshCollision,
            std::optional<TPSTabHover> const& landmarkCollision)
        {
            // event: if the user clicks and something is hovered, select it; otherwise, add a landmark
            if (htResult.isLeftClickReleasedWithoutDragging)
            {
                if (landmarkCollision && landmarkCollision->MaybeSceneElementID)
                {
                    if (!osc::IsShiftDown())
                    {
                        m_State->UserSelection.clear();
                    }
                    m_State->UserSelection.select(*landmarkCollision->MaybeSceneElementID);
                }
                else if (meshCollision)
                {
                    ActionAddLandmarkTo(
                        *m_State->EditedDocument,
                        m_DocumentIdentifier,
                        meshCollision->position
                    );
                }
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (htResult.isHovered && osc::IsAnyKeyPressed({ImGuiKey_Delete, ImGuiKey_Backspace}))
            {
                ActionDeleteSceneElementsByID(
                    *m_State->EditedDocument,
                    m_State->UserSelection.getUnderlyingSet()
                );
                m_State->UserSelection.clear();
            }
        }

        // draws 2D ImGui overlays over the scene render
        void drawOverlays(osc::Rect const& renderRect)
        {
            ImGui::SetCursorScreenPos(renderRect.p1 + m_OverlayPadding);

            drawInformationIcon();

            ImGui::SameLine();

            drawImportButton();

            ImGui::SameLine();

            drawExportButton();

            ImGui::SameLine();

            drawAutoFitCameraButton();

            ImGui::SameLine();

            drawLandmarkRadiusSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            // use text-like button to ensure the information icon aligns with other row items
            ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
            ImGui::Button(ICON_FA_INFO_CIRCLE);
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

                ImGui::TextDisabled("Input Information:");

                drawInformationTable();

                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ImGui::BeginTable("##inputinfo", 2))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# landmarks");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getInputDocument(m_DocumentIdentifier).Landmarks.size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getInputMesh(m_DocumentIdentifier).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getInputMesh(m_DocumentIdentifier).getIndices().size()/3);

                ImGui::EndTable();
            }
        }

        // draws an import button that enables the user to import things for this input
        void drawImportButton()
        {
            ImGui::Button(ICON_FA_FILE_IMPORT " import" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##importcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh"))
                {
                    ActionBrowseForNewMesh(*m_State->EditedDocument, m_DocumentIdentifier);
                }
                if (ImGui::MenuItem("Landmarks from CSV"))
                {
                    ActionLoadLandmarksCSV(*m_State->EditedDocument, m_DocumentIdentifier);
                }
                ImGui::EndPopup();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            ImGui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObj(m_State->getInputMesh(m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(m_State->getInputMesh(m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Landmarks to CSV"))
                {
                    ActionSaveLandmarksToCSV(m_State->EditedDocument->getScratch(), m_DocumentIdentifier);
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, m_State->getInputMesh(m_DocumentIdentifier).getBounds());
                m_State->LinkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
        }

        // draws a slider that lets the user edit how large the landmarks are
        void drawLandmarkRadiusSlider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            ImGuiSliderFlags const flags = ImGuiSliderFlags_Logarithmic;

            char const* const label = "landmark radius";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - m_OverlayPadding.x);
            ImGui::SliderFloat(label, &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        // renders this panel's 3D scene to a texture
        osc::RenderTexture& renderScene(
            glm::vec2 dims,
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSTabHover> const& maybeLandmarkCollision)
        {
            osc::SceneRendererParams const params = CalcRenderParams(m_Camera, dims);
            std::vector<osc::SceneDecoration> const decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.draw(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<osc::SceneDecoration> generateDecorations(
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSTabHover> const& maybeLandmarkCollision) const
        {
            // generate in-scene 3D decorations
            std::vector<osc::SceneDecoration> decorations;
            decorations.reserve(6 + m_State->getInputDocument(m_DocumentIdentifier).Landmarks.size());  // likely guess

            AppendCommonDecorations(*m_State, m_State->getInputMesh(m_DocumentIdentifier), m_WireframeMode, decorations);

            // append each landmark as a sphere
            for (auto const& [id, pos] : m_State->getInputDocument(m_DocumentIdentifier).Landmarks)
            {
                TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, id};

                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = pos;

                osc::SceneDecoration& decoration = decorations.emplace_back(m_State->LandmarkSphere, transform, m_State->LandmarkColor);

                if (m_State->UserSelection.contains(fullID))
                {
                    decoration.color += glm::vec4{0.25f, 0.25f, 0.25f, 0.0f};
                    decoration.color = glm::clamp(decoration.color, glm::vec4{0.0f}, glm::vec4{1.0f});
                    decoration.flags = osc::SceneDecorationFlags_IsSelected;
                }
                else if (m_State->CurrentHover && m_State->CurrentHover->MaybeSceneElementID == fullID)
                {
                    decoration.color += glm::vec4{0.15f, 0.15f, 0.15f, 0.0f};
                    decoration.color = glm::clamp(decoration.color, glm::vec4{0.0f}, glm::vec4{1.0f});
                    decoration.flags = osc::SceneDecorationFlags_IsHovered;
                }
            }

            // if applicable, show mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = maybeMeshCollision->position;

                glm::vec4 color = m_State->LandmarkColor;
                color.a *= 0.25f;

                decorations.emplace_back(m_State->LandmarkSphere, transform, color);
            }

            return decorations;
        }

        std::shared_ptr<TPSTabSharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getInputMesh(m_DocumentIdentifier).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer{osc::App::config(), osc::App::singleton<osc::MeshCache>(), osc::App::singleton<osc::ShaderCache>()};
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
        glm::vec2 m_OverlayPadding = {10.0f, 10.0f};
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class TPS3DResultPanel final : public TPS3DTabPanel {
    public:

        TPS3DResultPanel(std::string_view panelName_, std::shared_ptr<TPSTabSharedState> state_) :
            TPS3DTabPanel{std::move(panelName_)},
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid sharedState state");
        }

    private:
        void implDrawContent() override
        {
            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            updateCamera();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = renderScene(dims);
            osc::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = osc::HittestLastImguiItem();

            drawOverlays(m_LastTextureHittestResult.rect);
        }

        void updateCamera()
        {
            // if cameras are linked together, ensure all cameras match the "base" camera
            if (m_State->LinkCameras && m_Camera != m_State->LinkedCameraBase)
            {
                if (m_State->OnlyLinkRotation)
                {
                    m_Camera.phi = m_State->LinkedCameraBase.phi;
                    m_Camera.theta = m_State->LinkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->LinkedCameraBase;
                }
            }

            // update camera if user drags it around etc.
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiUserInput(osc::Dimensions(m_LastTextureHittestResult.rect), m_Camera))
                {
                    m_State->LinkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(osc::Rect const& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            ImGui::SetCursorScreenPos(renderRect.p1 + m_OverlayPadding);

            drawInformationIcon();

            ImGui::SameLine();

            drawExportButton();

            ImGui::SameLine();

            drawAutoFitCameraButton();

            ImGui::SameLine();

            {
                ImGui::Checkbox("show destination", &m_ShowDestinationMesh);
            }

            ImGui::SameLine();

            drawBlendingFactorSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            // use text-like button to ensure the information icon aligns with other row items
            ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
            ImGui::Button(ICON_FA_INFO_CIRCLE);
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

                ImGui::TextDisabled("Result Information:");

                drawInformationTable();

                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInformationTable()
        {
            if (ImGui::BeginTable("##inputinfo", 2))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getTransformedMesh().getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", m_State->getTransformedMesh().getIndices().size()/3);

                ImGui::EndTable();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            ImGui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObj(m_State->getTransformedMesh());
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(m_State->getTransformedMesh());
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, m_State->getTransformedMesh().getBounds());
                m_State->LinkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
        }

        void drawBlendingFactorSlider()
        {
            char const* const label = "blending factor";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - m_OverlayPadding.x);
            float factor = m_State->EditedDocument->getScratch().BlendingFactor;
            if (ImGui::SliderFloat(label, &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactor(*m_State->EditedDocument, factor);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                ActionSetBlendFactorAndSave(*m_State->EditedDocument, factor);
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<osc::SceneDecoration> generateDecorations() const
        {
            std::vector<osc::SceneDecoration> decorations;
            decorations.reserve(5);  // likely guess

            AppendCommonDecorations(*m_State, m_State->getTransformedMesh(), m_WireframeMode, decorations);

            if (m_ShowDestinationMesh)
            {
                osc::SceneDecoration& dec = decorations.emplace_back(m_State->EditedDocument->getScratch().Destination.Mesh);
                dec.color = {1.0f, 0.0f, 0.0f, 0.5f};
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered texture
        osc::RenderTexture& renderScene(glm::vec2 dims)
        {
            std::vector<osc::SceneDecoration> const decorations = generateDecorations();
            osc::SceneRendererParams const params = CalcRenderParams(m_Camera, dims);
            return m_CachedRenderer.draw(decorations, params);
        }

        std::shared_ptr<TPSTabSharedState> m_State;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getTransformedMesh().getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            osc::App::singleton<osc::MeshCache>(),
            osc::App::singleton<osc::ShaderCache>()
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        glm::vec2 m_OverlayPadding = {10.0f, 10.0f};
    };

    // draws the top toolbar (bar containing icons for new, save, open, undo, redo, etc.)
    class TPS3DToolbar final {
    public:
        TPS3DToolbar(
            std::string_view label,
            std::shared_ptr<TPSTabSharedState> tabState_) :

            m_Label{std::move(label)},
            m_TabState{std::move(tabState_)}
        {
        }

        void draw()
        {
            float const height = ImGui::GetFrameHeight() + 2.0f*ImGui::GetStyle().WindowPadding.y;
            ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
            if (osc::BeginMainViewportTopBar(m_Label.c_str(), height, flags))
            {
                drawContent();
            }
            ImGui::End();
        }
    private:
        void drawContent()
        {
            // document-related stuff
            drawNewDocumentButton();
            ImGui::SameLine();
            drawOpenDocumentButton();
            ImGui::SameLine();
            drawSaveLandmarksButton();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // undo/redo-related stuff
            m_UndoButton.draw();
            ImGui::SameLine();
            m_RedoButton.draw();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // camera stuff
            drawCameraLockCheckbox();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // landmark stuff
            drawResetLandmarksButton();
        }

        void drawNewDocumentButton()
        {
            if (ImGui::Button(ICON_FA_FILE))
            {
                ActionCreateNewDocument(*m_TabState->EditedDocument);
            }
            osc::DrawTooltipIfItemHovered("Create New Document", "Creates the default scene (undoable)");
        }

        void drawOpenDocumentButton()
        {
            ImGui::Button(ICON_FA_FOLDER_OPEN);

            if (ImGui::BeginPopupContextItem("##OpenFolder", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Load Source Mesh"))
                {
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Destination);
                }
                ImGui::EndPopup();
            }
            osc::DrawTooltipIfItemHovered("Open File", "Open Source/Destination data");
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSaveLandmarksToPairedCSV(m_TabState->EditedDocument->getScratch());
            }
            osc::DrawTooltipIfItemHovered("Save Landmarks to CSV", "Saves all pair-able landmarks to a CSV file, for external processing");
        }

        void drawCameraLockCheckbox()
        {
            {
                bool linkCameras = m_TabState->LinkCameras;
                if (ImGui::Checkbox("link cameras", &linkCameras))
                {
                    m_TabState->LinkCameras = linkCameras;
                }
            }

            ImGui::SameLine();

            {
                bool onlyLinkRotation = m_TabState->OnlyLinkRotation;
                if (ImGui::Checkbox("only link rotation", &onlyLinkRotation))
                {
                    m_TabState->OnlyLinkRotation = onlyLinkRotation;
                }
            }
        }

        void drawResetLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(*m_TabState->EditedDocument);
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSTabSharedState> m_TabState;
        osc::UndoButton m_UndoButton{m_TabState->EditedDocument};
        osc::RedoButton m_RedoButton{m_TabState->EditedDocument};
    };

    // draws the bottom status bar
    class TPS3DStatusBar final {
    public:
        TPS3DStatusBar(
            std::string_view label,
            std::shared_ptr<TPSTabSharedState> tabState_) :

            m_Label{std::move(label)},
            m_TabState{std::move(tabState_)}
        {
        }

        void draw()
        {
            if (osc::BeginMainViewportBottomBar(m_Label.c_str()))
            {
                drawContent();
            }
            ImGui::End();
        }

    private:
        void drawContent()
        {
            if (m_TabState->CurrentHover)
            {
                glm::vec3 const pos = m_TabState->CurrentHover->WorldspaceLocation;
                ImGui::TextUnformatted("(");
                ImGui::SameLine();
                for (int i = 0; i < 3; ++i)
                {
                    glm::vec4 color = {0.5f, 0.5f, 0.5f, 1.0f};
                    color[i] = 1.0f;
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::Text("%f", pos[i]);
                    ImGui::SameLine();
                    ImGui::PopStyleColor();
                }
                ImGui::TextUnformatted(")");
                ImGui::SameLine();
                if (m_TabState->CurrentHover->MaybeSceneElementID)
                {
                    ImGui::TextDisabled("(left-click to select %s)", m_TabState->CurrentHover->MaybeSceneElementID->elementID.c_str());
                }
                else
                {
                    ImGui::TextDisabled("(left-click to add a landmark)");
                }
            }
            else
            {
                ImGui::TextDisabled("(nothing hovered)");
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSTabSharedState> m_TabState;
    };

    // draws the 'file' menu (a sub menu of the main menu)
    class TPS3DFileMenu final {
    public:
        explicit TPS3DFileMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_TabState{std::move(tabState_)}
        {
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
            if (ImGui::MenuItem(ICON_FA_FILE " New"))
            {
                ActionCreateNewDocument(*m_TabState->EditedDocument);
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT " Import"))
            {
                drawImportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
            {
                drawExportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES " Close"))
            {
                m_TabState->requestCloseTab();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit"))
            {
                osc::App::upd().requestQuit();
            }
        }

        void drawImportMenuContent()
        {
            if (ImGui::MenuItem("Source Mesh"))
            {
                ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Mesh"))
            {
                ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_TabState->EditedDocument, TPSDocumentInputIdentifier::Destination);
            }
        }

        void drawExportMenuContent()
        {
            if (ImGui::MenuItem("Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_TabState->EditedDocument->getScratch(), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_TabState->EditedDocument->getScratch(), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Landmark Pairs to CSV"))
            {
                ActionSaveLandmarksToPairedCSV(m_TabState->EditedDocument->getScratch());
            }
        }

        std::shared_ptr<TPSTabSharedState> m_TabState;
    };

    // draws the 'edit' menu (a sub menu of the main menu)
    class TPS3DEditMenu final {
    public:
        TPS3DEditMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_TabState{std::move(tabState_)}
        {
        }

        void draw()
        {
            if (ImGui::BeginMenu("Edit"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }

    private:

        void drawContent()
        {
            if (ImGui::MenuItem("Undo", nullptr, nullptr, m_TabState->EditedDocument->canUndo()))
            {
                ActionUndo(*m_TabState->EditedDocument);
            }
            if (ImGui::MenuItem("Redo", nullptr, nullptr, m_TabState->EditedDocument->canRedo()))
            {
                ActionRedo(*m_TabState->EditedDocument);
            }
        }

        std::shared_ptr<TPSTabSharedState> m_TabState;
    };

    // draws the 'window' menu (a sub menu of the main menu)
    class TPS3DWindowMenu final {
    public:
        TPS3DWindowMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_TabState{std::move(tabState_)}
        {
        }

        void draw()
        {
            if (ImGui::BeginMenu("Window"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }
    private:
        void drawContent()
        {
            for (TPSUIPanelInfo& panel : m_TabState->Panels)
            {
                bool selected = panel.Instance.has_value();
                if (ImGui::MenuItem(panel.Name.c_str(), nullptr, &selected))
                {
                    if (panel.Instance.has_value() && (*panel.Instance)->isOpen())
                    {
                        panel.Instance.reset();
                    }
                    else
                    {
                        panel.Instance = panel.ConstructorFunc(m_TabState);
                        (*panel.Instance)->open();
                    }
                }
            }
        }

        std::shared_ptr<TPSTabSharedState> m_TabState;
    };

    // draws the main menu content (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class TPS3DMainMenu final {
    public:
        explicit TPS3DMainMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_WindowMenu{tabState_},
            m_AboutTab{}
        {
        }

        void draw()
        {
            m_FileMenu.draw();
            m_EditMenu.draw();
            m_WindowMenu.draw();
            m_AboutTab.draw();
        }
    private:
        TPS3DFileMenu m_FileMenu;
        TPS3DEditMenu m_EditMenu;
        TPS3DWindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutTab;
    };

    std::vector<TPSUIPanelInfo> GetAvailablePanels()
    {
        return
        {
            {
                "Source Mesh",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<TPS3DInputPanel>("Source Mesh", std::move(st), TPSDocumentInputIdentifier::Source);
                },
                true,
            },
            {
                "Destination Mesh",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<TPS3DInputPanel>("Destination Mesh", std::move(st), TPSDocumentInputIdentifier::Destination);
                },
                true,
            },
            {
                "Result",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<TPS3DResultPanel>("Result", std::move(st));
                },
                true
            },
            {
                "History",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<osc::UndoRedoPanel>("History", st->EditedDocument);
                },
                false,
            },
            {
                "Log",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<osc::LogViewerPanel>("Log");
                },
                false,
            },
            {
                "Performance",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<osc::PerfPanel>("Performance");
                },
                false,
            },
        };
    }
}

// tab implementation
class osc::TPS3DTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        OSC_ASSERT(m_Parent != nullptr);
        OSC_ASSERT(m_TabState != nullptr && "the tab state should be initialized by this point");

        // initialize default-open tabs
        for (TPSUIPanelInfo& panel : m_TabState->Panels)
        {
            if (panel.IsEnabledByDefault)
            {
                panel.Instance = panel.ConstructorFunc(m_TabState);
                (*panel.Instance)->open();
            }
        }
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
        App::upd().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        return false;
    }

    void onTick()
    {
        // re-perform hover test each frame
        m_TabState->CurrentHover.reset();

        // garbage collect closed-panel instance data
        for (TPSUIPanelInfo& panel : m_TabState->Panels)
        {
            if (panel.Instance && !(*panel.Instance)->isOpen())
            {
                panel.Instance.reset();
            }
        }
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.draw();
        for (TPSUIPanelInfo& panel : m_TabState->Panels)
        {
            if (panel.Instance)
            {
                (*panel.Instance)->draw();
            }
        }
        m_StatusBar.draw();
    }

private:

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " TPS3DTab";
    TabHost* m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<TPSTabSharedState> m_TabState = std::make_shared<TPSTabSharedState>(m_ID, m_Parent);

    // not-user-toggleable widgets
    TPS3DMainMenu m_MainMenu{m_TabState};
    TPS3DToolbar m_TopToolbar{"##TPS3DToolbar", m_TabState};
    TPS3DStatusBar m_StatusBar{"##TPS3DStatusBar", m_TabState};
};


// public API (PIMPL)

osc::TPS3DTab::TPS3DTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::TPS3DTab::TPS3DTab(TPS3DTab&&) noexcept = default;
osc::TPS3DTab& osc::TPS3DTab::operator=(TPS3DTab&&) noexcept = default;
osc::TPS3DTab::~TPS3DTab() noexcept = default;

osc::UID osc::TPS3DTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::TPS3DTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::TPS3DTab::implParent() const
{
    return m_Impl->parent();
}

void osc::TPS3DTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::TPS3DTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::TPS3DTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::TPS3DTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::TPS3DTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::TPS3DTab::implOnDraw()
{
    m_Impl->onDraw();
}
