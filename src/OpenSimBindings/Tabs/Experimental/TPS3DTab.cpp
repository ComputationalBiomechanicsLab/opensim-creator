#include "TPS3DTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
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
#include "src/Utils/UID.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/NamedPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"

#include <glm/mat3x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>
#include <Simbody.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <sstream>
#include <iostream>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

// 3D TPS algorithm stuff
//
// most of the background behind this is discussed in issue #467. For redundancy's sake, here
// are some of the references used to write this implementation:
//
// - primary literature source: https://ieeexplore.ieee.org/document/24792
// - blog explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
// - blog explanation #2: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
//
// the code here doesn't explore things like how to actually render the scene, user interaction,
// undo/redo, etc - that's handled in the "TPS document" section
namespace
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    struct LandmarkPair3D final {
        glm::vec3 Src;
        glm::vec3 Dest;

        LandmarkPair3D() = default;
        LandmarkPair3D(glm::vec3 const& src_, glm::vec3 const& dest_) :
            Src{src_},
            Dest{dest_}
        {
        }
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
    struct TPSInputs3D final {
        TPSInputs3D() = default;

        TPSInputs3D(std::vector<LandmarkPair3D> landmarks_, float blendingFactor_) :
            Landmarks{std::move(landmarks_)},
            BlendingFactor{std::move(blendingFactor_)}
        {
        }

        std::vector<LandmarkPair3D> Landmarks;
        float BlendingFactor = 1.0f;
    };

    bool operator==(TPSInputs3D const& a, TPSInputs3D const& b) noexcept
    {
        return a.Landmarks == b.Landmarks && a.BlendingFactor == b.BlendingFactor;
    }

    bool operator!=(TPSInputs3D const& a, TPSInputs3D const& b) noexcept
    {
        return !(a == b);
    }

    std::ostream& operator<<(std::ostream& o, TPSInputs3D const& inputs)
    {
        o << "TPSInputs3D{landmarks = [";
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
        glm::vec3 Weight;
        glm::vec3 ControlPoint;

        TPSNonAffineTerm3D(glm::vec3 const& weight_, glm::vec3 const& controlPoint_) :
            Weight{weight_},
            ControlPoint{controlPoint_}
        {
        }
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
    float RadialBasisFunction3D(glm::vec3 controlPoint, glm::vec3 p)
    {
        glm::vec3 const diff = controlPoint - p;
        float const r2 = glm::dot(diff, diff);

        if (r2 == 0.0f)
        {
            // this ensures that the result is always non-zero and non-NaN (this might be
            // necessary for some types of linear solvers?)
            return std::numeric_limits<float>::min();
        }
        else
        {
            return r2 * std::log(r2);
        }
    }

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D CalcCoefficients(nonstd::span<LandmarkPair3D const> landmarkPairs, float blendingFactor)
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

        int const numPairs = static_cast<int>(landmarkPairs.size());

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
                glm::vec3 const& pi = landmarkPairs[row].Src;
                glm::vec3 const& pj = landmarkPairs[col].Src;

                L(row, col) = RadialBasisFunction3D(pi, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            int const pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row)
            {
                L(row, pStartColumn)     = 1.0;
                L(row, pStartColumn + 1) = landmarkPairs[row].Src.x;
                L(row, pStartColumn + 2) = landmarkPairs[row].Src.y;
                L(row, pStartColumn + 3) = landmarkPairs[row].Src.z;
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            int const ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col)
            {
                L(ptStartRow, col)     = 1.0;
                L(ptStartRow + 1, col) = landmarkPairs[col].Src.x;
                L(ptStartRow + 2, col) = landmarkPairs[col].Src.y;
                L(ptStartRow + 3, col) = landmarkPairs[col].Src.z;
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
            glm::vec3 const blended = glm::mix(landmarkPairs[row].Src, landmarkPairs[row].Dest, blendingFactor);
            Vx[row] = blended.x;
            Vy[row] = blended.y;
            Vz[row] = blended.z;
        }

        // construct coefficient vectors that will receive the solver's result
        SimTK::Vector Cx(numPairs + 4, 0.0);
        SimTK::Vector Cy(numPairs + 4, 0.0);
        SimTK::Vector Cz(numPairs + 4, 0.0);

        // solve `L*Cx = Vx` and `L*Cy = Vy` for `Cx` and `Cy` (the coefficients)
        SimTK::FactorQTZ const F{L};
        F.solve(Vx, Cx);
        F.solve(Vy, Cy);
        F.solve(Vz, Cz);

        // the coefficient vectors now contain (e.g. for X): [w1, w2, ... wx, a0, a1x, a1y a1z]
        //
        // extract them into the return value

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
            glm::vec3 const& controlPoint = landmarkPairs[i].Src;
            rv.NonAffineTerms.emplace_back(weight, controlPoint);
        }

        return rv;
    }

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D CalcCoefficients(TPSInputs3D const& inputs)
    {
        // this is just a convenience form of the function that takes each input seperately
        return CalcCoefficients(inputs.Landmarks, inputs.BlendingFactor);
    }

    // evaluates the TPS equation with the given coefficients and input point
    glm::vec3 EvaluateTPSEquation(TPSCoefficients3D const& coefs, glm::vec3 const& p)
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

    // returns a mesh that is the equivalent of applying the 3D TPS warp to all
    // vertices of the input mesh
    osc::Mesh ApplyThinPlateWarpToMesh(TPSCoefficients3D const& coefs, osc::Mesh const& mesh)
    {
        osc::Mesh rv = mesh;

        rv.transformVerts([&coefs](nonstd::span<glm::vec3> vs)
        {
            for (glm::vec3& v : vs)
            {
                v = EvaluateTPSEquation(coefs, v);
            }
        });

        return rv;
    }
}

