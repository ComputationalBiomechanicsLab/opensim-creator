#pragma once

#include <libopynsim/shims/cpp23/mdspan.h>
#include <libopynsim/utilities/landmark_pair_3d.h>

#include <SimTKcommon/SmallMatrix.h>
#include <liboscar/graphics/mesh.h>

#include <concepts>
#include <iosfwd>
#include <span>
#include <utility>
#include <vector>

// Thin Plate Spline (TPS), 3D implementation.
//
// Most of the background behind this is discussed in:
//
// - https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/467
//
// Here are links to some of the reference material used to write this implementation:
//
// - Primary literature source: https://ieeexplore.ieee.org/document/24792
// - Blog explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
// - Blog explanation #2: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
namespace opyn
{
    // Represents the inputs of the Thin-Plate Spline (TPS) warping algorithm.
    //
    // These are supplied by the caller to solve the necessary TPS coefficients.
    template<std::floating_point T>
    struct TPSCoefficientSolverInputs3D final {

        TPSCoefficientSolverInputs3D() = default;

        explicit TPSCoefficientSolverInputs3D(std::vector<landmark_pair_3d<T>> landmarks_) :
            landmarks{std::move(landmarks_)}
        {}

        friend bool operator==(const TPSCoefficientSolverInputs3D&, const TPSCoefficientSolverInputs3D&) = default;

        // A sequence of source-to-destination point pairs in 3D that the TPS
        // warping algorithm is trying to fit a warping equation to.
        std::vector<landmark_pair_3d<T>> landmarks;

        // Set this to `true` if the resulting warping equation should translate
        // points in the source coordinate system to the destination coordinate
        // system (i.e. enable/disable writing `a1`).
        bool apply_affine_translation = true;

        // Set this to `true` if the resulting warping equation should scale
        // points in the source coordinate system to the destination coordinate
        // system (i.e. disable/enable normalizing `a2`-`a4`).
        bool apply_affine_scale = true;

        // Set this to `true` if the resulting warping equation should rotate
        // points in the source coordinate system to the destination coordinate
        // system (i.e. disable/enable the change-of-basis part of `a2`-`a4`).
        bool apply_affine_rotation = true;

        // Set this to `true` if the resulting warping equation should apply
        // non-affine warping to points in the source coordinate system when
        // mapping to the destination coordinate system (i.e. "the bendy parts"
        // of the warp, or `non_affine_terms`.
        bool apply_non_affine_warp = true;
    };

    // Pretty-prints solver inputs for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSCoefficientSolverInputs3D<float>&);

    // Pretty-prints solver inputs for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSCoefficientSolverInputs3D<double>&);

    // Represents a non-affine term of the 3D Thin-Plate Spline (TPS) equation.
    //
    // In the literature, the TPS warping equation is usually written:
    //
    //     f(p) = a1 + a2*p.x + a3*p.y + a4*p.z + SUM{ wi * U(||control_point - p||) }
    //
    // This class encodes the `wi` and `control_point` parts of that equation. It can
    // be colloquially thought of as the "non-affine" or "bendy" parts of the warping
    // operation.
    template<std::floating_point T>
    struct TPSNonAffineTerm3D final {

        explicit TPSNonAffineTerm3D(
            const SimTK::Vec<3, T>& weight_,
            const SimTK::Vec<3, T>& control_point_) :

            weight{weight_},
            control_point{control_point_}
        {}

        friend bool operator==(const TPSNonAffineTerm3D&, const TPSNonAffineTerm3D&) = default;

        SimTK::Vec<3, T> weight;
        SimTK::Vec<3, T> control_point;
    };

    // Pretty-prints a non-affine term for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSNonAffineTerm3D<float>&);

    // Pretty-prints a non-affine term for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSNonAffineTerm3D<double>&);

    // Represents all the coefficients of a 3D Thin-Plate Spline (TPS) point
    // warping equation.
    //
    // In the literature, these are usually represented as the `a1`, `a2`, `a3`,
    // `a4`, and `{w, control_point}` terms. These coefficients can be used to
    // warp a point in the source coordinate system into the destination coordinate
    // system.
    template<std::floating_point T>
    struct TPSCoefficients3D final {

        // Constructs the coefficients of an identity warping operation.
        TPSCoefficients3D() = default;

        friend bool operator==(const TPSCoefficients3D&, const TPSCoefficients3D&) = default;

        SimTK::Vec<3, T> a1 = {T{0.0}, T{0.0}, T{0.0}};
        SimTK::Vec<3, T> a2 = {T{1.0}, T{0.0}, T{0.0}};
        SimTK::Vec<3, T> a3 = {T{0.0}, T{1.0}, T{0.0}};
        SimTK::Vec<3, T> a4 = {T{0.0}, T{0.0}, T{1.0}};
        std::vector<TPSNonAffineTerm3D<T>> non_affine_terms;
    };

    // Pretty-prints the coefficients for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSCoefficients3D<float>&);

    // Pretty-prints the coefficients for readability/debugging.
    std::ostream& operator<<(std::ostream&, const TPSCoefficients3D<double>&);

    // computes all coefficients of the 3D TPS equation (a1, a2, a3, a4, and all the w's)
    TPSCoefficients3D<float> tps3d_solve_coefficients(const TPSCoefficientSolverInputs3D<float>&);
    TPSCoefficients3D<double> tps3d_solve_coefficients(const TPSCoefficientSolverInputs3D<double>&);
    TPSCoefficients3D<double> tps3d_solve_coefficients(
        cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride>,
        cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride>
    );

    // Evaluates the 3D Thin-Plate Spline (TPS) point warping equation for a single point.
    SimTK::Vec<3, float>  tps3d_warp_point(
        const TPSCoefficients3D<float>&,
        SimTK::Vec<3, float>
    );

    // Evaluates the 3D Thin-Plate Spline (TPS) point warping equation for a single point.
    SimTK::Vec<3, double> tps3d_warp_point(
        const TPSCoefficients3D<double>&,
        SimTK::Vec<3, double>
    );

    // Evaluates the 3D Thin-Plate Spline (TPS) point warping equation for a single point
    // and linearly interpolates between the source point and the warped point by
    // `linear_interpolant`.
    SimTK::Vec<3, float> tps3d_warp_point(
        const TPSCoefficients3D<float>&,
        SimTK::Vec<3, float>,
        float linear_interpolant
    );

    // returns points that are the equivalent of applying the 3D TPS warp to each input point
    std::vector<SimTK::Vec<3, float>> tps3d_warp_points(
        const TPSCoefficients3D<float>&,
        std::span<const SimTK::Vec<3, float>>,
        float linear_interpolant
    );

    // applies the 3D TPS warp in-place to each SimTK::Vec3 in the provided span
    void tps3d_warp_points_in_place(
        const TPSCoefficients3D<float>&,
        std::span<SimTK::Vec<3, float>>,
        float linear_interpolant
    );

    osc::Mesh tps3d_warp_mesh(TPSCoefficients3D<float>&, const osc::Mesh&, float linear_interpolant);
}
