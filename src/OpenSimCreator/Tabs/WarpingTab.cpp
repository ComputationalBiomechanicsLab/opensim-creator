#include "WarpingTab.hpp"

#include "OpenSimCreator/Utils/TPS3D.hpp"
#include "OpenSimCreator/Bindings/SimTKMeshLoader.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Bindings/GlmHelpers.hpp>
#include <oscar/Formats/CSV.hpp>
#include <oscar/Formats/OBJ.hpp>
#include <oscar/Formats/STL.hpp>
#include <oscar/Graphics/CachedSceneRenderer.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/MeshGen.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/CollisionTests.hpp>
#include <oscar/Maths/Constants.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Panels/LogViewerPanel.hpp>
#include <oscar/Panels/Panel.hpp>
#include <oscar/Panels/PanelManager.hpp>
#include <oscar/Panels/PerfPanel.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Panels/ToggleablePanelFlags.hpp>
#include <oscar/Panels/UndoRedoPanel.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Tabs/TabHost.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/ScopeGuard.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Utils/UndoRedo.hpp>
#include <oscar/Widgets/Popup.hpp>
#include <oscar/Widgets/PopupManager.hpp>
#include <oscar/Widgets/RedoButton.hpp>
#include <oscar/Widgets/StandardPopup.hpp>
#include <oscar/Widgets/UndoButton.hpp>
#include <oscar/Widgets/WindowMenu.hpp>

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
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// constants
namespace
{
    constexpr glm::vec2 c_OverlayPadding = {10.0f, 10.0f};
    constexpr osc::Color c_PairedLandmarkColor = osc::Color::green();
    constexpr osc::Color c_UnpairedLandmarkColor = osc::Color::red();
}

// Thin-Plate Spline (TPS) document datastructure
//
// the core datastructures that the user edits via the UI
namespace
{
    // identifies one of the two inputs (source/destination) of the TPS document
    enum class TPSDocumentInputIdentifier {
        Source = 0,
        Destination,
        NUM_OPTIONS,
    };

    // identifies a specific part of the input of the TPS document
    enum class TPSDocumentInputElementType {
        Landmark = 0,
        Mesh,
        NUM_OPTIONS,
    };

    // a landmark pair in the TPS document (might be midway through definition)
    struct TPSDocumentLandmarkPair final {

        explicit TPSDocumentLandmarkPair(std::string id_) :
            id{std::move(id_)}
        {
        }

        std::string id;
        std::optional<glm::vec3> maybeSourceLocation;
        std::optional<glm::vec3> maybeDestinationLocation;
    };

    // a TPS document
    //
    // a central datastructure that the user edits in-place via the UI
    struct TPSDocument final {
        osc::Mesh sourceMesh = osc::GenUntexturedUVSphere(16, 16);
        osc::Mesh destinationMesh = osc::GenUntexturedYToYCylinder(16);
        std::vector<TPSDocumentLandmarkPair> landmarkPairs;
        float blendingFactor = 1.0f;
        size_t nextLandmarkID = 0;
    };

    // an associative identifier that points to a specific part of a TPS document
    //
    // (handy for selection logic etc.)
    struct TPSDocumentElementID final {

        TPSDocumentElementID(
            TPSDocumentInputIdentifier whichInput_,
            TPSDocumentInputElementType elementType_,
            std::string elementID_) :

            whichInput{whichInput_},
            elementType{elementType_},
            elementID{std::move(elementID_)}
        {
        }

        TPSDocumentInputIdentifier whichInput;
        TPSDocumentInputElementType elementType;
        std::string elementID;
    };

    bool operator==(TPSDocumentElementID const& a, TPSDocumentElementID const& b) noexcept
    {
        return
            a.whichInput == b.whichInput &&
            a.elementType == b.elementType &&
            a.elementID == b.elementID;
    }
}

namespace std
{
    template<>
    struct hash<TPSDocumentElementID> {
        size_t operator()(TPSDocumentElementID const& el) const noexcept
        {
            return osc::HashOf(el.whichInput, el.elementType, el.elementID);
        }
    };
}

