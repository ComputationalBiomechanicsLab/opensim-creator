#pragma once

#include <OpenSimCreator/Utils/LandmarkPair3D.h>

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>

#include <iosfwd>
#include <span>
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
    // required inputs to the 3D TPS algorithm
    //
    // these are supplied by the user and used to solve for the coefficients
    struct TPSCoefficientSolverInputs3D final {

        TPSCoefficientSolverInputs3D() = default;

        explicit TPSCoefficientSolverInputs3D(std::vector<LandmarkPair3D> landmarks_) :
            landmarks{std::move(landmarks_)}
        {}

        friend bool operator==(const TPSCoefficientSolverInputs3D&, const TPSCoefficientSolverInputs3D&) = default;

        std::vector<LandmarkPair3D> landmarks;
    };

    std::ostream& operator<<(std::ostream&, const TPSCoefficientSolverInputs3D&);

    // a single non-affine term of the 3D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    struct TPSNonAffineTerm3D final {

        TPSNonAffineTerm3D(
            const Vec3& weight_,
            const Vec3& controlPoint_) :

            weight{weight_},
            controlPoint{controlPoint_}
        {}

        friend bool operator==(const TPSNonAffineTerm3D&, const TPSNonAffineTerm3D&) = default;

        Vec3 weight;
        Vec3 controlPoint;
    };

    std::ostream& operator<<(std::ostream&, const TPSNonAffineTerm3D&);

    // all coefficients in the 3D TPS equation
    //
    // i.e. these are the a1, a2, a3, a4, and w's (+ control points) terms of the equation
    struct TPSCoefficients3D final {
        friend bool operator==(const TPSCoefficients3D&, const TPSCoefficients3D&) = default;

        // default the coefficients to an "identity" warp
        Vec3 a1 = {0.0f, 0.0f, 0.0f};
        Vec3 a2 = {1.0f, 0.0f, 0.0f};
        Vec3 a3 = {0.0f, 1.0f, 0.0f};
        Vec3 a4 = {0.0f, 0.0f, 1.0f};
        std::vector<TPSNonAffineTerm3D> nonAffineTerms;
    };

    std::ostream& operator<<(std::ostream&, const TPSCoefficients3D&);

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D CalcCoefficients(const TPSCoefficientSolverInputs3D&);

    // evaluates the TPS equation with the given coefficients and input point
    Vec3 EvaluateTPSEquation(const TPSCoefficients3D&, Vec3);

    // returns a mesh that is the equivalent of applying the 3D TPS warp to the mesh
    Mesh ApplyThinPlateWarpToMesh(const TPSCoefficients3D&, const Mesh&, float blendingFactor);

    // returns points that are the equivalent of applying the 3D TPS warp to each input point
    std::vector<Vec3> ApplyThinPlateWarpToPoints(const TPSCoefficients3D&, std::span<const Vec3>, float blendingFactor);

    // applies the 3D TPS warp in-place to each SimTK::Vec3 in the provided span
    void ApplyThinPlateWarpToPointsInPlace(const TPSCoefficients3D&, std::span<Vec3>, float blendingFactor);
}
