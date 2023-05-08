#include "TPS3DTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Formats/CSV.hpp"
#include "src/Formats/OBJ.hpp"
#include "src/Formats/STL.hpp"
#include "src/Graphics/CachedSceneRenderer.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
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
#include "src/Maths/Segment.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/Graphics/SimTKMeshLoader.hpp"
#include "src/OpenSimBindings/TPS3D.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/Panels/LogViewerPanel.hpp"
#include "src/Panels/Panel.hpp"
#include "src/Panels/PanelManager.hpp"
#include "src/Panels/PerfPanel.hpp"
#include "src/Panels/StandardPanel.hpp"
#include "src/Panels/ToggleablePanelFlags.hpp"
#include "src/Panels/UndoRedoPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/UndoRedo.hpp"
#include "src/Widgets/Popup.hpp"
#include "src/Widgets/PopupManager.hpp"
#include "src/Widgets/RedoButton.hpp"
#include "src/Widgets/StandardPopup.hpp"
#include "src/Widgets/UndoButton.hpp"
#include "src/Widgets/WindowMenu.hpp"

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

// generic graphics algorithms/constants
//
// (these have nothing to do with TPS, but are used to help render the UI)
namespace
{
    static glm::vec2 constexpr c_OverlayPadding = {10.0f, 10.0f};
    static osc::Color constexpr c_PairedLandmarkColor = osc::Color::green();
    static osc::Color constexpr c_UnpairedLandmarkColor = osc::Color::red();
}

// TPS document datastructure
//
// this covers the datastructures that the user is dynamically editing
namespace
{
    // an enum used to identify one of the two inputs (source/destination) of the TPS
    // document at runtime
    enum class TPSDocumentInputIdentifier {
        Source = 0,
        Destination,
        TOTAL,
    };

    // an enum used to identify what type of part of the input is described
    enum class TPSDocumentInputElementType {
        Landmark = 0,
        Mesh,
        TOTAL,
    };

    // a single landmark pair in the TPS document
    //
    // (can be midway through definition by the user)
    struct TPSDocumentLandmarkPair final {

        explicit TPSDocumentLandmarkPair(std::string id_) :
            id{std::move(id_)}
        {
        }

        std::string id;
        std::optional<glm::vec3> maybeSourceLocation;
        std::optional<glm::vec3> maybeDestinationLocation;
    };