// generic UI stuff
namespace
{
    // returns a material that can draw a mesh's triangles in wireframe-style
    osc::Material CreateWireframeOverlayMaterial()
    {
        osc::Material material{osc::ShaderCache::get("shaders/SceneSolidColor.vert", "shaders/SceneSolidColor.frag")};
        material.setVec4("uDiffuseColor", {0.0f, 0.0f, 0.0f, 0.6f});
        material.setWireframeMode(true);
        material.setTransparent(true);
        return material;
    }

    // auto-focuses the given polar camera on the mesh, such that it fits in frame
    void AutofocusCameraOnMesh(osc::Mesh const& mesh, osc::PolarPerspectiveCamera& camera)
    {
        osc::AABB const bounds = mesh.getBounds();
        camera.radius = 2.0f * osc::LongestDim(osc::Dimensions(bounds));
        camera.focusPoint = -osc::Midpoint(bounds);
    }

    // returns the 3D position of the intersection between, if any, the user's mouse and the mesh
    std::optional<glm::vec3> RaycastMesh(osc::PolarPerspectiveCamera const& camera, osc::Mesh const& mesh, osc::Rect const& renderRect, glm::vec2 mousePos)
    {
        osc::Line const ray = camera.unprojectTopLeftPosToWorldRay(mousePos - renderRect.p1, osc::Dimensions(renderRect));
        osc::RayCollision const maybeCollision = osc::GetClosestWorldspaceRayCollision(mesh, osc::Transform{}, ray);
        return maybeCollision ? std::optional<glm::vec3>{ray.origin + ray.dir*maybeCollision.distance} : std::nullopt;
    }

    // returns a camera that is focused on the given box
    osc::PolarPerspectiveCamera CreateCameraFocusedOn(osc::Mesh const& mesh)
    {
        osc::PolarPerspectiveCamera rv;
        rv.radius = 3.0f * osc::LongestDim(osc::Dimensions(mesh.getBounds()));
        rv.theta = 0.25f * osc::fpi;
        rv.phi = 0.25f * osc::fpi;
        return rv;
    }
}

