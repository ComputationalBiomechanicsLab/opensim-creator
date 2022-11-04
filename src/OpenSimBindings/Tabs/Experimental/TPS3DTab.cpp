#include "TPS3DTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Formats/CSV.hpp"
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
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/UndoRedo.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/NamedPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/RedoButton.hpp"
#include "src/Widgets/UndoButton.hpp"
#include "src/Widgets/UndoRedoPanel.hpp"

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
#include <execution>
#include <fstream>
#include <functional>
#include <string>
#include <sstream>
#include <iostream>
#include <limits>
#include <optional>
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
        TPSCoefficientSolverInputs3D(std::vector<LandmarkPair3D> landmarks_, float blendingFactor_) :
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

    std::ostream& operator<<(std::ostream& o, TPSCoefficientSolverInputs3D const& inputs)
    {
        o << "TPSCoefficientSolverInputs3D{landmarks = [";
        char const* delim = "";
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
        TPSNonAffineTerm3D(glm::vec3 const& weight_, glm::vec3 const& controlPoint_) :
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

    // pretty-prints TPSCoefficients3D
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

    // this is effectviely the "U" term in the TPS algorithm literature (which is usually U(r) = r^2 * log(r^2))
    //
    // i.e. U(||pi - p||) in the literature is equivalent to `RadialBasisFunction3D(pi, p)` here
    float RadialBasisFunction3D(glm::vec3 controlPoint, glm::vec3 const& p)
    {
        glm::vec3 const diff = controlPoint - p;
        float const r2 = glm::dot(diff, diff);

        if (r2 != 0.0f)
        {
            return r2 * std::log(r2);
        }
        else
        {
            // this ensures that the result is always non-zero and non-NaN (this might be
            // necessary for some types of linear solvers?)
            return std::numeric_limits<float>::min();
        }
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
        glm::vec3 rv = coefs.a1 + coefs.a2*p.x + coefs.a3*p.y + coefs.a4*p.z;

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
            // multithread the warping, because it is slow when applied to large meshes
            std::for_each(std::execution::par_unseq, verts.begin(), verts.end(), [&coefs](glm::vec3& vert)
            {
                vert = EvaluateTPSEquation(coefs, vert);
            });
        });

        return rv;
    }

    // returns all pair-able landmarks between the `src` and `dest`
    std::vector<LandmarkPair3D> GetLandmarkPairs(
        std::unordered_map<size_t, glm::vec3> const& src,
        std::unordered_map<size_t, glm::vec3> const& dest)
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
        TPSDocumentInput(osc::Mesh const& mesh_) : Mesh{mesh_} {}

        osc::Mesh Mesh;
        std::unordered_map<size_t, glm::vec3> Landmarks;
    };

    // the whole TPS document that the user is editing at any given moment
    struct TPSDocument final {
        TPSDocumentInput Source{osc::GenUntexturedUVSphere(16, 16)};
        TPSDocumentInput Destination{osc::GenUntexturedSimbodyCylinder(16)};
        float BlendingFactor = 1.0f;
    };

    // returns all pair-able landmarks in the document
    std::vector<LandmarkPair3D> GetLandmarkPairs(TPSDocument const& doc)
    {
        return GetLandmarkPairs(doc.Source.Landmarks, doc.Destination.Landmarks);
    }

    // action: add a landmark to the source mesh
    void ActionAddLandmarkTo(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentInput& input, glm::vec3 const& pos)
    {
        input.Landmarks[input.Landmarks.size()] = pos;
        doc.commitScratch("added landmark");
    }

    // action: prompt the user to browse for a different source mesh
    void ActionBrowseForNewMesh(osc::UndoRedoT<TPSDocument>& doc, TPSDocumentInput& input)
    {
        std::filesystem::path const p = osc::PromptUserForFile("vtp,obj");
        if (!p.empty())
        {
            input.Mesh = osc::LoadMeshViaSimTK(p);
            doc.commitScratch("changed mesh");
        }
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

    void ActionCreateNewDocument(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch() = TPSDocument{};
        doc.commitScratch("created new document");
    }

    void ActionClearAllLandmarks(osc::UndoRedoT<TPSDocument>& doc)
    {
        doc.updScratch().Source.Landmarks.clear();
        doc.updScratch().Destination.Landmarks.clear();
        doc.commitScratch("cleared all landmarks");
    }

    void ActionSaveLandmarksToCSV(TPSDocument const& doc)
    {
        std::vector<LandmarkPair3D> const pairs = GetLandmarkPairs(doc);
        std::filesystem::path const p = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (p.empty())
        {
            return;  // user didn't select a save location
        }

        std::ofstream outfile{p};

        if (!outfile)
        {
            return;  // couldn't open file for writing
        }

        osc::CSVWriter writer{outfile};

        std::vector<std::string> cols = {"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"};

        writer.writerow(cols);  // header
        for (LandmarkPair3D const& p : pairs)
        {
            cols.at(0) = std::to_string(p.Src.x);
            cols.at(1) = std::to_string(p.Src.y);
            cols.at(2) = std::to_string(p.Src.z);

            cols.at(0) = std::to_string(p.Dest.x);
            cols.at(1) = std::to_string(p.Dest.y);
            cols.at(2) = std::to_string(p.Dest.z);
            writer.writerow(cols);
        }
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
    std::optional<glm::vec3> RaycastMesh(
        osc::PolarPerspectiveCamera const& camera,
        osc::Mesh const& mesh,
        osc::Rect const& renderRect,
        glm::vec2 mousePos)
    {
        osc::Line const ray = camera.unprojectTopLeftPosToWorldRay(mousePos - renderRect.p1, osc::Dimensions(renderRect));
        osc::RayCollision const maybeCollision = osc::GetClosestWorldspaceRayCollision(mesh, osc::Transform{}, ray);
        return maybeCollision ? std::optional<glm::vec3>{ray.origin + ray.dir*maybeCollision.distance} : std::nullopt;
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
    // top-level tab state
    //
    // (shared by all panels)
    struct TPSUITabSate final {
        std::shared_ptr<osc::UndoRedoT<TPSDocument>> EditedDocument = std::make_shared<osc::UndoRedoT<TPSDocument>>();
        TPSResultCache ResultCache;
        bool LinkCameras = true;
        osc::PolarPerspectiveCamera LinkedCameraBase = CreateCameraFocusedOn(EditedDocument->getScratch().Source.Mesh.getBounds());
        osc::Material WireframeMaterial = osc::CreateWireframeOverlayMaterial();
        std::shared_ptr<osc::Mesh const> LandmarkSphere = osc::App::meshes().getSphereMesh();
        glm::vec4 LandmarkColor = {1.0f, 0.0f, 0.0f, 1.0f};
        std::optional<glm::vec3> PerFrameHover;

        osc::Mesh const& getTransformedMesh()
        {
            return ResultCache.lookup(EditedDocument->getScratch());
        }
    };

    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        TPSUITabSate const& shared,
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
            dec.maybeMaterial = shared.WireframeMaterial;
        }

        // add grid decorations
        DrawXZGrid(out);
        DrawXZFloorLines(out, 100.0f);
    }
}