    // the whole TPS document that the user edits in-place
    struct TPSDocument final {
        osc::Mesh sourceMesh = osc::GenUntexturedUVSphere(16, 16);
        osc::Mesh destinationMesh = osc::GenUntexturedSimbodyCylinder(16);
        std::vector<TPSDocumentLandmarkPair> landmarkPairs;
        float blendingFactor = 1.0f;
        size_t nextLandmarkID = 0;
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
    // helper: returns the (mutable) source/destination of the given landmark pair, if available
    std::optional<glm::vec3>& UpdLocation(TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(static_cast<int>(TPSDocumentInputIdentifier::TOTAL) == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // helper: returns the source/destination of the given landmark pair, if available
    std::optional<glm::vec3> const& GetLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(static_cast<int>(TPSDocumentInputIdentifier::TOTAL) == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // helper: returns the source/destination mesh in the given document (mutable)
    osc::Mesh& UpdMesh(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(static_cast<int>(TPSDocumentInputIdentifier::TOTAL) == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // helper: returns the source/destination mesh in the given document
    osc::Mesh const& GetMesh(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(static_cast<int>(TPSDocumentInputIdentifier::TOTAL) == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // helper: returns `true` if both the source and destination are defined for the given UI landmark
    bool IsFullyPaired(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // helper: returns `true` if the given UI landmark has either a source or a destination defined
    bool HasSourceOrDestinationLocation(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // helper: returns source + destination landmark pair, if the provided UI landmark has both defined
    std::optional<osc::LandmarkPair3D> TryExtractLandmarkPair(TPSDocumentLandmarkPair const& p)
    {
        if (p.maybeSourceLocation && p.maybeDestinationLocation)
        {
            return osc::LandmarkPair3D{*p.maybeSourceLocation, *p.maybeDestinationLocation};
        }
        else
        {
            return std::nullopt;
        }
    }

    // helper: returns all paired landmarks in the document argument
    std::vector<osc::LandmarkPair3D> GetLandmarkPairs(TPSDocument const& doc)
    {
        std::vector<osc::LandmarkPair3D> rv;
        rv.reserve(std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), IsFullyPaired));

        for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
        {
            if (std::optional<osc::LandmarkPair3D> maybePair = TryExtractLandmarkPair(p))
            {
                rv.emplace_back(*maybePair);
            }
        }
        return rv;
    }

    // helper: returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        size_t rv = 0;
        for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
        {
            if (GetLocation(p, which))
            {
                ++rv;
            }
        }
        return rv;
    }

    // helper: add a source/destination landmark at the given location
    void AddLandmarkToInput(
        TPSDocument& doc,
        TPSDocumentInputIdentifier which,
        glm::vec3 const& pos)
    {
        // first, try assigning it to an empty slot in the existing data
        //
        // (e.g. imagine the caller added a few source points and is now
        //       trying to add destination points - they should probably
        //       be paired in-sequence with the unpaired source points)
        bool wasAssignedToExistingEmptySlot = false;
        for (TPSDocumentLandmarkPair& p : doc.landmarkPairs)
        {
            std::optional<glm::vec3>& maybeLoc = UpdLocation(p, which);
            if (!maybeLoc)
            {
                maybeLoc = pos;
                wasAssignedToExistingEmptySlot = true;
                break;
            }
        }

        // if there wasn't an empty slot, then create a new landmark pair and
        // assign the location to the relevant part of the pair
        if (!wasAssignedToExistingEmptySlot)
        {
            std::stringstream ss;
            ss << "landmark_" << doc.nextLandmarkID++;
            TPSDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(std::move(ss).str());
            UpdLocation(p, which) = pos;
        }
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
    void ActionAddLandmarkTo(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentInputIdentifier which,
        glm::vec3 const& pos)
    {
        AddLandmarkToInput(doc.updScratch(), which, pos);
        doc.commitScratch("added landmark");
    }

    // action: prompt the user to browse for a different source mesh
    void ActionBrowseForNewMesh(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeMeshPath = osc::PromptUserForFile("vtp,obj");
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }

        UpdMesh(doc.updScratch(), which) = osc::LoadMeshViaSimTK(*maybeMeshPath);

        doc.commitScratch("changed mesh");
    }

    // action: load landmarks from a headerless CSV file into source/destination
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
            return;  // the landmarks file was empty, or had invalid data
        }

        for (glm::vec3 const& landmark : landmarks)
        {
            AddLandmarkToInput(doc.updScratch(), which, landmark);
        }

        doc.commitScratch("loaded landmarks");
    }

    // action: set the TPS blending factor for the result, but don't save it to undo/redo storage
    void ActionSetBlendFactorWithoutSaving(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        doc.updScratch().blendingFactor = factor;
    }

    // action: set the TPS blending factor and save a commit of the change
    void ActionSetBlendFactorAndSave(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        ActionSetBlendFactorWithoutSaving(doc, factor);
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
        doc.updScratch().landmarkPairs.clear();
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
            if (id.elementType == TPSDocumentInputElementType::Landmark)
            {
                auto it = std::find_if(
                    scratch.landmarkPairs.begin(),
                    scratch.landmarkPairs.end(),
                    [&id](TPSDocumentLandmarkPair const& p) { return p.id == id.elementID; }
                );
                if (it != scratch.landmarkPairs.end())
                {
                    UpdLocation(*it, id.whichInput).reset();

                    if (!HasSourceOrDestinationLocation(*it))
                    {
                        // the landmark now has no data associated with it: garbage collect it
                        scratch.landmarkPairs.erase(it);
                    }
                }
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

        auto fOutput = std::make_shared<std::ofstream>(*maybeCSVPath);
        if (!fOutput)
        {
            return;  // couldn't open file for writing
        }
        osc::CSVWriter writer{fOutput};

        std::vector<std::string> cols(3);
        for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
        {
            if (std::optional<glm::vec3> const loc = GetLocation(p, which))
            {
                cols.at(0) = std::to_string(loc->x);
                cols.at(1) = std::to_string(loc->y);
                cols.at(2) = std::to_string(loc->z);
                writer.writeRow(cols);
            }
        }
    }

    // action: save all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSaveLandmarksToPairedCSV(TPSDocument const& doc)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        auto fOutput = std::make_shared<std::ofstream>(*maybeCSVPath);
        if (!(*fOutput))
        {
            return;  // couldn't open file for writing
        }
        osc::CSVWriter writer{fOutput};

        std::vector<osc::LandmarkPair3D> const pairs = GetLandmarkPairs(doc);
        std::vector<std::string> cols =
        {
            "source.x",
            "source.y",
            "source.z",
            "dest.x",
            "dest.y",
            "dest.z",
        };

        // write header
        {
            writer.writeRow(cols);
        }

        // write data rows
        for (osc::LandmarkPair3D const& p : pairs)
        {
            cols.at(0) = std::to_string(p.source.x);
            cols.at(1) = std::to_string(p.source.y);
            cols.at(2) = std::to_string(p.source.z);

            cols.at(0) = std::to_string(p.destination.x);
            cols.at(1) = std::to_string(p.destination.y);
            cols.at(2) = std::to_string(p.destination.z);
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

        auto outFile = std::make_shared<std::ofstream>(*maybeOBJFile, std::ios_base::out | std::ios_base::trunc);
        if (!(*outFile))
        {
            return;  // couldn't open for writing
        }
        osc::ObjWriter writer{outFile};

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

        auto outFile = std::make_shared<std::ofstream>(
            *maybeSTLPath,
            std::ios_base::binary | std::ios_base::out | std::ios_base::trunc
        );
        if (!(*outFile))
        {
            return;  // couldn't open for writing
        }
        osc::StlWriter writer{outFile};

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
            if (m_CachedSourceMesh != doc.sourceMesh)
            {
                m_CachedSourceMesh = doc.sourceMesh;
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
                doc.blendingFactor,
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

// TPSUI top-level state
//
// these are datastructures that are shared between panels etc.
namespace
{
    // holds information about the user's current mouse hover
    struct TPSUIViewportHover final {

        explicit TPSUIViewportHover(glm::vec3 const& worldspaceLocation_) :
            maybeSceneElementID{std::nullopt},
            worldspaceLocation{worldspaceLocation_}
        {
        }

        TPSUIViewportHover(
            TPSDocumentElementID sceneElementID_,
            glm::vec3 const& worldspaceLocation_) :

            maybeSceneElementID{std::move(sceneElementID_)},
            worldspaceLocation{worldspaceLocation_}
        {
        }

        std::optional<TPSDocumentElementID> maybeSceneElementID;
        glm::vec3 worldspaceLocation;
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

    // shared, top-level TPS3D tab state
    //
    // (shared by all panels)
    struct TPSTabSharedState final {

        TPSTabSharedState(
            osc::UID tabID_,
            std::weak_ptr<osc::TabHost> parent_) :

            tabID{std::move(tabID_)},
            tabHost{std::move(parent_)}
        {
            OSC_ASSERT(tabHost.lock() != nullptr && "top-level tab host required for this UI");
        }

        // ID of the top-level TPS3D tab
        osc::UID tabID;

        // handle to the screen that owns the TPS3D tab
        std::weak_ptr<osc::TabHost> tabHost;

        // cached TPS3D algorithm result (to prevent recomputing it each frame)
        TPSResultCache meshResultCache;

        // the document the user is editing
        std::shared_ptr<osc::UndoRedoT<TPSDocument>> editedDocument = std::make_shared<osc::UndoRedoT<TPSDocument>>();

        // `true` if the user wants the cameras to be linked
        bool linkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool onlyLinkRotation = false;

        // shared linked camera
        osc::PolarPerspectiveCamera linkedCameraBase = CreateCameraFocusedOn(editedDocument->getScratch().sourceMesh.getBounds());

        // wireframe material, used to draw scene elements in a wireframe style
        osc::Material wireframeMaterial = osc::CreateWireframeOverlayMaterial(osc::App::config(), *osc::App::singleton<osc::ShaderCache>());

        // shared sphere mesh (used by rendering code)
        osc::Mesh landmarkSphere = osc::App::singleton<osc::MeshCache>()->getSphereMesh();

        // current user selection
        TPSTabSelection userSelection;

        // current user hover: reset per-frame
        std::optional<TPSUIViewportHover> currentHover;

        // available/active panels that the user can toggle via the `window` menu
        std::shared_ptr<osc::PanelManager> panelManager = std::make_shared<osc::PanelManager>();

        // currently active tab-wide popups
        osc::PopupManager popupManager;
    };

    TPSDocument const& GetScratch(TPSTabSharedState const& state)
    {
        return state.editedDocument->getScratch();
    }

    TPSDocument& UpdScratch(TPSTabSharedState& state)
    {
        return state.editedDocument->updScratch();
    }

    osc::Mesh const& GetScratchMesh(TPSTabSharedState& state, TPSDocumentInputIdentifier which)
    {
        return GetMesh(GetScratch(state), std::move(which));
    }

    // returns a (potentially cached) post-TPS-warp mesh
    osc::Mesh const& GetResultMesh(TPSTabSharedState& state)
    {
        return state.meshResultCache.lookup(state.editedDocument->getScratch());
    }

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSTabSharedState const& sharedState,
        osc::Mesh const& tpsSourceOrDestinationMesh,
        bool wireframeMode,
        std::function<void(osc::SceneDecoration&&)> const& out,
        osc::Color meshColor = osc::Color::white())
    {
        // draw the mesh
        {
            osc::SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.color = meshColor;
            out(std::move(dec));
        }

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            osc::SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.maybeMaterial = sharedState.wireframeMaterial;
            out(std::move(dec));
        }

        // add grid decorations
        DrawXZGrid(*osc::App::singleton<osc::MeshCache>(), out);
        DrawXZFloorLines(*osc::App::singleton<osc::MeshCache>(), out, 100.0f);
    }
}


// TPS3D UI widgets
//
// these appear within panels in the UI
namespace
{
    // widget: the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class TPS3DToolbar final {
    public:
        TPS3DToolbar(
            std::string_view label,
            std::shared_ptr<TPSTabSharedState> tabState_) :

            m_Label{std::move(label)},
            m_State{std::move(tabState_)}
        {
        }

        void draw()
        {
            float const height = ImGui::GetFrameHeight() + 2.0f*ImGui::GetStyle().WindowPadding.y;
            ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
            if (osc::BeginMainViewportTopBar(m_Label, height, flags))
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
                ActionCreateNewDocument(*m_State->editedDocument);
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
                    ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
                }
                ImGui::EndPopup();
            }
            osc::DrawTooltipIfItemHovered("Open File", "Open Source/Destination data");
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSaveLandmarksToPairedCSV(GetScratch(*m_State));
            }
            osc::DrawTooltipIfItemHovered("Save Landmarks to CSV", "Saves all pair-able landmarks to a CSV file, for external processing");
        }

        void drawCameraLockCheckbox()
        {
            {
                bool linkCameras = m_State->linkCameras;
                if (ImGui::Checkbox("link cameras", &linkCameras))
                {
                    m_State->linkCameras = linkCameras;
                }
            }

            ImGui::SameLine();

            {
                bool onlyLinkRotation = m_State->onlyLinkRotation;
                if (ImGui::Checkbox("only link rotation", &onlyLinkRotation))
                {
                    m_State->onlyLinkRotation = onlyLinkRotation;
                }
            }
        }

        void drawResetLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(*m_State->editedDocument);
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSTabSharedState> m_State;
        osc::UndoButton m_UndoButton{m_State->editedDocument};
        osc::RedoButton m_RedoButton{m_State->editedDocument};
    };

    // widget: bottom status bar (shows status messages, hover information, etc.)
    class TPS3DStatusBar final {
    public:
        TPS3DStatusBar(
            std::string_view label,
            std::shared_ptr<TPSTabSharedState> tabState_) :

            m_Label{std::move(label)},
            m_State{std::move(tabState_)}
        {
        }

        void draw()
        {
            if (osc::BeginMainViewportBottomBar(m_Label))
            {
                drawContent();
            }
            ImGui::End();
        }

    private:
        void drawContent()
        {
            if (m_State->currentHover)
            {
                glm::vec3 const pos = m_State->currentHover->worldspaceLocation;
                ImGui::TextUnformatted("(");
                ImGui::SameLine();
                for (int i = 0; i < 3; ++i)
                {
                    osc::Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                    color[i] = 1.0f;
                    ImGui::PushStyleColor(ImGuiCol_Text, glm::vec4{color});
                    ImGui::Text("%f", pos[i]);
                    ImGui::SameLine();
                    ImGui::PopStyleColor();
                }
                ImGui::TextUnformatted(")");
                ImGui::SameLine();
                if (m_State->currentHover->maybeSceneElementID)
                {
                    ImGui::TextDisabled("(left-click to select %s)", m_State->currentHover->maybeSceneElementID->elementID.c_str());
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
        std::shared_ptr<TPSTabSharedState> m_State;
    };

    // widget: the 'file' menu (a sub menu of the main menu)
    class TPS3DFileMenu final {
    public:
        explicit TPS3DFileMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
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
                ActionCreateNewDocument(*m_State->editedDocument);
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
                m_State->tabHost.lock()->closeTab(m_State->tabID);
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
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Mesh"))
            {
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
        }

        void drawExportMenuContent()
        {
            if (ImGui::MenuItem("Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(GetScratch(*m_State), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(GetScratch(*m_State), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Landmark Pairs to CSV"))
            {
                ActionSaveLandmarksToPairedCSV(GetScratch(*m_State));
            }
        }

        std::shared_ptr<TPSTabSharedState> m_State;
    };

    // widget: the 'edit' menu (a sub menu of the main menu)
    class TPS3DEditMenu final {
    public:
        explicit TPS3DEditMenu(std::shared_ptr<TPSTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
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
            if (ImGui::MenuItem("Undo", nullptr, nullptr, m_State->editedDocument->canUndo()))
            {
                ActionUndo(*m_State->editedDocument);
            }
            if (ImGui::MenuItem("Redo", nullptr, nullptr, m_State->editedDocument->canRedo()))
            {
                ActionRedo(*m_State->editedDocument);
            }
        }

        std::shared_ptr<TPSTabSharedState> m_State;
    };

    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class TPS3DMainMenu final {
    public:
        explicit TPS3DMainMenu(std::shared_ptr<TPSTabSharedState> const& tabState_) :
            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_WindowMenu{tabState_->panelManager},
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
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutTab;
    };
}

// TPSUI UI popups
//
// popups that can be opened by panels/buttons in the rest of the UI
namespace
{
    // a pairing of an ID with a location in space
    struct IDedLocation final {

        IDedLocation(std::string id_, glm::vec3 const& location_) :
            id{std::move(id_)},
            location{location_}
        {
        }

        std::string id;
        glm::vec3 location;
    };

    // a popup that prompts a user to select landmarks etc. for adding a new frame
    class TPS3DDefineFramePopup final : public osc::StandardPopup {
    public:
        TPS3DDefineFramePopup(
            std::shared_ptr<TPSTabSharedState> state_,
            osc::PolarPerspectiveCamera const& camera_,
            bool wireframeMode_,
            float landmarkRadius_,
            IDedLocation originLandmark_) :

            StandardPopup{"##FrameEditorOverlay", {}, osc::GetMinimalWindowFlags() & ~(ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs)},
            m_State{std::move(state_)},
            m_Camera{camera_},
            m_OriginLandmark{std::move(originLandmark_)},
            m_FirstLandmark{std::nullopt},
            m_SecondLandmark{std::nullopt},
            m_FlipFirstEdge{false},
            m_FlipSecondEdge{false},
            m_EdgeIndexToAxisIndex{{0, 1, 2}},
            m_CachedRenderer{osc::App::config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()},
            m_WireframeMode{std::move(wireframeMode_)},
            m_LandmarkRadius{std::move(landmarkRadius_)}
        {
            setModal(true);
        }

        using osc::StandardPopup::setRect;

    private:
        void implBeforeImguiBeginPopup() final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }

        void implAfterImguiBeginPopup() final
        {
            ImGui::PopStyleVar();
        }

        void implDrawContent() final
        {
            if (ImGui::IsKeyReleased(ImGuiKey_Escape))
            {
                requestClose();
            }

            // compute: top-level UI variables (render rect, mouse pos, etc.)
            osc::Rect const contentRect = osc::ContentRegionAvailScreenRect();
            glm::vec2 const contentRectDims = osc::Dimensions(contentRect);
            glm::vec2 const mousePos = ImGui::GetMousePos();
            osc::Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, osc::Dimensions(contentRect));

            // hittest: calculate which landmark is under the mouse (if any)
            std::optional<IDedLocation> const maybeHoveredLandmark = osc::IsPointInRect(contentRect, mousePos) ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // camera: update from input state
            if (osc::IsPointInRect(contentRect, mousePos))
            {
                osc::UpdatePolarCameraFromImGuiMouseInputs(osc::Dimensions(contentRect), m_Camera);
            }

            // render: render 3D scene to a texture based on current state+hovering
            osc::RenderTexture& sceneRender = renderScene(contentRectDims, maybeHoveredLandmark);
            osc::DrawTextureAsImGuiImage(sceneRender);
            osc::ImGuiItemHittestResult const htResult = osc::HittestLastImguiItem();

            // events: handle any changes due to hovering over, clicking, etc.
            handleInputAndHoverEvents(htResult, maybeHoveredLandmark);

            // 2D UI: draw 2D elements (buttons, text, etc.) as an overlay
            drawOverlays(contentRect);
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<IDedLocation> getMouseLandmarkCollisions(osc::Line const& cameraRay) const
        {
            std::optional<IDedLocation> rv;
            glm::vec3 worldspaceLoc = {};
            for (TPSDocumentLandmarkPair const& p : GetScratch(*m_State).landmarkPairs)
            {
                if (!p.maybeSourceLocation)
                {
                    // doesn't have a source/destination landmark
                    continue;
                }
                // else: hittest the landmark as a sphere

                std::optional<osc::RayCollision> const coll = osc::GetRayCollisionSphere(cameraRay, osc::Sphere{*p.maybeSourceLocation, m_LandmarkRadius});
                if (coll)
                {
                    if (!rv || glm::length(worldspaceLoc - cameraRay.origin) > coll->distance)
                    {
                        rv.emplace(p.id, *p.maybeSourceLocation);
                        worldspaceLoc = coll->position;
                    }
                }
            }
            return rv;
        }

        // renders this panel's 3D scene to a texture
        osc::RenderTexture& renderScene(
            glm::vec2 renderDimensions,
            std::optional<IDedLocation> const& maybeHoveredLandmark)
        {
            osc::SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                osc::App::get().getMSXAASamplesRecommended(),
                renderDimensions
            );
            std::vector<osc::SceneDecoration> const decorations = generateDecorations(maybeHoveredLandmark);
            return m_CachedRenderer.draw(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<osc::SceneDecoration> generateDecorations(
            std::optional<IDedLocation> const& maybeHoveredLandmark) const
        {
            std::vector<osc::SceneDecoration> rv;
            auto appendToRv = [&rv](osc::SceneDecoration&& dec) { rv.push_back(std::move(dec)); };

            // append common decorations (the mesh, grid, etc.)
            AppendCommonDecorations(
                *m_State,
                GetScratchMesh(*m_State, TPSDocumentInputIdentifier::Source),
                m_WireframeMode,
                appendToRv,
                osc::Color{1.0f, 1.0f, 1.0f, 0.25f}
            );

            // append not-special landmarks (i.e. landmarks that aren't part of the selection)
            for (TPSDocumentLandmarkPair const& p : GetScratch(*m_State).landmarkPairs)
            {
                if (p.id == m_OriginLandmark.id ||
                    (m_FirstLandmark && p.id == m_FirstLandmark->id) ||
                    (m_SecondLandmark && p.id == m_SecondLandmark->id))
                {
                    // it's a special landmark: don't draw it
                    continue;
                }

                if (!p.maybeSourceLocation)
                {
                    // no source location data: don't draw it
                    continue;
                }

                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = *p.maybeSourceLocation;

                osc::SceneDecoration& decoration = rv.emplace_back(m_State->landmarkSphere);
                decoration.transform = transform;
                if (maybeHoveredLandmark && maybeHoveredLandmark->id == p.id && !(m_FirstLandmark && m_SecondLandmark))
                {
                    glm::vec3 const start = m_OriginLandmark.location;
                    glm::vec3 startToEnd = *p.maybeSourceLocation - start;
                    if (!m_FirstLandmark && m_FlipFirstEdge)
                    {
                        startToEnd = -startToEnd;
                    }

                    // hovering this non-special landmark and can make it the first/second
                    decoration.color = {1.0f, 1.0f, 1.0f, 0.9f};
                    decoration.flags |= osc::SceneDecorationFlags_IsHovered;

                    osc::ArrowProperties props;
                    props.worldspaceStart = start;
                    props.worldspaceEnd = start + startToEnd;
                    props.tipLength = 2.0f*m_LandmarkRadius;
                    props.neckThickness = 0.25f*m_LandmarkRadius;
                    props.headThickness = 0.5f*m_LandmarkRadius;
                    if (!m_FirstLandmark)
                    {
                        props.color = {0.0f, 0.0f, 0.0f, 0.25f};
                        props.color[m_EdgeIndexToAxisIndex[0]] = 1.0f;
                    }
                    else
                    {
                        props.color = {1.0f, 1.0f, 1.0f, 0.25f};
                    }
                    osc::DrawArrow(*osc::App::singleton<osc::MeshCache>(), props, appendToRv);
                }
                else
                {
                    decoration.color = {1.0f, 1.0f, 1.0f, 0.80f};
                }
            }

            // draw origin
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = m_OriginLandmark.location;

                osc::SceneDecoration& decoration = rv.emplace_back(m_State->landmarkSphere);
                decoration.transform = transform;
                decoration.color = osc::Color::white();
            }

            // draw first landmark
            if (m_FirstLandmark)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = m_FirstLandmark->location;

                osc::SceneDecoration& decoration = rv.emplace_back(m_State->landmarkSphere);
                decoration.transform = transform;
                decoration.color = osc::Color::white();
                if (maybeHoveredLandmark && maybeHoveredLandmark->id == m_FirstLandmark->id)
                {
                    // hovering over first landmark (can be deselected)
                    decoration.flags |= osc::SceneDecorationFlags_IsHovered;
                }
                else
                {
                    decoration.flags |= osc::SceneDecorationFlags_IsSelected;
                }

                glm::vec3 const start = m_OriginLandmark.location;
                glm::vec3 startToEnd = m_FirstLandmark->location - start;
                if (m_FlipFirstEdge)
                {
                    startToEnd = -startToEnd;
                }

                osc::ArrowProperties props;
                props.worldspaceStart = start;
                props.worldspaceEnd = start + startToEnd;
                props.tipLength = 2.0f*m_LandmarkRadius;
                props.neckThickness = 0.25f*m_LandmarkRadius;
                props.headThickness = 0.5f*m_LandmarkRadius;
                props.color = {0.0f, 0.0f, 0.0f, 1.0f};
                props.color[m_EdgeIndexToAxisIndex[0]] = 1.0f;

                osc::DrawArrow(*osc::App::singleton<osc::MeshCache>(), props, appendToRv);
            }

            // draw second landmark
            if (m_SecondLandmark)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = m_SecondLandmark->location;

                osc::SceneDecoration& decoration = rv.emplace_back(m_State->landmarkSphere);
                decoration.transform = transform;
                decoration.color = osc::Color::white();
                if (maybeHoveredLandmark && maybeHoveredLandmark->id == m_SecondLandmark->id)
                {
                    // hovering over second landmark (can be deselected)
                    decoration.flags |= osc::SceneDecorationFlags_IsHovered;
                }
                else
                {
                    decoration.flags |= osc::SceneDecorationFlags_IsSelected;
                }

                osc::ArrowProperties props;
                props.worldspaceStart = m_OriginLandmark.location;
                props.worldspaceEnd = m_SecondLandmark->location;
                props.tipLength = 2.0f*m_LandmarkRadius;
                props.neckThickness = 0.25f*m_LandmarkRadius;
                props.headThickness = 0.5f*m_LandmarkRadius;
                props.color = {1.0f, 1.0f, 1.0f, 0.75f};
                osc::DrawArrow(*osc::App::singleton<osc::MeshCache>(), props, appendToRv);
            }

            // if applicable, draw completed frame
            //
            // (assume X is already drawn)
            if (m_FirstLandmark && m_SecondLandmark)
            {
                float const legLen = 2.0f * m_LandmarkRadius;
                float const legThickness = 0.33f * m_LandmarkRadius;

                glm::vec3 const origin = m_OriginLandmark.location;
                glm::vec3 firstEdgeDir = glm::normalize(m_FirstLandmark->location - origin);
                if (m_FlipFirstEdge)
                {
                    firstEdgeDir = -firstEdgeDir;
                }
                glm::vec3 secondEdgeDir = glm::normalize(m_SecondLandmark->location - origin);
                if (m_FlipSecondEdge)
                {
                    secondEdgeDir = -secondEdgeDir;
                }

                std::array<glm::vec3, 3> edges{};
                edges[0] = firstEdgeDir;
                edges[1] = glm::cross(firstEdgeDir, secondEdgeDir);
                edges[2] = glm::cross(edges[0], edges[1]);

                // map axes via mapping lut
                std::array<glm::vec3, 3> axes{};
                axes[m_EdgeIndexToAxisIndex[0]] = edges[0];
                axes[m_EdgeIndexToAxisIndex[1]] = edges[1];
                axes[m_EdgeIndexToAxisIndex[2]] = edges[2];

                for (size_t i = 0; i < axes.size(); ++i)
                {
                    osc::Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                    color[static_cast<int>(i)] = 1.0f;

                    osc::DrawLineSegment(
                        *osc::App::singleton<osc::MeshCache>(),
                        {origin, origin + legLen*axes[i]},
                        color,
                        legThickness,
                        appendToRv
                    );
                }
            }

            return rv;
        }

        // handles and state changes that occur as a result of user interaction
        void handleInputAndHoverEvents(
            osc::ImGuiItemHittestResult const& htResult,
            std::optional<IDedLocation> const& maybeHoveredLandmark)
        {
            // event: if the user left-clicks while hovering a landmark...
            if (htResult.isLeftClickReleasedWithoutDragging && maybeHoveredLandmark)
            {
                if (maybeHoveredLandmark->id == m_OriginLandmark.id)
                {
                    // ...and the landmark was the origin, do nothing (they can't (de)select the origin).
                    ;
                }
                else
                {
                    // ...else, if the landmark wasn't the origin...
                    if (m_FirstLandmark && m_FirstLandmark->id == maybeHoveredLandmark->id)
                    {
                        // ...and it was the first landmark, deselect it.
                        m_FirstLandmark.reset();
                    }
                    else if (m_SecondLandmark && m_SecondLandmark->id == maybeHoveredLandmark->id)
                    {
                        // ...and it was the second landmark, deselect it.
                        m_SecondLandmark.reset();
                    }
                    else if (!m_FirstLandmark)
                    {
                        // ...and the first landmark is assignable, then assign it.
                        m_FirstLandmark = maybeHoveredLandmark;
                    }
                    else if (!m_SecondLandmark)
                    {
                        // ...and the second landmark is assignable, then assign it.
                        m_SecondLandmark = maybeHoveredLandmark;
                    }
                    else
                    {
                        // ...and both landmarks are assigned, do nothing (enough landmarks have already been assigned).
                        ;
                    }
                }
            }
        }

        // draws 2D ImGui overlays over the scene render
        void drawOverlays(osc::Rect const& renderRect)
        {
            ImGui::SetCursorScreenPos(renderRect.p1 + c_OverlayPadding);

            ImGui::Text("select reference points (click again to de-select)");
            ImGui::Checkbox("Flip First Edge", &m_FlipFirstEdge);
            ImGui::Checkbox("Flip Plane Normal", &m_FlipSecondEdge);

            if (m_FirstLandmark && m_SecondLandmark)
            {

                if (ImGui::Button("Swap Edges"))
                {
                    std::swap(m_FirstLandmark, m_SecondLandmark);
                }

                if (ImGui::Button("Finish"))
                {
                    osc::log::info("TODO: implement adding the frame to the scene");
                }
            }

            drawEdgeMappingTable();

            if (ImGui::Button("Cancel"))
            {
                requestClose();
            }
        }

        // draws a table that lets the user change how each computed edge maps to the resultant axes
        void drawEdgeMappingTable()
        {
            if (ImGui::BeginTable("##axismappings", 4, ImGuiTableFlags_SizingStretchSame, {0.15f*ImGui::GetContentRegionAvail().x, 0.0f}))
            {
                ImGui::TableSetupColumn("",ImGuiTableColumnFlags_NoSort);
                ImGui::TableSetupColumn("X");
                ImGui::TableSetupColumn("Y");
                ImGui::TableSetupColumn("Z");

                ImGui::TableHeadersRow();

                // each row is an edge
                for  (int edge = 0; edge < 3; ++edge)
                {
                    ImGui::PushID(edge);
                    int const activeAxis = m_EdgeIndexToAxisIndex[edge];

                    ImGui::TableNextRow();

                    // first column labels which edge the row refers to
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("E%i", edge);

                    // remaining columns are for each axis
                    for (int axis = 0; axis < 3; ++axis)
                    {
                        ImGui::PushID(axis);

                        ImGui::TableSetColumnIndex(axis + 1);

                        bool isActive = axis == activeAxis;
                        if (ImGui::RadioButton("##AxisSelector", isActive))
                        {
                            if (!isActive)
                            {
                                auto it = std::find(m_EdgeIndexToAxisIndex.begin(), m_EdgeIndexToAxisIndex.end(), axis);
                                if (it != m_EdgeIndexToAxisIndex.end())
                                {
                                    std::swap(m_EdgeIndexToAxisIndex[edge], *it);
                                }
                            }
                        }

                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        std::shared_ptr<TPSTabSharedState> m_State;
        osc::PolarPerspectiveCamera m_Camera;
        IDedLocation m_OriginLandmark;
        std::optional<IDedLocation> m_FirstLandmark;
        std::optional<IDedLocation> m_SecondLandmark;
        bool m_FlipFirstEdge;
        bool m_FlipSecondEdge;
        std::array<int, 3> m_EdgeIndexToAxisIndex;
        osc::CachedSceneRenderer m_CachedRenderer;
        bool m_WireframeMode;
        float m_LandmarkRadius;
    };
}


// TPS3D UI panel implementations
//
// implementation code for each panel shown in the UI
namespace
{
    // generic base class for the panels shown in the TPS3D tab
    class TPS3DTabPanel : public osc::StandardPanel {
    public:
        using osc::StandardPanel::StandardPanel;

    private:
        void implBeforeImGuiBegin() final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void implAfterImGuiBegin() final
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
        void implDrawContent() final
        {
            // compute top-level UI variables (render rect, mouse pos, etc.)
            osc::Rect const contentRect = osc::ContentRegionAvailScreenRect();
            glm::vec2 const contentRectDims = osc::Dimensions(contentRect);
            glm::vec2 const mousePos = ImGui::GetMousePos();
            osc::Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, osc::Dimensions(contentRect));

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            osc::Mesh const& inputMesh = GetScratchMesh(*m_State, m_DocumentIdentifier);
            std::optional<osc::RayCollision> const meshCollision = m_LastTextureHittestResult.isHovered ?
                osc::GetClosestWorldspaceRayCollision(inputMesh, osc::Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<TPSUIViewportHover> landmarkCollision = m_LastTextureHittestResult.isHovered ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // hover state: update central hover state
            if (landmarkCollision)
            {
                // update central state to tell it that there's a new hover
                m_State->currentHover = landmarkCollision;
            }
            else if (meshCollision)
            {
                m_State->currentHover.emplace(meshCollision->position);
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

            // ensure any popup overlays have latest render rect
            if (auto overlay = m_MaybeActiveModalOverlay.lock())
            {
                overlay->setRect(contentRect);
            }
        }

        void updateCamera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            if (m_State->linkCameras && m_Camera != m_State->linkedCameraBase)
            {
                if (m_State->onlyLinkRotation)
                {
                    m_Camera.phi = m_State->linkedCameraBase.phi;
                    m_Camera.theta = m_State->linkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->linkedCameraBase;
                }
            }

            // if the user interacts with the render, update the camera as necessary
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(osc::Dimensions(m_LastTextureHittestResult.rect), m_Camera))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<TPSUIViewportHover> getMouseLandmarkCollisions(osc::Line const& cameraRay) const
        {
            std::optional<TPSUIViewportHover> rv;
            for (TPSDocumentLandmarkPair const& p : GetScratch(*m_State).landmarkPairs)
            {
                std::optional<glm::vec3> const maybePos = GetLocation(p, m_DocumentIdentifier);

                if (!maybePos)
                {
                    // doesn't have a source/destination landmark
                    continue;
                }
                // else: hittest the landmark as a sphere

                std::optional<osc::RayCollision> const coll = osc::GetRayCollisionSphere(cameraRay, osc::Sphere{*maybePos, m_LandmarkRadius});
                if (coll)
                {
                    if (!rv || glm::length(rv->worldspaceLocation - cameraRay.origin) > coll->distance)
                    {
                        TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, p.id};
                        rv.emplace(std::move(fullID), *maybePos);
                    }
                }
            }
            return rv;
        }

        void handleInputAndHoverEvents(
            osc::ImGuiItemHittestResult const& htResult,
            std::optional<osc::RayCollision> const& meshCollision,
            std::optional<TPSUIViewportHover> const& landmarkCollision)
        {
            // event: if the user left-clicks and something is hovered, select it; otherwise, add a landmark
            if (htResult.isLeftClickReleasedWithoutDragging)
            {
                if (landmarkCollision && landmarkCollision->maybeSceneElementID)
                {
                    if (!osc::IsShiftDown())
                    {
                        m_State->userSelection.clear();
                    }
                    m_State->userSelection.select(*landmarkCollision->maybeSceneElementID);
                }
                else if (meshCollision)
                {
                    ActionAddLandmarkTo(
                        *m_State->editedDocument,
                        m_DocumentIdentifier,
                        meshCollision->position
                    );
                }
            }

            // event: if the user right-clicks a landmark in the source document, bring up the source frame overlay
            if (htResult.isRightClickReleasedWithoutDragging &&
                m_DocumentIdentifier == TPSDocumentInputIdentifier::Source &&
                landmarkCollision &&
                landmarkCollision->maybeSceneElementID &&
                landmarkCollision->maybeSceneElementID->elementType == TPSDocumentInputElementType::Landmark)
            {
                auto overlay = std::make_shared<TPS3DDefineFramePopup>(
                    m_State,
                    m_Camera,
                    m_WireframeMode,
                    m_LandmarkRadius,
                    IDedLocation{landmarkCollision->maybeSceneElementID->elementID, landmarkCollision->worldspaceLocation}
                );
                overlay->setRect(htResult.rect);
                overlay->open();
                m_State->popupManager.push_back(overlay);
                m_MaybeActiveModalOverlay = overlay;
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (htResult.isHovered && osc::IsAnyKeyPressed({ImGuiKey_Delete, ImGuiKey_Backspace}))
            {
                ActionDeleteSceneElementsByID(
                    *m_State->editedDocument,
                    m_State->userSelection.getUnderlyingSet()
                );
                m_State->userSelection.clear();
            }
        }

        // draws 2D ImGui overlays over the scene render
        void drawOverlays(osc::Rect const& renderRect)
        {
            ImGui::SetCursorScreenPos(renderRect.p1 + c_OverlayPadding);

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
                ImGui::Text("%zu", CountNumLandmarksForInput(GetScratch(*m_State), m_DocumentIdentifier));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# verts");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetScratchMesh(*m_State, m_DocumentIdentifier).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetScratchMesh(*m_State, m_DocumentIdentifier).getIndices().size()/3);

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
                    ActionBrowseForNewMesh(*m_State->editedDocument, m_DocumentIdentifier);
                }
                if (ImGui::MenuItem("Landmarks from CSV"))
                {
                    ActionLoadLandmarksCSV(*m_State->editedDocument, m_DocumentIdentifier);
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
                    ActionTrySaveMeshToObj(GetScratchMesh(*m_State, m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(GetScratchMesh(*m_State, m_DocumentIdentifier));
                }
                if (ImGui::MenuItem("Landmarks to CSV"))
                {
                    ActionSaveLandmarksToCSV(GetScratch(*m_State), m_DocumentIdentifier);
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, GetScratchMesh(*m_State, m_DocumentIdentifier).getBounds(), osc::AspectRatio(m_LastTextureHittestResult.rect));
                m_State->linkedCameraBase = m_Camera;
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
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - c_OverlayPadding.x);
            ImGui::SliderFloat(label, &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        // renders this panel's 3D scene to a texture
        osc::RenderTexture& renderScene(
            glm::vec2 dims,
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSUIViewportHover> const& maybeLandmarkCollision)
        {
            osc::SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                osc::App::get().getMSXAASamplesRecommended(),
                dims
            );
            std::vector<osc::SceneDecoration> const decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.draw(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<osc::SceneDecoration> generateDecorations(
            std::optional<osc::RayCollision> const& maybeMeshCollision,
            std::optional<TPSUIViewportHover> const& maybeLandmarkCollision) const
        {
            // generate in-scene 3D decorations
            std::vector<osc::SceneDecoration> decorations;
            decorations.reserve(6 + CountNumLandmarksForInput(GetScratch(*m_State), m_DocumentIdentifier));  // likely guess

            AppendCommonDecorations(
                *m_State,
                GetScratchMesh(*m_State, m_DocumentIdentifier),
                m_WireframeMode,
                [&decorations](osc::SceneDecoration&& dec) { decorations.push_back(std::move(dec)); }
            );

            // append each landmark as a sphere
            for (TPSDocumentLandmarkPair const& p : GetScratch(*m_State).landmarkPairs)
            {
                std::optional<glm::vec3> const maybeLocation = GetLocation(p, m_DocumentIdentifier);

                if (!maybeLocation)
                {
                    continue;  // no source/destination location for the landmark
                }

                TPSDocumentElementID fullID{m_DocumentIdentifier, TPSDocumentInputElementType::Landmark, p.id};

                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = *maybeLocation;

                osc::Color const& color = IsFullyPaired(p) ? c_PairedLandmarkColor : c_UnpairedLandmarkColor;

                osc::SceneDecoration& decoration = decorations.emplace_back(m_State->landmarkSphere, transform, color);

                if (m_State->userSelection.contains(fullID))
                {
                    glm::vec4 tmpColor = decoration.color;
                    tmpColor += glm::vec4{0.25f, 0.25f, 0.25f, 0.0f};
                    tmpColor = glm::clamp(tmpColor, glm::vec4{0.0f}, glm::vec4{1.0f});

                    decoration.color = osc::Color{tmpColor};
                    decoration.flags = osc::SceneDecorationFlags_IsSelected;
                }
                else if (m_State->currentHover && m_State->currentHover->maybeSceneElementID == fullID)
                {
                    glm::vec4 tmpColor = decoration.color; 
                    tmpColor += glm::vec4{0.15f, 0.15f, 0.15f, 0.0f};
                    tmpColor = glm::clamp(tmpColor, glm::vec4{0.0f}, glm::vec4{1.0f});

                    decoration.color = osc::Color{tmpColor};
                    decoration.flags = osc::SceneDecorationFlags_IsHovered;
                }
            }

            // if applicable, show mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = maybeMeshCollision->position;

                osc::Color color = c_UnpairedLandmarkColor;
                color.a *= 0.25f;

                decorations.emplace_back(m_State->landmarkSphere, transform, color);
            }

            return decorations;
        }

        std::shared_ptr<TPSTabSharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetScratchMesh(*m_State, m_DocumentIdentifier).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer{osc::App::config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()};
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
        std::weak_ptr<TPS3DDefineFramePopup> m_MaybeActiveModalOverlay;
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class TPS3DResultPanel final : public TPS3DTabPanel {
    public:

        TPS3DResultPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSTabSharedState> state_) :

            TPS3DTabPanel{std::move(panelName_)},
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid sharedState state");
        }

    private:
        void implDrawContent() final
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
            if (m_State->linkCameras && m_Camera != m_State->linkedCameraBase)
            {
                if (m_State->onlyLinkRotation)
                {
                    m_Camera.phi = m_State->linkedCameraBase.phi;
                    m_Camera.theta = m_State->linkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->linkedCameraBase;
                }
            }

            // update camera if user drags it around etc.
            if (m_LastTextureHittestResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(osc::Dimensions(m_LastTextureHittestResult.rect), m_Camera))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
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
                ImGui::Text("%zu", GetResultMesh(*m_State).getVerts().size());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("# triangles");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", GetResultMesh(*m_State).getIndices().size()/3);

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
                    ActionTrySaveMeshToObj(GetResultMesh(*m_State));
                }
                if (ImGui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStl(GetResultMesh(*m_State));
                }
                ImGui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                osc::AutoFocus(m_Camera, GetResultMesh(*m_State).getBounds(), AspectRatio(m_LastTextureHittestResult.rect));
                m_State->linkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
        }

        void drawBlendingFactorSlider()
        {
            char const* const label = "blending factor";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - m_OverlayPadding.x);
            float factor = GetScratch(*m_State).blendingFactor;

            if (ImGui::SliderFloat(label, &factor, 0.0f, 1.0f))
            {
                ActionSetBlendFactorWithoutSaving(*m_State->editedDocument, factor);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                ActionSetBlendFactorAndSave(*m_State->editedDocument, factor);
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<osc::SceneDecoration> generateDecorations() const
        {
            std::vector<osc::SceneDecoration> decorations;

            AppendCommonDecorations(
                *m_State,
                GetResultMesh(*m_State),
                m_WireframeMode,
                [&decorations](osc::SceneDecoration&& dec) { decorations.push_back(std::move(dec)); }
            );

            if (m_ShowDestinationMesh)
            {
                osc::SceneDecoration& dec = decorations.emplace_back(GetScratch(*m_State).destinationMesh);
                dec.color = {1.0f, 0.0f, 0.0f, 0.5f};
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered texture
        osc::RenderTexture& renderScene(glm::vec2 dims)
        {
            std::vector<osc::SceneDecoration> const decorations = generateDecorations();
            osc::SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                osc::App::get().getMSXAASamplesRecommended(),
                dims
            );
            return m_CachedRenderer.draw(decorations, params);
        }

        std::shared_ptr<TPSTabSharedState> m_State;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetResultMesh(*m_State).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            *osc::App::singleton<osc::MeshCache>(),
            *osc::App::singleton<osc::ShaderCache>()
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        glm::vec2 m_OverlayPadding = {10.0f, 10.0f};
    };

    // pushes all available panels the TPS3D tab can render into the out param
    void PushBackAvailablePanels(std::shared_ptr<TPSTabSharedState> const& state, osc::PanelManager& out)
    {
        out.registerToggleablePanel(
            "Source Mesh",
            [state](std::string_view panelName) { return std::make_shared<TPS3DInputPanel>(panelName, state, TPSDocumentInputIdentifier::Source); }
        );

        out.registerToggleablePanel(
            "Destination Mesh",
            [state](std::string_view panelName) { return std::make_shared<TPS3DInputPanel>(panelName, state, TPSDocumentInputIdentifier::Destination); }
        );

        out.registerToggleablePanel(
            "Result",
            [state](std::string_view panelName) { return std::make_shared<TPS3DResultPanel>(panelName, state); }
        );

        out.registerToggleablePanel(
            "History",
            [state](std::string_view panelName) { return std::make_shared<osc::UndoRedoPanel>(panelName, state->editedDocument); },
            osc::ToggleablePanelFlags_Default & ~osc::ToggleablePanelFlags_IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Log",
            [](std::string_view panelName) { return std::make_shared<osc::LogViewerPanel>(panelName); },
            osc::ToggleablePanelFlags_Default & ~osc::ToggleablePanelFlags_IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Performance",
            [](std::string_view panelName) { return std::make_shared<osc::PerfPanel>(panelName); },
            osc::ToggleablePanelFlags_Default & ~osc::ToggleablePanelFlags_IsEnabledByDefault
        );
    }
}

// top-level tab implementation
class osc::TPS3DTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) : m_Parent{std::move(parent_)}
    {
        OSC_ASSERT(m_Parent.lock() != nullptr);

        // initialize panels
        PushBackAvailablePanels(m_SharedState, *m_SharedState->panelManager);
        m_SharedState->panelManager->activateAllDefaultOpenPanels();
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " TPS3DTab";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    void onTick()
    {
        // re-perform hover test each frame
        m_SharedState->currentHover.reset();

        // garbage collect panel data
        m_SharedState->panelManager->garbageCollectDeactivatedPanels();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.draw();
        m_SharedState->panelManager->drawAllActivatedPanels();
        m_StatusBar.draw();

        // draw active popups over the UI
        m_SharedState->popupManager.draw();
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<TPSTabSharedState> m_SharedState = std::make_shared<TPSTabSharedState>(m_TabID, m_Parent);

    // not-user-toggleable widgets
    TPS3DMainMenu m_MainMenu{m_SharedState};
    TPS3DToolbar m_TopToolbar{"##TPS3DToolbar", m_SharedState};
    TPS3DStatusBar m_StatusBar{"##TPS3DStatusBar", m_SharedState};
};


// public API (PIMPL)

osc::CStringView osc::TPS3DTab::id() noexcept
{
    return "OpenSim/Experimental/TPS3D";
}

osc::TPS3DTab::TPS3DTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
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

void osc::TPS3DTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::TPS3DTab::implOnUnmount()
{
    m_Impl->onUnmount();
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
