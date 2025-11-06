#pragma once

#include <libopynsim/Shims/Cpp23/mdspan.h>
#include <libopynsim/Utils/LandmarkPair3D.h>
#include <SimTKcommon/SmallMatrix.h>

#include <concepts>
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
namespace opyn
{
    // required inputs to the 3D TPS algorithm
    //
    // these are supplied by the user and used to solve for the coefficients
    template<std::floating_point T>
    struct TPSCoefficientSolverInputs3D final {

        TPSCoefficientSolverInputs3D() = default;

        explicit TPSCoefficientSolverInputs3D(std::vector<LandmarkPair3D<T>> landmarks_) :
            landmarks{std::move(landmarks_)}
        {}

        friend bool operator==(const TPSCoefficientSolverInputs3D&, const TPSCoefficientSolverInputs3D&) = default;

        std::vector<LandmarkPair3D<T>> landmarks;
        bool applyAffineTranslation = true;
        bool applyAffineScale = true;
        bool applyAffineRotation = true;
        bool applyNonAffineWarp = true;
    };

    std::ostream& operator<<(std::ostream&, const TPSCoefficientSolverInputs3D<float>&);
    std::ostream& operator<<(std::ostream&, const TPSCoefficientSolverInputs3D<double>&);

    // a single non-affine term of the 3D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    template<std::floating_point T>
    struct TPSNonAffineTerm3D final {

        TPSNonAffineTerm3D(
            const SimTK::Vec<3, T>& weight_,
            const SimTK::Vec<3, T>& controlPoint_) :

            weight{weight_},
            controlPoint{controlPoint_}
        {}

        friend bool operator==(const TPSNonAffineTerm3D&, const TPSNonAffineTerm3D&) = default;

        SimTK::Vec<3, T> weight;
        SimTK::Vec<3, T> controlPoint;
    };

    std::ostream& operator<<(std::ostream&, const TPSNonAffineTerm3D<float>&);
    std::ostream& operator<<(std::ostream&, const TPSNonAffineTerm3D<double>&);

    // all coefficients in the 3D TPS equation
    //
    // i.e. these are the a1, a2, a3, a4, and w's (+ control points) terms of the equation
    template<std::floating_point T>
    struct TPSCoefficients3D final {
        friend bool operator==(const TPSCoefficients3D&, const TPSCoefficients3D&) = default;

        // default the coefficients to an "identity" warp
        SimTK::Vec<3, T> a1 = {T{0.0}, T{0.0}, T{0.0}};
        SimTK::Vec<3, T> a2 = {T{1.0}, T{0.0}, T{0.0}};
        SimTK::Vec<3, T> a3 = {T{0.0}, T{1.0}, T{0.0}};
        SimTK::Vec<3, T> a4 = {T{0.0}, T{0.0}, T{1.0}};
        std::vector<TPSNonAffineTerm3D<T>> nonAffineTerms;
    };

    std::ostream& operator<<(std::ostream&, const TPSCoefficients3D<float>&);
    std::ostream& operator<<(std::ostream&, const TPSCoefficients3D<double>&);

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D<float> TPSCalcCoefficients(const TPSCoefficientSolverInputs3D<float>&);
    TPSCoefficients3D<double> TPSCalcCoefficients(const TPSCoefficientSolverInputs3D<double>&);
    TPSCoefficients3D<double> TPSCalcCoefficients(
        cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride>,
        cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride>
    );

    // evaluates the TPS equation with the given coefficients and input point
    SimTK::Vec<3, float>  TPSWarpPoint(const TPSCoefficients3D<float>&, SimTK::Vec<3, float>);
    SimTK::Vec<3, double> TPSWarpPoint(const TPSCoefficients3D<double>&, SimTK::Vec<3, double>);

    // evaluates the TPS equation with the given coefficients and input point, lerping the result
    // by `blendingFactor` between the input point and the "fully warped" point.
    SimTK::Vec<3, float> TPSWarpPoint(const TPSCoefficients3D<float>&, SimTK::Vec<3, float>, float blendingFactor);

    // returns points that are the equivalent of applying the 3D TPS warp to each input point
    std::vector<SimTK::Vec<3, float>> TPSWarpPoints(const TPSCoefficients3D<float>&, std::span<const SimTK::Vec<3, float>>, float blendingFactor);

    // applies the 3D TPS warp in-place to each SimTK::Vec3 in the provided span
    void TPSWarpPointsInPlace(const TPSCoefficients3D<float>&, std::span<SimTK::Vec<3, float>>, float blendingFactor);
}