// TPS-tab-specific UI stuff
namespace
{
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

    // a "panel" in the UI
    //
    // effectively, this is data that's common to both panel types (input/result)
    struct TPSUIPanel {
        TPSUIPanel(osc::Mesh const& mesh_) : Mesh{mesh_} {}

        osc::Mesh Mesh;
        bool WireframeMode = true;
        osc::PolarPerspectiveCamera Camera = CreateCameraFocusedOn(Mesh);
        osc::SceneRendererParams LastParams;
        std::vector<osc::SceneDecoration> LastDecorations;
        osc::SceneRenderer Renderer;
    };

    // a "user input" panel: i.e. one of the two panels the user is setting landmarks on etc.
    struct TPSUIInputPanel final : public TPSUIPanel {
        using TPSUIPanel::TPSUIPanel;

        float LandmarkRadius = 0.05f;
        std::unordered_map<size_t, glm::vec3> Landmarks;
    };

    // a "result" panel: i.e. the rightmost panel containing the computed result
    struct TPSUIResultPanel final : public TPSUIPanel {
        using TPSUIPanel::TPSUIPanel;

        float BlendingFactor = 1.0f;
        bool ShowDestinationMesh = false;
        TPSInputs3D LastTPSInputs;
        TPSCoefficients3D LastCoefficients;
        osc::Mesh LastInputMesh;
    };

    // returns `true` if `results.LastTPSInputs` differs from the latest set of TPS inputs derived from the arguments
    bool UpdateTPSInputsIfNecessary(TPSUIInputPanel const& src, TPSUIInputPanel const& dest, TPSUIResultPanel& results)
    {
        TPSInputs3D newInputs
        {
            GetLandmarkPairs(src.Landmarks, dest.Landmarks),
            results.BlendingFactor,
        };

        if (newInputs != results.LastTPSInputs)
        {
            results.LastTPSInputs = std::move(newInputs);
            return true;
        }
        else
        {
            return false;  // no change in the inputs
        }
    }

    // returns `true` if `results.LastCoefficients` differs from the latest set of coefficients derived from the arguments
    bool UpdateTPSCoefficientsIfNecessary(TPSUIInputPanel const& src, TPSUIInputPanel const& dest, TPSUIResultPanel& results)
    {
        if (!UpdateTPSInputsIfNecessary(src, dest, results))
        {
            // cache: the inputs have not been updated, so the coefficients will not change
            return false;
        }

        TPSCoefficients3D newCoefficients = CalcCoefficients(results.LastTPSInputs);

        if (newCoefficients != results.LastCoefficients)
        {
            results.LastCoefficients = std::move(newCoefficients);
            return true;
        }
        else
        {
            return false;  // no change in the coefficients
        }
    }

    bool UpdateWarpedMeshIfNecessary(TPSUIInputPanel const& src, TPSUIInputPanel const& dest, TPSUIResultPanel& results)
    {
        bool const coefficientsChanged = UpdateTPSCoefficientsIfNecessary(src, dest, results);
        bool const meshChanged = src.Mesh != results.LastInputMesh;

        if (coefficientsChanged || meshChanged)
        {
            results.Mesh = ApplyThinPlateWarpToMesh(results.LastCoefficients, src.Mesh);
            results.LastInputMesh = src.Mesh;
            return true;
        }
        else
        {
            return false;
        }
    }

    // the entire user-editable TPS "scene"
    struct TPSUIScene final {
        TPSUIScene()
        {
            UpdateWarpedMeshIfNecessary(SrcPanel, DestPanel, ResultPanel);
        }

        TPSUIInputPanel SrcPanel{osc::GenUntexturedUVSphere(16, 16)};
        TPSUIInputPanel DestPanel{osc::GenUntexturedSimbodyCylinder(16)};
        TPSUIResultPanel ResultPanel{SrcPanel.Mesh};
        std::optional<osc::PolarPerspectiveCamera> MaybeLockedCameraBase = SrcPanel.Camera;
        osc::Material WireframeMaterial = CreateWireframeOverlayMaterial();
        std::shared_ptr<osc::Mesh const> LandmarkSphere = osc::App::meshes().getSphereMesh();
        glm::vec4 LandmarkColor = {1.0f, 0.0f, 0.0f, 1.0f};
    };