// ImGui-level datastructures (panels, etc.)
namespace
{
    // generic base class for the panels shown in the TPS3D tab
    class TPS3DTabPanel : public osc::NamedPanel {
    public:
        using osc::NamedPanel::NamedPanel;

    private:
        void implPushWindowStyles() override final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }
        void implPopWindowStyles() override final
        {
            ImGui::PopStyleVar();
        }
    };

    // an "input" panel (i.e. source or destination mesh, before warping)
    class InputPanel final : public TPS3DTabPanel {
    public:
        InputPanel(std::string_view panelName_, std::shared_ptr<TPSUITabSate> state_, bool useSrc_) :
            TPS3DTabPanel{std::move(panelName_), ImGuiDockNodeFlags_PassthruCentralNode},
            m_State{std::move(state_)},
            m_UseSource{useSrc_}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid shared state");
        }

        osc::PolarPerspectiveCamera const& getCamera() const
        {
            return m_Camera;
        }

        osc::PolarPerspectiveCamera& updCamera()
        {
            return m_Camera;
        }

    private:
        void implDraw() override
        {
            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = renderScene(dims);
            osc::ImGuiImageHittestResult const htResult = osc::DrawTextureAsImGuiImageAndHittest(renderTexture);

            // update camera if user drags it around etc.
            std::optional<glm::vec3> maybeCollision;
            if (htResult.isHovered)
            {
                osc::UpdatePolarCameraFromImGuiUserInput(dims, m_Camera);
                glm::vec2 const mousePos = ImGui::GetMousePos();
                maybeCollision = RaycastMesh(m_Camera, getMesh(), htResult.rect, mousePos);
            }

            // if the user is currently hitting something in 3D, update global mouse cursor tracker
            if (maybeCollision)
            {
                m_State->PerFrameHover = *maybeCollision;
            }

            // if user left-clicks, add a landmark there
            if (maybeCollision && htResult.isLeftClickReleasedWithoutDragging)
            {
                ActionAddLandmarkTo(*m_State->EditedDocument, updDocumentInput(), *maybeCollision);
            }

            // draw any 2D ImGui overlays
            drawOverlays(htResult.rect);
        }

        void drawOverlays(osc::Rect const& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            glm::vec2 const padding = {10.0f, 10.0f};
            glm::vec2 const originalCursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(renderRect.p1 + padding);
            // TODO: breaks scrolling? OSC_SCOPE_GUARD({ ImGui::SetCursorScreenPos(originalCursorPos); });

            // ImGui: draw "browse for new mesh" button
            if (ImGui::Button("browse"))
            {
                ActionBrowseForNewMesh(*m_State->EditedDocument, updDocumentInput());
            }

            ImGui::SameLine();

            // ImGui: draw "autofit camera" button
            if (ImGui::Button(ICON_FA_EXPAND))
            {
                osc::AutoFocus(m_Camera, getMesh().getBounds());
            }

            ImGui::SameLine();

            // ImGui: draw landmark radius editor
            {
                char const* label = "landmark radius";
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - 10.0f);
                ImGui::SliderFloat(label, &m_LandmarkRadius, 0.0001f, 1.0f, "%.4f", 2.0f);
            }
        }

        // renders this panel's 3D scene to a texture
        osc::RenderTexture& renderScene(glm::vec2 dims)
        {
            osc::SceneRendererParams const params = CalcRenderParams(m_Camera, dims);
            std::vector<osc::SceneDecoration> const decorations = generateDecorations();
            return m_CachedRenderer.draw(decorations, params);
        }

        // returns a fresh list of decorations for this panels
        std::vector<osc::SceneDecoration> generateDecorations() const
        {
            // generate in-scene 3D decorations
            std::vector<osc::SceneDecoration> decorations;
            decorations.reserve(5 + getDocumentInput().Landmarks.size());  // likely guess

            AppendCommonDecorations(*m_State, getMesh(), m_WireframeMode, decorations);

            // append each landmark as a sphere
            for (auto const& [id, pos] : getDocumentInput().Landmarks)
            {
                osc::Transform transform{};
                transform.scale *= m_LandmarkRadius;
                transform.position = pos;

                decorations.emplace_back(m_State->LandmarkSphere, transform, m_State->LandmarkColor);
            }

            return decorations;
        }

        TPSDocumentInput& updDocumentInput()
        {
            TPSDocument& doc = m_State->EditedDocument->updScratch();
            return m_UseSource ? doc.Source : doc.Destination;
        }

        TPSDocumentInput const& getDocumentInput() const
        {
            TPSDocument const& doc = m_State->EditedDocument->updScratch();
            return m_UseSource ? doc.Source : doc.Destination;
        }

        osc::Mesh const& getMesh() const
        {
            return getDocumentInput().Mesh;
        }

        std::shared_ptr<TPSUITabSate> m_State;
        bool m_UseSource = true;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(getMesh().getBounds());
        osc::CachedSceneRenderer m_CachedRenderer;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class ResultPanel final : public TPS3DTabPanel {
    public:

        ResultPanel(std::string_view panelName_, std::shared_ptr<TPSUITabSate> state_) :
            TPS3DTabPanel{std::move(panelName_)},
            m_State{std::move(state_)}
        {
            OSC_ASSERT(m_State != nullptr && "the input panel requires a valid shared state");
        }

        osc::PolarPerspectiveCamera const& getCamera() const
        {
            return m_Camera;
        }

        osc::PolarPerspectiveCamera& updCamera()
        {
            return m_Camera;
        }

    private:
        void implDraw() override
        {
            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = renderScene(dims);
            osc::ImGuiImageHittestResult const htResult = osc::DrawTextureAsImGuiImageAndHittest(renderTexture);

            // update camera if user drags it around etc.
            if (htResult.isHovered)
            {
                osc::UpdatePolarCameraFromImGuiUserInput(dims, m_Camera);
            }

            drawOverlays(htResult.rect);
        }

        // draw ImGui overlays over a result panel
        void drawOverlays(osc::Rect const& renderRect)
        {
            // ImGui: draw buttons etc. at the top of the panel
            {
                // ImGui: set cursor to draw over the top-right of the render texture (with padding)
                glm::vec2 const padding = {10.0f, 10.0f};
                glm::vec2 const originalCursorPos = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(renderRect.p1 + padding);
                OSC_SCOPE_GUARD({ ImGui::SetCursorScreenPos(originalCursorPos); });

                // ImGui: draw checkbox for toggling whether to show the destination mesh in the scene
                {
                    ImGui::Checkbox("show destination", &m_ShowDestinationMesh);
                }
            }

            // ImGui: draw slider overlay that controls TPS blend factor at the bottom of the panel
            {
                float constexpr leftPadding = 10.0f;
                float constexpr bottomPadding = 10.0f;
                float const panelHeight = ImGui::GetTextLineHeight() + 2.0f*ImGui::GetStyle().FramePadding.y;

                glm::vec2 const oldCursorPos = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos({renderRect.p1.x + leftPadding, renderRect.p2.y - (panelHeight + bottomPadding)});
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - leftPadding - ImGui::CalcTextSize("blending factor").x - ImGui::GetStyle().ItemSpacing.x);
                float factor = m_State->EditedDocument->getScratch().BlendingFactor;
                if (ImGui::SliderFloat("blending factor", &factor, 0.0f, 1.0f))
                {
                    ActionSetBlendFactor(*m_State->EditedDocument, factor);
                }
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    ActionSetBlendFactorAndSave(*m_State->EditedDocument, factor);
                }
                ImGui::SetCursorScreenPos(oldCursorPos);
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

        std::shared_ptr<TPSUITabSate> m_State;
        osc::PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getTransformedMesh().getBounds());
        osc::CachedSceneRenderer m_CachedRenderer;
        bool m_WireframeMode = true;
        bool m_ShowDestinationMesh = false;
    };

    class TopToolbar final {
    public:
        TopToolbar(
            std::string_view label,
            std::shared_ptr<TPSUITabSate> tabState_) :

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
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, m_TabState->EditedDocument->updScratch().Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionBrowseForNewMesh(*m_TabState->EditedDocument, m_TabState->EditedDocument->updScratch().Destination);
                }
                ImGui::EndPopup();
            }
            osc::DrawTooltipIfItemHovered("Open File", "Open Source/Destination data");
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSaveLandmarksToCSV(m_TabState->EditedDocument->getScratch());
            }
            osc::DrawTooltipIfItemHovered("Save Landmarks to CSV", "Saves all pair-able landmarks to a CSV file, for external processing");
        }

        void drawCameraLockCheckbox()
        {
            bool v = m_TabState->LinkCameras;
            if (ImGui::Checkbox("link cameras", &v))
            {
                m_TabState->LinkCameras = v;
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
        std::shared_ptr<TPSUITabSate> m_TabState;
        osc::UndoButton m_UndoButton{m_TabState->EditedDocument};
        osc::RedoButton m_RedoButton{m_TabState->EditedDocument};
    };

    class BottomToolBar final {
    public:
        BottomToolBar(
            std::string_view label,
            std::shared_ptr<TPSUITabSate> tabState_) :

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
                glm::vec3 const pos = *m_TabState->PerFrameHover;
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
                ImGui::TextDisabled("(left-click to add a landmark)");
            }
            else
            {
                ImGui::TextDisabled("(nothing hovered)");
            }
        }

        std::string m_Label;
        std::shared_ptr<TPSUITabSate> m_TabState;
    };
}

