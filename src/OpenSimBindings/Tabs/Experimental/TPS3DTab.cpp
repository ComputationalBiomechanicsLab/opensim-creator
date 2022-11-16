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
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
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
#include "src/Widgets/VirtualPanel.hpp"

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

// core 3D TPS algorithm code
//
// most of the background behind this is discussed in issue #467. For redundancy's sake, here
// are some of the references used to write this implementation:
//
// - primary literature source: https://ieeexplore.ieee.org/document/24792
// - blog explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
// - blog explanation #2: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
namespace
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    struct LandmarkPair3D final {

        LandmarkPair3D() = default;

        LandmarkPair3D(glm::vec3 const& src_, glm::vec3 const& dest_) :
            Src{src_},
            Dest{dest_}
        {
        }

        glm::vec3 Src;
        glm::vec3 Dest;
    };

    bool operator==(LandmarkPair3D const& a, LandmarkPair3D const& b) noexcept
    {
        return a.Src == b.Src && a.Dest == b.Dest;
    }

    bool operator!=(LandmarkPair3D const& a, LandmarkPair3D const& b) noexcept
    {
        return !(a == b);
    }

    // pretty-prints `LandmarkPair3D`
    std::ostream& operator<<(std::ostream& o, LandmarkPair3D const& p)
    {
        using osc::operator<<;
        o << "LandmarkPair3D{Src = " << p.Src << ", dest = " << p.Dest << '}';
        return o;
    }

    // required inputs to the 3D TPS algorithm
    //
    // these are supplied by the user and used to solve for the coefficients
    struct TPSCoefficientSolverInputs3D final {

        TPSCoefficientSolverInputs3D() = default;

        TPSCoefficientSolverInputs3D(
            std::vector<LandmarkPair3D> landmarks_,
            float blendingFactor_) :

            Landmarks{std::move(landmarks_)},
            BlendingFactor{std::move(blendingFactor_)}
        {
        }

        std::vector<LandmarkPair3D> Landmarks;
        float BlendingFactor = 1.0f;
    };

    bool operator==(TPSCoefficientSolverInputs3D const& a, TPSCoefficientSolverInputs3D const& b) noexcept
    {
        return a.Landmarks == b.Landmarks && a.BlendingFactor == b.BlendingFactor;
    }

    bool operator!=(TPSCoefficientSolverInputs3D const& a, TPSCoefficientSolverInputs3D const& b) noexcept
    {
        return !(a == b);
    }

    // pretty-prints `TPSCoefficientSolverInputs3D`
    std::ostream& operator<<(std::ostream& o, TPSCoefficientSolverInputs3D const& inputs)
    {
        o << "TPSCoefficientSolverInputs3D{landmarks = [";
        std::string_view delim = "";
        for (LandmarkPair3D const& landmark : inputs.Landmarks)
        {
            o << delim << landmark;
            delim = ", ";
        }
        o << "], BlendingFactor = " << inputs.BlendingFactor << '}';
        return o;
    }

    // a single non-affine term of the 3D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    struct TPSNonAffineTerm3D final {
        TPSNonAffineTerm3D(
            glm::vec3 const& weight_,
            glm::vec3 const& controlPoint_) :

            Weight{weight_},
            ControlPoint{controlPoint_}
        {
        }

        glm::vec3 Weight;
        glm::vec3 ControlPoint;
    };

    bool operator==(TPSNonAffineTerm3D const& a, TPSNonAffineTerm3D const& b) noexcept
    {
        return a.Weight == b.Weight && a.ControlPoint == b.ControlPoint;
    }

    bool operator!=(TPSNonAffineTerm3D const& a, TPSNonAffineTerm3D const& b) noexcept
    {
        return !(a == b);
    }

    // pretty-prints `TPSNonAffineTerm3D`
    std::ostream& operator<<(std::ostream& o, TPSNonAffineTerm3D const& wt)
    {
        using osc::operator<<;
        return o << "TPSNonAffineTerm3D{Weight = " << wt.Weight << ", ControlPoint = " << wt.ControlPoint << '}';
    }

    // all coefficients in the 3D TPS equation
    //
    // i.e. these are the a1, a2, a3, a4, and w's (+ control points) terms of the equation
    struct TPSCoefficients3D final {

        // default the coefficients to an "identity" warp
        glm::vec3 a1 = {0.0f, 0.0f, 0.0f};
        glm::vec3 a2 = {1.0f, 0.0f, 0.0f};
        glm::vec3 a3 = {0.0f, 1.0f, 0.0f};
        glm::vec3 a4 = {0.0f, 0.0f, 1.0f};
        std::vector<TPSNonAffineTerm3D> NonAffineTerms;
    };

    bool operator==(TPSCoefficients3D const& a, TPSCoefficients3D const& b) noexcept
    {
        return
            a.a1 == b.a1 &&
            a.a2 == b.a2 &&
            a.a3 == b.a3 &&
            a.a4 == b.a4 &&
            a.NonAffineTerms == b.NonAffineTerms;
    }

    bool operator!=(TPSCoefficients3D const& a, TPSCoefficients3D const& b) noexcept
    {
        return !(a == b);
    }

    // pretty-prints `TPSCoefficients3D`
    std::ostream& operator<<(std::ostream& o, TPSCoefficients3D const& coefs)
    {
        using osc::operator<<;
        o << "TPSCoefficients3D{a1 = " << coefs.a1 << ", a2 = " << coefs.a2 << ", a3 = " << coefs.a3 << ", a4 = " << coefs.a4;
        for (size_t i = 0; i < coefs.NonAffineTerms.size(); ++i)
        {
            o << ", w" << i << " = " << coefs.NonAffineTerms[i];
        }
        o << '}';
        return o;
    }

    // this is effectviely the "U" term in the TPS algorithm literature
    //
    // i.e. U(||pi - p||) in the literature is equivalent to `RadialBasisFunction3D(pi, p)` here
    float RadialBasisFunction3D(glm::vec3 const& controlPoint, glm::vec3 const& p)
    {
        // this implementation uses the U definition from the following (later) source:
        //
        // Chapter 3, "Semilandmarks in Three Dimensions" by Phillip Gunz, Phillip Mitteroecker,
        // and Fred L. Bookstein
        //
        // the original Bookstein paper uses U(v) = |v|^2 * log(|v|^2), but subsequent literature
        // (e.g. the above book) uses U(v) = |v|. The primary author (Gunz) claims that the original
        // basis function is not as good as just using the magnitude?

        return glm::length(controlPoint - p);
    }

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D CalcCoefficients(TPSCoefficientSolverInputs3D const& inputs)
    {
        // this is based on the Bookstein Thin Plate Sline (TPS) warping algorithm
        //
        // 1. A TPS warp is (simplifying here) a linear combination:
        //
        //     f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||controlPoint_i - p||) }
        //
        //    which can be represented as a matrix multiplication between the terms (1, p.x, p.y,
        //    p.z, U(||cpi - p||)) and the coefficients (a1, a2, a3, a4, wi..)
        //
        // 2. The caller provides "landmark pairs": these are (effectively) the input
        //    arguments and the expected output
        //
        // 3. This algorithm uses the input + output to solve for the linear coefficients.
        //    Once those coefficients are known, we then have a linear equation that we
        //    we can pump new inputs into (e.g. mesh points, muscle points)
        //
        // 4. So, given the equation L * [w a] = [v o], where L is a matrix of linear terms,
        //    [w a] is a vector of the linear coefficients (we're solving for these), and [v o]
        //    is the expected output (v), with some (padding) zero elements (o)
        //
        // 5. Create matrix L:
        //
        //   |K  P|
        //   |PT 0|
        //
        //     where:
        //
        //     - K is a symmetric matrix of each *input* landmark pair evaluated via the
        //       basis function:
        //
        //        |U(p00) U(p01) U(p02)  ...  |
        //        |U(p10) U(p11) U(p12)  ...  |
        //        | ...    ...    ...   U(pnn)|
        //
        //     - P is a n-row 4-column matrix containing the number 1 (the constant term),
        //       x, y, and z (effectively, the p term):
        //
        //       |1 x1 y1 z1|
        //       |1 x2 y2 z2|
        //
        //     - PT is the transpose of P
        //     - 0 is a 4x4 zero matrix (padding)
        //
        // 6. Use a linear solver to solve L * [w a] = [v o] to yield [w a]
        // 8. Return the coefficients, [w a]

        OSC_PERF("CalcCoefficients");

        int const numPairs = static_cast<int>(inputs.Landmarks.size());

        if (numPairs == 0)
        {
            // edge-case: there are no pairs, so return an identity-like transform
            return TPSCoefficients3D{};
        }

        // construct matrix L
        SimTK::Matrix L(numPairs + 4, numPairs + 4);

        // populate the K part of matrix L (upper-left)
        for (int row = 0; row < numPairs; ++row)
        {
            for (int col = 0; col < numPairs; ++col)
            {
                glm::vec3 const& pi = inputs.Landmarks[row].Src;
                glm::vec3 const& pj = inputs.Landmarks[col].Src;

                L(row, col) = RadialBasisFunction3D(pi, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            int const pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row)
            {
                L(row, pStartColumn)     = 1.0;
                L(row, pStartColumn + 1) = inputs.Landmarks[row].Src.x;
                L(row, pStartColumn + 2) = inputs.Landmarks[row].Src.y;
                L(row, pStartColumn + 3) = inputs.Landmarks[row].Src.z;
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            int const ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col)
            {
                L(ptStartRow, col)     = 1.0;
                L(ptStartRow + 1, col) = inputs.Landmarks[col].Src.x;
                L(ptStartRow + 2, col) = inputs.Landmarks[col].Src.y;
                L(ptStartRow + 3, col) = inputs.Landmarks[col].Src.z;
            }
        }

        // populate the 0 part of matrix L (bottom-right)
        {
            int const zeroStartRow = numPairs;
            int const zeroStartCol = numPairs;

            for (int row = 0; row < 4; ++row)
            {
                for (int col = 0; col < 4; ++col)
                {
                    L(zeroStartRow + row, zeroStartCol + col) = 0.0;
                }
            }
        }

        // construct "result" vectors Vx and Vy (these hold the landmark destinations)
        SimTK::Vector Vx(numPairs + 4, 0.0);
        SimTK::Vector Vy(numPairs + 4, 0.0);
        SimTK::Vector Vz(numPairs + 4, 0.0);
        for (int row = 0; row < numPairs; ++row)
        {
            glm::vec3 const blended = glm::mix(inputs.Landmarks[row].Src, inputs.Landmarks[row].Dest, inputs.BlendingFactor);
            Vx[row] = blended.x;
            Vy[row] = blended.y;
            Vz[row] = blended.z;
        }

        // create a linear solver that can be used to solve `L*Cn = Vn` for `Cn` (where `n` is a dimension)
        SimTK::FactorQTZ const F{L};

        // solve for each dimension
        SimTK::Vector Cx(numPairs + 4, 0.0);
        F.solve(Vx, Cx);
        SimTK::Vector Cy(numPairs + 4, 0.0);
        F.solve(Vy, Cy);
        SimTK::Vector Cz(numPairs + 4, 0.0);
        F.solve(Vz, Cz);

        // `Cx/Cy/Cz` now contain the solved coefficients (e.g. for X): [w1, w2, ... wx, a0, a1x, a1y a1z]
        //
        // extract the coefficients into the return value

        TPSCoefficients3D rv;

        // populate affine a1, a2, a3, and a4 terms
        rv.a1 = {Cx[numPairs],   Cy[numPairs]  , Cz[numPairs]  };
        rv.a2 = {Cx[numPairs+1], Cy[numPairs+1], Cz[numPairs+1]};
        rv.a3 = {Cx[numPairs+2], Cy[numPairs+2], Cz[numPairs+2]};
        rv.a4 = {Cx[numPairs+3], Cy[numPairs+3], Cz[numPairs+3]};

        // populate `wi` coefficients (+ control points, needed at evaluation-time)
        rv.NonAffineTerms.reserve(numPairs);
        for (int i = 0; i < numPairs; ++i)
        {
            glm::vec3 const weight = {Cx[i], Cy[i], Cz[i]};
            glm::vec3 const& controlPoint = inputs.Landmarks[i].Src;
            rv.NonAffineTerms.emplace_back(weight, controlPoint);
        }

        return rv;
    }

    // evaluates the TPS equation with the given coefficients and input point
    glm::vec3 EvaluateTPSEquation(TPSCoefficients3D const& coefs, glm::vec3 p)
    {
        // this implementation effectively evaluates `fx(x, y, z)`, `fy(x, y, z)`, and
        // `fz(x, y, z)` the same time, because `TPSCoefficients3D` stores the X, Y, and Z
        // variants of the coefficients together in memory (as `vec3`s)

        // compute affine terms (a1 + a2*x + a3*y + a4*z)
        glm::dvec3 rv = glm::dvec3{coefs.a1} + glm::dvec3{coefs.a2*p.x} + glm::dvec3{coefs.a3*p.y} + glm::dvec3{coefs.a4*p.z};

        // accumulate non-affine terms (effectively: wi * U(||controlPoint - p||))
        for (TPSNonAffineTerm3D const& term : coefs.NonAffineTerms)
        {
            rv += term.Weight * RadialBasisFunction3D(term.ControlPoint, p);
        }

        return rv;
    }

    // returns a mesh that is the equivalent of applying the 3D TPS warp to each vertex of the mesh
    osc::Mesh ApplyThinPlateWarpToMesh(TPSCoefficients3D const& coefs, osc::Mesh const& mesh)
    {
        OSC_PERF("ApplyThinPlateWarpToMesh");

        osc::Mesh rv = mesh;  // make a local copy of the input mesh

        rv.transformVerts([&coefs](nonstd::span<glm::vec3> verts)
        {
            // parallelize function evaluation, because the mesh may contain *a lot* of
            // verts and the TPS equation may contain *a lot* of coefficients
            osc::ForEachParUnseq(8192, verts, [&coefs](glm::vec3& vert)
            {
                vert = EvaluateTPSEquation(coefs, vert);
            });
        });

        // TODO: come up with a more robust way to transform the normals

        return rv;
    }

    // returns all pair-able landmarks between the `src` and `dest`
    std::vector<LandmarkPair3D> GetLandmarkPairs(
        std::unordered_map<std::string, glm::vec3> const& src,
        std::unordered_map<std::string, glm::vec3> const& dest)
    {
        std::vector<LandmarkPair3D> rv;
        rv.reserve(src.size());  // probably a good guess
        for (auto const& [k, srcPos] : src)
        {
            auto const it = dest.find(k);
            if (it != dest.end())
            {
                rv.emplace_back(srcPos, it->second);
            }
        }
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

    // the whole TPS document that the user is editing at any given moment
    struct TPSDocument final {
        TPSDocumentInput Source{osc::GenUntexturedUVSphere(16, 16)};
        TPSDocumentInput Destination{osc::GenUntexturedSimbodyCylinder(16)};
        float BlendingFactor = 1.0f;
    };

    // enum used to label a part of the TPS document
    enum class TPSDocumentIdentifier {
        Source,
        Destination,
    };

    TPSDocumentInput const& GetInputOrThrow(TPSDocument const& doc, TPSDocumentIdentifier which)
    {
        switch (which)
        {
        case TPSDocumentIdentifier::Source:
            return doc.Source;
        case TPSDocumentIdentifier::Destination:
            return doc.Destination;
        default:
            throw std::runtime_error{"invalid document identifier provided to a getter"};
        }
    }

    TPSDocumentInput& UpdScratchInputOrThrow(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentIdentifier which)
    {
        switch (which)
        {
        case TPSDocumentIdentifier::Source:
            return doc.updScratch().Source;
        case TPSDocumentIdentifier::Destination:
            return doc.updScratch().Destination;
        default:
            throw std::runtime_error{"invalid document identifier provided to a getter"};
        }
    }

    TPSDocumentInput const& GetScratchInputOrThrow(osc::UndoRedoT<TPSDocument> const& doc, TPSDocumentIdentifier which)
    {
        switch (which)
        {
        case TPSDocumentIdentifier::Source:
            return doc.getScratch().Source;
        case TPSDocumentIdentifier::Destination:
            return doc.getScratch().Destination;
        default:
            throw std::runtime_error{"invalid document identifier provided to a getter"};
        }
    }

    // returns all pairable landmarks in the document
    std::vector<LandmarkPair3D> GetLandmarkPairs(TPSDocument const& doc)
    {
        return GetLandmarkPairs(doc.Source.Landmarks, doc.Destination.Landmarks);
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
    std::string ActionAddLandmarkTo(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentIdentifier which, glm::vec3 const& pos)
    {
        TPSDocumentInput& input = UpdScratchInputOrThrow(doc, which);
        std::string const id = std::to_string(input.Landmarks.size());

        input.Landmarks[id] = pos;
        doc.commitScratch("added landmark");

        return id;
    }

    // action: prompt the user to browse for a different source mesh
    void ActionBrowseForNewMesh(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentIdentifier which)
    {
        std::filesystem::path const p = osc::PromptUserForFile("vtp,obj");
        if (p.empty())
        {
            return;  // user didn't select anything
        }

        TPSDocumentInput& input = UpdScratchInputOrThrow(doc, which);
        input.Mesh = osc::LoadMeshViaSimTK(p);
        doc.commitScratch("changed mesh");
    }

    // action load landmarks from a headerless CSV file into source/destination
    void ActionLoadLandmarksCSV(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentIdentifier which)
    {
        std::filesystem::path const p = osc::PromptUserForFile("csv");
        if (p.empty())
        {
            return;  // user didn't select anything
        }

        std::ifstream file{p};

        if (!file)
        {
            return;  // some kind of error opening the file
        }

        osc::CSVReader reader{file};
        std::vector<glm::vec3> landmarks;

        while (auto maybeCols = reader.next())
        {
            std::vector<std::string> cols = *maybeCols;
            if (cols.size() < 3)
            {
                continue;  // too few columns: ignore
            }
            std::optional<float> x = osc::FromCharsStripWhitespace(cols[0]);
            std::optional<float> y = osc::FromCharsStripWhitespace(cols[1]);
            std::optional<float> z = osc::FromCharsStripWhitespace(cols[2]);
            if (!(x && y && z))
            {
                continue;  // a column was non-numeric: ignore entire line
            }
            landmarks.emplace_back(*x, *y, *z);
        }

        if (landmarks.empty())
        {
            return;  // file was empty, or had invalid data
        }

        TPSDocumentInput& input = UpdScratchInputOrThrow(doc, which);
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

    void ActionDeleteLandmarksByID(
        osc::UndoRedoT<TPSDocument>& doc,
        TPSDocumentIdentifier which,
        std::unordered_set<std::string> const& landmarkIDs)
    {
        if (landmarkIDs.empty())
        {
            return;
        }

        TPSDocumentInput& input = UpdScratchInputOrThrow(doc, which);
        for (std::string const& s : landmarkIDs)
        {
            input.Landmarks.erase(s);
        }
        doc.commitScratch("deleted landmarks");
    }

    // action save all source/destination landmarks to a simple headerless CSV file (matches loading)
    void ActionSaveLandmarksToCSV(TPSDocument const& doc, TPSDocumentIdentifier which)
    {
        std::filesystem::path const filePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (filePath.empty())
        {
            return;  // user didn't select a save location
        }

        std::ofstream outfile{filePath};

        if (!outfile)
        {
            return;  // couldn't open file for writing
        }

        osc::CSVWriter writer{outfile};
        std::vector<std::string> cols(3);

        TPSDocumentInput const& input = GetInputOrThrow(doc, which);
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
        std::vector<LandmarkPair3D> const pairs = GetLandmarkPairs(doc);
        std::filesystem::path const filePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (filePath.empty())
        {
            return;  // user didn't select a save location
        }

        std::ofstream outfile{filePath};

        if (!outfile)
        {
            return;  // couldn't open file for writing
        }

        osc::CSVWriter writer{outfile};

        std::vector<std::string> cols = {"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"};

        writer.writeRow(cols);  // write header
        for (LandmarkPair3D const& p : pairs)
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
        std::filesystem::path const filePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");

        if (filePath.empty())
        {
            return;  // user didn't select a save location
        }

        std::ios_base::openmode const flags =
            std::ios_base::out |
            std::ios_base::trunc;

        std::ofstream outfile{filePath, flags};

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
        std::filesystem::path const filePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");

        if (filePath.empty())
        {
            return;  // user didn't select a save location
        }

        std::ios_base::openmode const flags =
            std::ios_base::binary |
            std::ios_base::out |
            std::ios_base::trunc;

        std::ofstream outfile{filePath, flags};

        if (!outfile)
        {
            return;  // couldn't open for writing
        }

        osc::StlWriter writer{outfile};

        writer.write(mesh);
    }

    // a cache that only recomputes the transformed mesh if the document
    // has changed (e.g. a user added a landmark or changed blending factor)
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

            TPSCoefficients3D newCoefficients = CalcCoefficients(m_CachedInputs);

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
            TPSCoefficientSolverInputs3D newInputs
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

        TPSCoefficientSolverInputs3D m_CachedInputs;
        TPSCoefficients3D m_CachedCoefficients;
        osc::Mesh m_CachedSourceMesh;
        osc::Mesh m_CachedResultMesh;
    };
}

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
        osc::Line const ray = camera.unprojectTopLeftPosToWorldRay(mousePos - renderRect.p1, osc::Dimensions(renderRect));
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

// TPS UI code
//
// UI code that is specific to the TPS3D UI
namespace
{
    struct TPSTabSharedState;

    // type-erased constructor function for an in-UI ImGui panel
    using PanelConstructor = std::function<std::shared_ptr<osc::VirtualPanel>(std::shared_ptr<TPSTabSharedState>)>;

    // holds information for a user-toggleable panel
    struct TPSUIPanel final {
        TPSUIPanel(
            std::string_view name_,
            PanelConstructor constructor_,
            bool isEnabledByDefault_) :

            Name{std::move(name_)},
            Constructor{std::move(constructor_)},
            IsEnabledByDefault{std::move(isEnabledByDefault_)},
            Instance{std::nullopt}
        {
        }

        std::string Name;
        PanelConstructor Constructor;
        bool IsEnabledByDefault;
        std::optional<std::shared_ptr<osc::VirtualPanel>> Instance;
    };

    // forward-declaration for a function that will return all available panels
    std::vector<TPSUIPanel> GetAvailablePanels();

    // holds information about the user's current mouse hover
    struct TPSTabHover final {
        explicit TPSTabHover(glm::vec3 const& worldspaceLocation_) :
            MaybeSceneElementID{std::nullopt},
            WorldspaceLocation{worldspaceLocation_}
        {
        }

        TPSTabHover(
            std::string sceneElementID_,
            glm::vec3 const& worldspaceLocation_) :

            MaybeSceneElementID{std::move(sceneElementID_)},
            WorldspaceLocation{worldspaceLocation_}
        {
        }

        std::optional<std::string> MaybeSceneElementID;
        glm::vec3 WorldspaceLocation;
    };

    // holds information about the user's current selection
    struct TPSTabSelection final {

        void clear()
        {
            m_SelectedSceneElements.clear();
        }

        void select(std::string const& id)
        {
            m_SelectedSceneElements.insert(id);
        }

        bool contains(std::string const& id) const
        {
            return m_SelectedSceneElements.find(id) != m_SelectedSceneElements.end();
        }

        std::unordered_set<std::string> const& getUnderlyingSet() const
        {
            return m_SelectedSceneElements;
        }

    private:
        std::unordered_set<std::string> m_SelectedSceneElements;
    };

    // top-level tab state
    //
    // (shared by all panels)
    struct TPSTabSharedState final {

        osc::Mesh const& getTransformedMesh()
        {
            return ResultCache.lookup(EditedDocument->getScratch());
        }

        TPSDocumentInput& updInputDocument(TPSDocumentIdentifier identifier)
        {
            return UpdScratchInputOrThrow(*EditedDocument, identifier);
        }

        TPSDocumentInput const& getInputDocument(TPSDocumentIdentifier identifier) const
        {
            return GetScratchInputOrThrow(*EditedDocument, identifier);
        }

        osc::Mesh const& getInputMesh(TPSDocumentIdentifier identifier) const
        {
            return getInputDocument(identifier).Mesh;
        }

        // the document the user is editing
        std::shared_ptr<osc::UndoRedoT<TPSDocument>> EditedDocument = std::make_shared<osc::UndoRedoT<TPSDocument>>();

        // cached TPS3D algorithm result (don't recompute it each frame)
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
        std::optional<TPSTabHover> PerFrameHover;

        // available/active panels that the user can toggle via the `window` menu
        std::vector<TPSUIPanel> Panels = GetAvailablePanels();
    };

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSTabSharedState const& sharedState,
        osc::Mesh const& mesh,
        bool wireframeMode,
        std::vector<osc::SceneDecoration>& out)
    {
        out.reserve(out.size() + 5);  // likely guess

        // draw the mesh
        out.emplace_back(mesh);

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            osc::SceneDecoration& dec = out.emplace_back(mesh);
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
            TPSDocumentIdentifier documentIdentifier_) :

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
                m_State->PerFrameHover = landmarkCollision;
            }
            else if (meshCollision)
            {
                m_State->PerFrameHover.emplace(meshCollision->position);
            }

            // render: draw the scene into the content rect and hittest it
            osc::RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
            osc::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = osc::HittestLastImguiItem();

            // handle any events due to hovering over, clicking, etc.
            handleInputAndHoverEvents(m_LastTextureHittestResult, meshCollision, landmarkCollision);

            // draw any 2D ImGui overlays
            drawOverlays(m_LastTextureHittestResult.rect);
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
                        rv.emplace(id, pos);
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
            // event: if the cameras are linked together, ensure this camera is updated from the linked camera
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

            // event: if the user interacts with the render, update the camera as necessary
            if (htResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiUserInput(osc::Dimensions(htResult.rect), m_Camera))
                {
                    m_State->LinkedCameraBase = m_Camera;  // reflects latest modification
                }
            }

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
                ActionDeleteLandmarksByID(
                    *m_State->EditedDocument,
                    m_DocumentIdentifier,
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
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = pos;

                osc::SceneDecoration& decoration = decorations.emplace_back(m_State->LandmarkSphere, transform, m_State->LandmarkColor);

                if (m_State->UserSelection.contains(id))
                {
                    decoration.color += glm::vec4{0.25f, 0.25f, 0.25f, 0.0f};
                    decoration.color = glm::clamp(decoration.color, glm::vec4{0.0f}, glm::vec4{1.0f});
                    decoration.flags = osc::SceneDecorationFlags_IsSelected;
                }
                else if (m_State->PerFrameHover && m_State->PerFrameHover->MaybeSceneElementID == id)
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
        TPSDocumentIdentifier m_DocumentIdentifier;
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

            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = renderScene(dims);
            osc::DrawTextureAsImGuiImage(renderTexture);
            osc::ImGuiItemHittestResult const htResult = osc::HittestLastImguiItem();

            // update camera if user drags it around etc.
            if (htResult.isHovered)
            {
                if (osc::UpdatePolarCameraFromImGuiUserInput(dims, m_Camera))
                {
                    m_State->LinkedCameraBase = m_Camera;  // reflects latest modification
                }
            }

            drawOverlays(htResult.rect);
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(osc::Rect const& renderRect)
        {
            glm::vec2 const padding = {10.0f, 10.0f};

            // ImGui: draw buttons etc. at the top of the panel
            {
                // ImGui: set cursor to draw over the top-right of the render texture (with padding)
                ImGui::SetCursorScreenPos(renderRect.p1 + padding);

                // ImGui: draw "autofit camera" button
                if (ImGui::Button(ICON_FA_EXPAND))
                {
                    osc::AutoFocus(m_Camera, m_State->getTransformedMesh().getBounds());
                    m_State->LinkedCameraBase = m_Camera;
                }

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_SAVE))
                {
                    ActionTrySaveMeshToObj(m_State->getTransformedMesh());
                }
                osc::DrawTooltipIfItemHovered("export to OBJ");

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_SAVE " "))
                {
                    ActionTrySaveMeshToStl(m_State->getTransformedMesh());
                }
                osc::DrawTooltipIfItemHovered("export to STL");

                ImGui::SameLine();

                // ImGui: draw checkbox for toggling whether to show the destination mesh in the scene
                {
                    ImGui::Checkbox("show destination", &m_ShowDestinationMesh);
                }
            }

            // ImGui: draw slider overlay that controls TPS blend factor at the bottom of the panel
            {
                float const panelHeight = ImGui::GetTextLineHeight() + 2.0f*ImGui::GetStyle().FramePadding.y;

                ImGui::SetCursorScreenPos({renderRect.p1.x + padding.x, renderRect.p2.y - (panelHeight + padding.y)});
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - padding.x - ImGui::CalcTextSize("blending factor").x - ImGui::GetStyle().ItemSpacing.x);
                float factor = m_State->EditedDocument->getScratch().BlendingFactor;
                if (ImGui::SliderFloat("blending factor", &factor, 0.0f, 1.0f))
                {
                    ActionSetBlendFactor(*m_State->EditedDocument, factor);
                }
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    ActionSetBlendFactorAndSave(*m_State->EditedDocument, factor);
                }
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
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
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
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentIdentifier::Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentIdentifier::Destination);
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
            if (m_TabState->PerFrameHover)
            {
                glm::vec3 const pos = m_TabState->PerFrameHover->WorldspaceLocation;
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
                if (m_TabState->PerFrameHover->MaybeSceneElementID)
                {
                    ImGui::TextDisabled("(left-click to select %s)", m_TabState->PerFrameHover->MaybeSceneElementID->c_str());
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
            if (ImGui::MenuItem("New Document"))
            {
                ActionCreateNewDocument(*m_TabState->EditedDocument);
            }
            if (ImGui::MenuItem("Load Source Mesh"))
            {
                ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentIdentifier::Source);
            }
            if (ImGui::MenuItem("Load Destination Mesh"))
            {
                ActionBrowseForNewMesh(*m_TabState->EditedDocument, TPSDocumentIdentifier::Destination);
            }
            if (ImGui::MenuItem("Load Source Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_TabState->EditedDocument, TPSDocumentIdentifier::Source);
            }
            if (ImGui::MenuItem("Load Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_TabState->EditedDocument, TPSDocumentIdentifier::Destination);
            }
            if (ImGui::MenuItem("Save Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_TabState->EditedDocument->getScratch(), TPSDocumentIdentifier::Source);
            }
            if (ImGui::MenuItem("Save Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_TabState->EditedDocument->getScratch(), TPSDocumentIdentifier::Destination);
            }
            if (ImGui::MenuItem("Save Landmark Pairs to CSV"))
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
            for (TPSUIPanel& panel : m_TabState->Panels)
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
                        panel.Instance = panel.Constructor(m_TabState);
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
            m_TabState{std::move(tabState_)}
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
        std::shared_ptr<TPSTabSharedState> m_TabState;
        TPS3DFileMenu m_FileMenu{m_TabState};
        TPS3DEditMenu m_EditMenu{m_TabState};
        TPS3DWindowMenu m_WindowMenu{m_TabState};
        osc::MainMenuAboutTab m_AboutTab;
    };

    std::vector<TPSUIPanel> GetAvailablePanels()
    {
        return
        {
            {
                "Source Mesh",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<TPS3DInputPanel>("Source Mesh", std::move(st), TPSDocumentIdentifier::Source);
                },
                true,
            },
            {
                "Destination Mesh",
                [](std::shared_ptr<TPSTabSharedState> st)
                {
                    return std::make_shared<TPS3DInputPanel>("Destination Mesh", std::move(st), TPSDocumentIdentifier::Destination);
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
        for (TPSUIPanel& panel : m_TabState->Panels)
        {
            if (panel.IsEnabledByDefault)
            {
                panel.Instance = panel.Constructor(m_TabState);
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

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        // re-perform hover test each frame
        m_TabState->PerFrameHover.reset();

        // garbage collect closed-panel instance data
        for (TPSUIPanel& panel : m_TabState->Panels)
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
        for (TPSUIPanel& panel : m_TabState->Panels)
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
    std::shared_ptr<TPSTabSharedState> m_TabState = std::make_shared<TPSTabSharedState>();

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
