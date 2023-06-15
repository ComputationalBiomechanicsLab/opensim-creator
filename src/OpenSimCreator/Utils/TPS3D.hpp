#pragma once

#include <oscar/Graphics/Mesh.hpp>

#include <glm/vec3.hpp>

#include <filesystem>
#include <iosfwd>
#include <utility>
#include <vector>

// core 3D TPS algorithm code
//
// most of the background behind this is discussed in issue #467. For redundancy's sake, here
// are some of the references used to write this implementation:
//
// - primary literature source: https://ieeexplore.ieee.org/document/24792
// - blog explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
// - blog explanation #2: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
namespace osc
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    struct LandmarkPair3D final {

        LandmarkPair3D() = default;

        LandmarkPair3D(
            glm::vec3 const& source_,
            glm::vec3 const& destination_) :

            source{source_},
            destination{destination_}
        {
        }

        glm::vec3 source;
        glm::vec3 destination;
    };

    bool operator==(LandmarkPair3D const&, LandmarkPair3D const&) noexcept;
    bool operator!=(LandmarkPair3D const&, LandmarkPair3D const&) noexcept;
    std::ostream& operator<<(std::ostream&, LandmarkPair3D const&);

    // required inputs to the 3D TPS algorithm
    //
    // these are supplied by the user and used to solve for the coefficients
    struct TPSCoefficientSolverInputs3D final {

        TPSCoefficientSolverInputs3D() = default;

        TPSCoefficientSolverInputs3D(
            std::vector<LandmarkPair3D> landmarks_,
            float blendingFactor_) :

            landmarks{std::move(landmarks_)},
            blendingFactor{std::move(blendingFactor_)}
        {
        }

        std::vector<LandmarkPair3D> landmarks;
        float blendingFactor = 1.0f;
    };

    bool operator==(TPSCoefficientSolverInputs3D const&, TPSCoefficientSolverInputs3D const&) noexcept;
    bool operator!=(TPSCoefficientSolverInputs3D const&, TPSCoefficientSolverInputs3D const&) noexcept;
    std::ostream& operator<<(std::ostream&, TPSCoefficientSolverInputs3D const&);

    // a single non-affine term of the 3D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    struct TPSNonAffineTerm3D final {

        TPSNonAffineTerm3D(
            glm::vec3 const& weight_,
            glm::vec3 const& controlPoint_) :

            weight{weight_},
            controlPoint{controlPoint_}
        {
        }

        glm::vec3 weight;
        glm::vec3 controlPoint;
    };

    bool operator==(TPSNonAffineTerm3D const&, TPSNonAffineTerm3D const&) noexcept;
    bool operator!=(TPSNonAffineTerm3D const&, TPSNonAffineTerm3D const&) noexcept;
    std::ostream& operator<<(std::ostream&, TPSNonAffineTerm3D const&);

    // all coefficients in the 3D TPS equation
    //
    // i.e. these are the a1, a2, a3, a4, and w's (+ control points) terms of the equation
    struct TPSCoefficients3D final {

        // default the coefficients to an "identity" warp
        glm::vec3 a1 = {0.0f, 0.0f, 0.0f};
        glm::vec3 a2 = {1.0f, 0.0f, 0.0f};
        glm::vec3 a3 = {0.0f, 1.0f, 0.0f};
        glm::vec3 a4 = {0.0f, 0.0f, 1.0f};
        std::vector<TPSNonAffineTerm3D> nonAffineTerms;
    };

    bool operator==(TPSCoefficients3D const&, TPSCoefficients3D const&) noexcept;
    bool operator!=(TPSCoefficients3D const&, TPSCoefficients3D const&) noexcept;
    std::ostream& operator<<(std::ostream&, TPSCoefficients3D const&);

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D CalcCoefficients(TPSCoefficientSolverInputs3D const&);

    // evaluates the TPS equation with the given coefficients and input point
    glm::vec3 EvaluateTPSEquation(TPSCoefficients3D const&, glm::vec3);

    // returns a mesh that is the equivalent of applying the 3D TPS warp to the mesh
    osc::Mesh ApplyThinPlateWarpToMesh(TPSCoefficients3D const& coefs, osc::Mesh const&);

    // returns 3D landmark positions loaded from a CSV (.landmarks) file
    std::vector<glm::vec3> LoadLandmarksFromCSVFile(std::filesystem::path const&);
}