// tab implementation
class osc::TPS3DTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        OSC_ASSERT(m_Parent != nullptr);
        OSC_ASSERT(m_TabState != nullptr && "the tab state should be initialized by this point");
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

        // if requested, lock cameras together
        if (m_TabState->LinkCameras)
        {
            osc::PolarPerspectiveCamera& baseCamera = m_TabState->LinkedCameraBase;

            if (m_SourcePanel.getCamera() != baseCamera)
            {
                baseCamera = m_SourcePanel.getCamera();
                m_DestPanel.updCamera() = baseCamera;
                m_ResultPanel.updCamera() = baseCamera;
            }
            else if (m_DestPanel.getCamera() != baseCamera)
            {
                baseCamera = m_DestPanel.getCamera();
                m_SourcePanel.updCamera() = baseCamera;
                m_ResultPanel.updCamera() = baseCamera;
            }
            else if (m_ResultPanel.getCamera() != baseCamera)
            {
                baseCamera = m_ResultPanel.getCamera();
                m_SourcePanel.updCamera() = baseCamera;
                m_DestPanel.updCamera() = baseCamera;
            }
        }
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.draw();
        m_SourcePanel.draw();
        m_DestPanel.draw();
        m_ResultPanel.draw();
        m_LogViewerPanel.draw();
        m_PerfPanel.draw();
        m_UndoRedoPanel.draw();
        m_BottomToolbar.draw();
    }

private:

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " TPS3DTab";
    TabHost* m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<TPSUITabSate> m_TabState = std::make_shared<TPSUITabSate>();

    // user-visible panels/bars
    TopToolbar m_TopToolbar{"##TopToolBar", m_TabState};
    InputPanel m_SourcePanel{"Source Mesh", m_TabState, true};
    InputPanel m_DestPanel{"Destination Mesh", m_TabState, false};
    ResultPanel m_ResultPanel{"Result", m_TabState};
    UndoRedoPanel m_UndoRedoPanel{"History", m_TabState->EditedDocument};
    LogViewerPanel m_LogViewerPanel{"Log"};
    PerfPanel m_PerfPanel{"Performance"};
    BottomToolBar m_BottomToolbar{"##BottomToolBar", m_TabState};
};


// public API (PIMPL)

osc::TPS3DTab::TPS3DTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::TPS3DTab::TPS3DTab(TPS3DTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::TPS3DTab& osc::TPS3DTab::operator=(TPS3DTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::TPS3DTab::~TPS3DTab() noexcept
{
    delete m_Impl;
}

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