    void AppendCommonDecorations(TPSUIScene const& scene, TPSUIPanel const& panel, std::vector<osc::SceneDecoration>& out)
    {
        out.reserve(out.size() + 5);  // likely guess

        // draw the mesh
        out.emplace_back(panel.Mesh);

        // if requested, also draw wireframe overlays for the mesh
        if (panel.WireframeMode)
        {
            osc::SceneDecoration& dec = out.emplace_back(panel.Mesh);
            dec.maybeMaterial = scene.WireframeMaterial;
        }

        // add grid decorations
        DrawXZGrid(out);
        DrawXZFloorLines(out, 100.0f);
    }

    // returns 3D decorations for the given input panel
    std::vector<osc::SceneDecoration> GenerateDecorations(TPSUIScene const& scene, TPSUIInputPanel const& inputPanel)
    {
        // generate in-scene 3D decorations
        std::vector<osc::SceneDecoration> decorations;
        decorations.reserve(5 + inputPanel.Landmarks.size());  // likely guess

        AppendCommonDecorations(scene, inputPanel, decorations);

        // append each landmark as a sphere
        for (auto const& [id, pos] : inputPanel.Landmarks)
        {
            osc::Transform transform{};
            transform.scale *= inputPanel.LandmarkRadius;
            transform.position = pos;

            decorations.emplace_back(scene.LandmarkSphere, transform, scene.LandmarkColor);
        }

        return decorations;
    }

    // returns 3D decorations for the given result panel
    std::vector<osc::SceneDecoration> GenerateDecorations(TPSUIScene const& scene, TPSUIResultPanel const& resultPanel)
    {
        // generate in-scene 3D decorations
        std::vector<osc::SceneDecoration> decorations;
        decorations.reserve(5);  // likely guess

        AppendCommonDecorations(scene, resultPanel, decorations);

        if (resultPanel.ShowDestinationMesh)
        {
            osc::SceneDecoration& dec = decorations.emplace_back(scene.DestPanel.Mesh);
            dec.color = {1.0f, 0.0f, 0.0f, 0.5f};
        }

        return decorations;
    }

    // returns scene rendering parameters for an generic panel
    osc::SceneRendererParams CalcRenderParams(TPSUIScene const& scene, TPSUIPanel const& panel, glm::vec2 dims)
    {
        osc::SceneRendererParams rv;
        rv.drawFloor = false;
        rv.backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        rv.dimensions = dims;
        rv.viewMatrix = panel.Camera.getViewMtx();
        rv.projectionMatrix = panel.Camera.getProjMtx(osc::AspectRatio(dims));
        rv.samples = osc::App::get().getMSXAASamplesRecommended();
        rv.lightDirection = osc::RecommendedLightDirection(panel.Camera);
        return rv;
    }

    // renders a panel to a texture via its renderer and returns a reference to the rendered texture
    template<typename TPanel>
    osc::RenderTexture& RenderPanelTexture(TPSUIScene const& scene, TPanel& panel, glm::vec2 dims)
    {
        osc::SceneRendererParams const params = CalcRenderParams(scene, panel, dims);
        std::vector<osc::SceneDecoration> const decorations = GenerateDecorations(scene, panel);

        if (params == panel.LastParams && decorations == panel.LastDecorations)
        {
            return panel.Renderer.updRenderTexture();  // caching: no need to redraw if the inputs haven't changed
        }

        // else: update cache and redraw
        panel.Renderer.draw(decorations, params);
        panel.LastParams = std::move(params);
        panel.LastDecorations = std::move(decorations);

        return panel.Renderer.updRenderTexture();
    }