// TPS document helper functions
namespace
{
    // returns the (mutable) source/destination of the given landmark pair, if available
    std::optional<glm::vec3>& UpdLocation(TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(osc::NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns the source/destination of the given landmark pair, if available
    std::optional<glm::vec3> const& GetLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(osc::NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    bool HasLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        return GetLocation(landmarkPair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    osc::Mesh& UpdMesh(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(osc::NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns the source/destination mesh in the given document
    osc::Mesh const& GetMesh(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(osc::NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    bool IsFullyPaired(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    bool HasSourceOrDestinationLocation(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<osc::LandmarkPair3D> TryExtractLandmarkPair(TPSDocumentLandmarkPair const& p)
    {
        if (IsFullyPaired(p))
        {
            return osc::LandmarkPair3D{*p.maybeSourceLocation, *p.maybeDestinationLocation};
        }
        else
        {
            return std::nullopt;
        }
    }

    // returns all fully paired landmarks in `doc`
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

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        auto const hasLocation = [which](TPSDocumentLandmarkPair const& p) { return HasLocation(p, which); };
        return std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), hasLocation);
    }

    // returns the next available (presumably, unique) landmark ID
    std::string NextLandmarkID(TPSDocument& doc)
    {
        std::stringstream ss;
        ss << "landmark_" << doc.nextLandmarkID++;
        return std::move(ss).str();
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
            TPSDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(NextLandmarkID(doc));
            UpdLocation(p, which) = pos;
        }
    }
}

// user-enactable actions
namespace
{
    // if possible, undos the document to the last change
    void ActionUndo(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.undo();
    }

    // if possible, redos the document to the last undone change
    void ActionRedo(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.redo();
    }

    // adds a landmark to the source mesh
    void ActionAddLandmarkTo(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentInputIdentifier which,
        glm::vec3 const& pos)
    {
        AddLandmarkToInput(doc.updScratch(), which, pos);
        doc.commitScratch("added landmark");
    }

    // prompts the user to browse for an input mesh and assigns it to the document
    void ActionBrowseForNewMesh(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeMeshPath =
            osc::PromptUserForFile(osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }

        UpdMesh(doc.updScratch(), which) = osc::LoadMeshViaSimTK(*maybeMeshPath);

        doc.commitScratch("changed mesh");
    }

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionLoadLandmarksCSV(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFile("csv");
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

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutSaving(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        doc.updScratch().blendingFactor = factor;
    }

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactorAndSave(osc::UndoRedoT<TPSDocument>& doc, float factor)
    {
        ActionSetBlendFactorWithoutSaving(doc, factor);
        doc.commitScratch("changed blend factor");
    }

    // creates a "fresh" (default) TPS document
    void ActionCreateNewDocument(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch() = TPSDocument{};
        doc.commitScratch("created new document");
    }

    // clears all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch().landmarkPairs.clear();
        doc.commitScratch("cleared all landmarks");
    }

    // deletes the specified landmarks from the TPS document
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
                auto const pairHasID = [&id](TPSDocumentLandmarkPair const& p) { return p.id == id.elementID; };
                auto const it = std::find_if(
                    scratch.landmarkPairs.begin(),
                    scratch.landmarkPairs.end(),
                    pairHasID
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

    // saves all source/destination landmarks to a simple headerless CSV file (matches loading)
    void ActionSaveLandmarksToCSV(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream fileOutputStream{*maybeCSVPath};
        if (!fileOutputStream)
        {
            return;  // couldn't open file for writing
        }

        for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
        {
            if (std::optional<glm::vec3> const loc = GetLocation(p, which))
            {
                osc::WriteCSVRow(
                    fileOutputStream,
                    osc::to_array(
                    {
                        std::to_string(loc->x),
                        std::to_string(loc->y),
                        std::to_string(loc->z),
                    })
                );
            }
        }
    }

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSaveLandmarksToPairedCSV(TPSDocument const& doc)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream fileOutputStream{*maybeCSVPath};
        if (!fileOutputStream)
        {
            return;  // couldn't open file for writing
        }

        std::vector<osc::LandmarkPair3D> const pairs = GetLandmarkPairs(doc);

        // write header
        osc::WriteCSVRow(
            fileOutputStream,
            osc::to_array<std::string>(
            {
                "source.x",
                "source.y",
                "source.z",
                "dest.x",
                "dest.y",
                "dest.z",
            })
        );

        // write data rows
        for (osc::LandmarkPair3D const& p : pairs)
        {
            osc::WriteCSVRow(
                fileOutputStream,
                osc::to_array(
                {
                    std::to_string(p.source.x),
                    std::to_string(p.source.y),
                    std::to_string(p.source.z),

                    std::to_string(p.destination.x),
                    std::to_string(p.destination.y),
                    std::to_string(p.destination.z),
                })
            );
        }
    }

    // prompts the user to save the mesh to an obj file
    void ActionTrySaveMeshToObj(osc::Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeSavePath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");
        if (!maybeSavePath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outputFileStream
        {
            *maybeSavePath,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary
        };
        if (!outputFileStream)
        {
            return;  // couldn't open for writing
        }

        osc::WriteMeshAsObj(
            outputFileStream,
            mesh,
            osc::ObjWriterFlags::NoWriteNormals  // warping might have screwed them
        );
    }

    // prompts the user to save the mesh to an stl file
    void ActionTrySaveMeshToStl(osc::Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeSTLPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");
        if (!maybeSTLPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outputFileStream
        {
            *maybeSTLPath,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            return;  // couldn't open for writing
        }

        osc::WriteMeshAsStl(outputFileStream, mesh);
    }
}

// TPS result cache
//
// caches the result of an (expensive) TPS warp of the mesh by checking
// whether the input arguments have changed
namespace
{
    // a cache for TPS mesh warping results
    class TPSResultCache final {
    public:
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

// UI: top-level datastructures that are shared between panels etc.
namespace
{
    // a mouse hovertest result
    struct TPSUIViewportHover final {

        explicit TPSUIViewportHover(glm::vec3 const& worldspaceLocation_) :
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

        std::optional<TPSDocumentElementID> maybeSceneElementID = std::nullopt;
        glm::vec3 worldspaceLocation;
    };

    // the user's current selection
    struct TPSUIUserSelection final {

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

    // top-level UI state that is shared by all UI panels
    struct TPSUISharedState final {

        TPSUISharedState(
            osc::UID tabID_,
            std::weak_ptr<osc::TabHost> parent_) :

            tabID{tabID_},
            tabHost{parent_.lock()}
        {
            OSC_ASSERT(tabHost != nullptr && "top-level tab host required for this UI");
        }

        // ID of the top-level TPS3D tab
        osc::UID tabID;

        // handle to the screen that owns the TPS3D tab
        std::shared_ptr<osc::TabHost> tabHost;

        // cached TPS3D algorithm result (to prevent recomputing it each frame)
        TPSResultCache meshResultCache;

        // the document that the user is editing
        std::shared_ptr<osc::UndoRedoT<TPSDocument>> editedDocument = std::make_shared<osc::UndoRedoT<TPSDocument>>();

        // `true` if the user wants the cameras to be linked
        bool linkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool onlyLinkRotation = false;

        // shared linked camera
        osc::PolarPerspectiveCamera linkedCameraBase = CreateCameraFocusedOn(editedDocument->getScratch().sourceMesh.getBounds());

        // wireframe material, used to draw scene elements in a wireframe style
        osc::Material wireframeMaterial = osc::CreateWireframeOverlayMaterial(
            osc::App::config(),
            *osc::App::singleton<osc::ShaderCache>()
        );

        // shared sphere mesh (used by rendering code)
        osc::Mesh landmarkSphere = osc::App::singleton<osc::MeshCache>()->getSphereMesh();

        // current user selection
        TPSUIUserSelection userSelection;

        // current user hover: reset per-frame
        std::optional<TPSUIViewportHover> currentHover;

        // available/active panels that the user can toggle via the `window` menu
        std::shared_ptr<osc::PanelManager> panelManager = std::make_shared<osc::PanelManager>();

        // currently active tab-wide popups
        osc::PopupManager popupManager;

        // shared mesh cache
        std::shared_ptr<osc::MeshCache> meshCache = osc::App::singleton<osc::MeshCache>();
    };

    TPSDocument const& GetScratch(TPSUISharedState const& state)
    {
        return state.editedDocument->getScratch();
    }

    osc::Mesh const& GetScratchMesh(TPSUISharedState& state, TPSDocumentInputIdentifier which)
    {
        return GetMesh(GetScratch(state), which);
    }

    // returns a (potentially cached) post-TPS-warp mesh
    osc::Mesh const& GetResultMesh(TPSUISharedState& state)
    {
        return state.meshResultCache.lookup(state.editedDocument->getScratch());
    }

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSUISharedState const& sharedState,
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
        DrawXZGrid(*sharedState.meshCache, out);
        DrawXZFloorLines(*sharedState.meshCache, out, 100.0f);
    }
}


// UI: widgets that appear within panels in the UI
namespace
{
    // the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class TPS3DToolbar final {
    public:
        TPS3DToolbar(
            std::string_view label,
            std::shared_ptr<TPSUISharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (osc::BeginToolbar(m_Label))
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
            m_UndoButton.onDraw();
            ImGui::SameLine();
            m_RedoButton.onDraw();
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
            osc::DrawTooltipIfItemHovered(
                "Create New Document",
                "Creates the default scene (undoable)"
            );
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
            osc::DrawTooltipIfItemHovered(
                "Open File",
                "Open Source/Destination data"
            );
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSaveLandmarksToPairedCSV(GetScratch(*m_State));
            }
            osc::DrawTooltipIfItemHovered(
                "Save Landmarks to CSV",
                "Saves all pair-able landmarks to a CSV file, for external processing"
            );
        }

        void drawCameraLockCheckbox()
        {
            ImGui::Checkbox("link cameras", &m_State->linkCameras);
            ImGui::SameLine();
            ImGui::Checkbox("only link rotation", &m_State->onlyLinkRotation);
        }

        void drawResetLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(*m_State->editedDocument);
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSUISharedState> m_State;
        osc::UndoButton m_UndoButton{m_State->editedDocument};
        osc::RedoButton m_RedoButton{m_State->editedDocument};
    };

    // widget: bottom status bar (shows status messages, hover information, etc.)
    class TPS3DStatusBar final {
    public:
        TPS3DStatusBar(
            std::string_view label,
            std::shared_ptr<TPSUISharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
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
                drawCurrentHoverInfo(*m_State->currentHover);
            }
            else
            {
                ImGui::TextDisabled("(nothing hovered)");
            }
        }

        void drawCurrentHoverInfo(TPSUIViewportHover const& hover)
        {
            drawColorCodedXYZ(hover.worldspaceLocation);
            ImGui::SameLine();
            if (hover.maybeSceneElementID)
            {
                ImGui::TextDisabled("(left-click to select %s)", hover.maybeSceneElementID->elementID.c_str());
            }
            else
            {
                ImGui::TextDisabled("(left-click to add a landmark)");
            }
        }

        void drawColorCodedXYZ(glm::vec3 pos)
        {
            ImGui::TextUnformatted("(");
            ImGui::SameLine();
            for (int i = 0; i < 3; ++i)
            {
                osc::Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                color[i] = 1.0f;

                osc::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%f", pos[i]);
                ImGui::SameLine();
                osc::PopStyleColor();
            }
            ImGui::TextUnformatted(")");
        }

        std::string m_Label;
        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the 'file' menu (a sub menu of the main menu)
    class TPS3DFileMenu final {
    public:
        explicit TPS3DFileMenu(std::shared_ptr<TPSUISharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
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
                m_State->tabHost->closeTab(m_State->tabID);
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

        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the 'edit' menu (a sub menu of the main menu)
    class TPS3DEditMenu final {
    public:
        explicit TPS3DEditMenu(std::shared_ptr<TPSUISharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
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

        std::shared_ptr<TPSUISharedState> m_State;
    };

    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class TPS3DMainMenu final {
    public:
        explicit TPS3DMainMenu(std::shared_ptr<TPSUISharedState> const& tabState_) :
            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_WindowMenu{tabState_->panelManager}
        {
        }

        void onDraw()
        {
            m_FileMenu.onDraw();
            m_EditMenu.onDraw();
            m_WindowMenu.onDraw();
            m_AboutTab.onDraw();
        }
    private:
        TPS3DFileMenu m_FileMenu;
        TPS3DEditMenu m_EditMenu;
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutTab;
    };
}

// TPS3D UI panel implementations
//
// implementation code for each panel shown in the UI
namespace
{
    // generic base class for the panels shown in the TPS3D tab
    class WarpingTabPanel : public osc::StandardPanel {
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
    class TPS3DInputPanel final : public WarpingTabPanel {
    public:
        TPS3DInputPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSUISharedState> state_,
            TPSDocumentInputIdentifier documentIdentifier_) :

            WarpingTabPanel{panelName_, ImGuiDockNodeFlags_PassthruCentralNode},
            m_State{std::move(state_)},
            m_DocumentIdentifier{documentIdentifier_}
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
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, osc::Dimensions(m_LastTextureHittestResult.rect)))
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
            osc::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ImGui::IsItemHovered())
            {
                osc::BeginTooltip();

                ImGui::TextDisabled("Input Information:");
                drawInformationTable();

                osc::EndTooltip();
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
            return m_CachedRenderer.render(decorations, params);
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
                    decoration.flags = osc::SceneDecorationFlags::IsSelected;
                }
                else if (m_State->currentHover && m_State->currentHover->maybeSceneElementID == fullID)
                {
                    glm::vec4 tmpColor = decoration.color;
                    tmpColor += glm::vec4{0.15f, 0.15f, 0.15f, 0.0f};
                    tmpColor = glm::clamp(tmpColor, glm::vec4{0.0f}, glm::vec4{1.0f});

                    decoration.color = osc::Color{tmpColor};
                    decoration.flags = osc::SceneDecorationFlags::IsHovered;
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

        std::shared_ptr<TPSUISharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetScratchMesh(*m_State, m_DocumentIdentifier).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            *osc::App::singleton<osc::MeshCache>(),
            *osc::App::singleton<osc::ShaderCache>(),
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class TPS3DResultPanel final : public WarpingTabPanel {
    public:

        TPS3DResultPanel(
            std::string_view panelName_,
            std::shared_ptr<TPSUISharedState> state_) :

            WarpingTabPanel{panelName_},
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
                if (osc::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, osc::Dimensions(m_LastTextureHittestResult.rect)))
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
            ImGui::Checkbox("show destination", &m_ShowDestinationMesh);
            ImGui::SameLine();
            drawBlendingFactorSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            osc::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ImGui::IsItemHovered())
            {
                osc::BeginTooltip();

                ImGui::TextDisabled("Result Information:");
                drawInformationTable();

                osc::EndTooltip();
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
                osc::AutoFocus(
                    m_Camera,
                    GetResultMesh(*m_State).getBounds(),
                    AspectRatio(m_LastTextureHittestResult.rect)
                );
                m_State->linkedCameraBase = m_Camera;
            }
            osc::DrawTooltipIfItemHovered(
                "Autoscale Scene",
                "Zooms camera to try and fit everything in the scene into the viewer"
            );
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
            return m_CachedRenderer.render(decorations, params);
        }

        std::shared_ptr<TPSUISharedState> m_State;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(GetResultMesh(*m_State).getBounds());
        osc::CachedSceneRenderer m_CachedRenderer
        {
            osc::App::config(),
            *osc::App::singleton<osc::MeshCache>(),
            *osc::App::singleton<osc::ShaderCache>(),
        };
        osc::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
        glm::vec2 m_OverlayPadding = {10.0f, 10.0f};
    };

    // pushes all available panels the TPS3D tab can render into the out param
    void PushBackAvailablePanels(std::shared_ptr<TPSUISharedState> const& state, osc::PanelManager& out)
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
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Log",
            [](std::string_view panelName) { return std::make_shared<osc::LogViewerPanel>(panelName); },
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );

        out.registerToggleablePanel(
            "Performance",
            [](std::string_view panelName) { return std::make_shared<osc::PerfPanel>(panelName); },
            osc::ToggleablePanelFlags::Default - osc::ToggleablePanelFlags::IsEnabledByDefault
        );
    }
}

// top-level tab implementation
class osc::WarpingTab::Impl final {
public:

    explicit Impl(std::weak_ptr<TabHost> parent_) : m_Parent{std::move(parent_)}
    {
        PushBackAvailablePanels(m_SharedState, *m_SharedState->panelManager);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " Mesh Warping";
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_SharedState->panelManager->onMount();
    }

    void onUnmount()
    {
        m_SharedState->panelManager->onUnmount();
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN)
        {
            return onKeydownEvent(e.key);
        }
        else
        {
            return false;
        }
    }

    void onTick()
    {
        // re-perform hover test each frame
        m_SharedState->currentHover.reset();

        // garbage collect panel data
        m_SharedState->panelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.onDraw();
        m_SharedState->panelManager->onDraw();
        m_StatusBar.onDraw();

        // draw active popups over the UI
        m_SharedState->popupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            ActionRedo(*m_SharedState->editedDocument);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            ActionUndo(*m_SharedState->editedDocument);
            return true;
        }
        else
        {
            return false;
        }
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<TPSUISharedState> m_SharedState = std::make_shared<TPSUISharedState>(m_TabID, m_Parent);

    // not-user-toggleable widgets
    TPS3DMainMenu m_MainMenu{m_SharedState};
    TPS3DToolbar m_TopToolbar{"##TPS3DToolbar", m_SharedState};
    TPS3DStatusBar m_StatusBar{"##TPS3DStatusBar", m_SharedState};
};


// public API (PIMPL)

osc::CStringView osc::WarpingTab::id() noexcept
{
    return "OpenSim/Warping";
}

osc::WarpingTab::WarpingTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::WarpingTab::WarpingTab(WarpingTab&&) noexcept = default;
osc::WarpingTab& osc::WarpingTab::operator=(WarpingTab&&) noexcept = default;
osc::WarpingTab::~WarpingTab() noexcept = default;

osc::UID osc::WarpingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::WarpingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::WarpingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::WarpingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::WarpingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::WarpingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::WarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::WarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}