    // "tick" the scene forward (i.e. perform any updates)
    void Tick(TPSUIScene& scene)
    {
        if (scene.MaybeLockedCameraBase)
        {
            osc::PolarPerspectiveCamera& baseCamera = *scene.MaybeLockedCameraBase;

            if (scene.SrcPanel.Camera != baseCamera)
            {
                baseCamera = scene.SrcPanel.Camera;
                scene.DestPanel.Camera = baseCamera;
                scene.ResultPanel.Camera = baseCamera;
            }
            else if (scene.DestPanel.Camera != baseCamera)
            {
                baseCamera = scene.DestPanel.Camera;
                scene.SrcPanel.Camera = baseCamera;
                scene.ResultPanel.Camera = baseCamera;
            }
            else if (scene.ResultPanel.Camera != baseCamera)
            {
                baseCamera = scene.ResultPanel.Camera;
                scene.SrcPanel.Camera = baseCamera;
                scene.DestPanel.Camera = baseCamera;
            }
        }
    }

    void ActionAddLandmark(TPSUIScene& scene, TPSUIInputPanel& panel, glm::vec3 const& pos)
    {
        panel.Landmarks[panel.Landmarks.size()] = pos;
        UpdateWarpedMeshIfNecessary(scene.SrcPanel, scene.DestPanel, scene.ResultPanel);
    }

    void ActionBrowseForMesh(TPSUIScene& scene, TPSUIInputPanel& panel)
    {
        std::filesystem::path const p = osc::PromptUserForFile("vtp,obj");
        if (!p.empty())
        {
            panel.Mesh = osc::LoadMeshViaSimTK(p);
            UpdateWarpedMeshIfNecessary(scene.SrcPanel, scene.DestPanel, scene.ResultPanel);
        }
    }

    void ActionSetLandmarkRadius(TPSUIInputPanel& panel, float radius)
    {
        panel.LandmarkRadius = radius;
    }

    void ActionSetBlendFactor(TPSUIScene const& scene, TPSUIResultPanel& panel, float factor)
    {
        panel.BlendingFactor = factor;
        UpdateWarpedMeshIfNecessary(scene.SrcPanel, scene.DestPanel, panel);
    }

    void ActionSetShowDestinationMesh(TPSUIResultPanel& panel, bool showDestinationMesh)
    {
        panel.ShowDestinationMesh = showDestinationMesh;
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
        InputPanel(std::string_view panelName_, std::shared_ptr<TPSUIScene> scene_, bool useSrc_) :
            TPS3DTabPanel{std::move(panelName_), ImGuiDockNodeFlags_PassthruCentralNode},
            m_Scene{std::move(scene_)},
            m_PanelData{useSrc_ ? &m_Scene->SrcPanel : &m_Scene->DestPanel}
        {
            OSC_ASSERT(m_Scene != nullptr && "the input panel requires a valid scene");
        }
    private:
        void implDraw() override
        {
            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = RenderPanelTexture(*m_Scene, *m_PanelData, dims);
            osc::ImGuiImageHittestResult const htResult = osc::DrawTextureAsImGuiImageAndHittest(renderTexture);

            // update camera if user drags it around etc.
            if (htResult.isHovered)
            {
                osc::UpdatePolarCameraFromImGuiUserInput(dims, m_PanelData->Camera);
            }

            // if user left-clicks, add a landmark there
            if (htResult.isLeftClickReleasedWithoutDragging)
            {
                glm::vec2 const mousePos = ImGui::GetMousePos();

                // if the user's mouse ray intersects the mesh, then that's where the landmark should be placed
                if (std::optional<glm::vec3> maybeCollision = RaycastMesh(m_PanelData->Camera, m_PanelData->Mesh, htResult.rect, mousePos))
                {
                    ActionAddLandmark(*m_Scene, *m_PanelData, *maybeCollision);
                }
            }

            // draw any overlays
            drawOverlays(htResult.rect);
        }

        void drawOverlays(osc::Rect const& renderRect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            glm::vec2 const padding = {10.0f, 10.0f};
            glm::vec2 const originalCursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(renderRect.p1 + padding);
            OSC_SCOPE_GUARD({ ImGui::SetCursorScreenPos(originalCursorPos); });

            // ImGui: draw "browse for new mesh" button
            if (ImGui::Button("browse"))
            {
                ActionBrowseForMesh(*m_Scene, *m_PanelData);
            }

            ImGui::SameLine();

            // ImGui: draw "autofit camera" button
            if (ImGui::Button(ICON_FA_EXPAND))
            {
                AutofocusCameraOnMesh(m_PanelData->Mesh, m_PanelData->Camera);
            }

            ImGui::SameLine();

            // ImGui: draw landmark radius editor
            {
                char const* label = "landmark radius";
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x - 10.0f);

                float radius = m_PanelData->LandmarkRadius;
                if (ImGui::SliderFloat(label, &radius, 0.0001f, 1.0f, "%.4f", 2.0f))
                {
                    ActionSetLandmarkRadius(*m_PanelData, radius);
                }
            }
        }

        std::shared_ptr<TPSUIScene> m_Scene;
        TPSUIInputPanel* m_PanelData = nullptr;
    };

    // a "result" panel (i.e. after applying a warp to the source)
    class ResultPanel final : public TPS3DTabPanel {
    public:
        ResultPanel(std::string_view panelName_, std::shared_ptr<TPSUIScene> scene_) :
            TPS3DTabPanel{std::move(panelName_)},
            m_Scene{std::move(scene_)}
        {
            OSC_ASSERT(m_Scene != nullptr && "the results panel must be given a scene");
        }

    private:
        void implDraw() override
        {
            // fill the entire available region with the render
            glm::vec2 const dims = ImGui::GetContentRegionAvail();

            // render it via ImGui and hittest it
            osc::RenderTexture& renderTexture = RenderPanelTexture(*m_Scene, m_Scene->ResultPanel, dims);
            osc::ImGuiImageHittestResult const htResult = osc::DrawTextureAsImGuiImageAndHittest(renderTexture);

            // update camera if user drags it around etc.
            if (htResult.isHovered)
            {
                osc::UpdatePolarCameraFromImGuiUserInput(dims, m_Scene->ResultPanel.Camera);
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
                    bool showDestinationMesh = m_Scene->ResultPanel.ShowDestinationMesh;
                    if (ImGui::Checkbox("show destination", &showDestinationMesh))
                    {
                        ActionSetShowDestinationMesh(m_Scene->ResultPanel, showDestinationMesh);
                    }
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
                float factor = m_Scene->ResultPanel.BlendingFactor;
                if (ImGui::SliderFloat("blending factor", &factor, 0.0f, 1.0f))
                {
                    ActionSetBlendFactor(*m_Scene, m_Scene->ResultPanel, factor);
                }
                ImGui::SetCursorScreenPos(oldCursorPos);
            }
        }

        std::shared_ptr<TPSUIScene> m_Scene;
    };
}

// tab implementation
class osc::TPS3DTab::Impl final {
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
        Tick(*m_Scene);
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_SourcePanel.draw();
        m_DestPanel.draw();
        m_ResultPanel.draw();
        m_LogViewerPanel.draw();
        m_PerfPanel.draw();
    }

private:

    // tab data
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " TPS3DTab";
    TabHost* m_Parent;

    // the scene (document) the user is editing
    std::shared_ptr<TPSUIScene> m_Scene = std::make_shared<TPSUIScene>();

    // user-visible panels
    InputPanel m_SourcePanel{"Source Mesh", m_Scene, true};
    InputPanel m_DestPanel{"Destination Mesh", m_Scene, false};
    ResultPanel m_ResultPanel{"Result", m_Scene};
    LogViewerPanel m_LogViewerPanel{"Log"};
    PerfPanel m_PerfPanel{"Performance"};
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
