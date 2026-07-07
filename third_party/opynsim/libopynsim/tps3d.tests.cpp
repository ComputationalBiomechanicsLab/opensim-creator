#include "tps3d.h"

#include <libopynsim/utilities/simbody_x_oscar.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <numbers>
#include <optional>
#include <vector>

using namespace opyn;

namespace
{
    // Returns the `i`th (zero-indexed) point of a set of `n` uniformly distributed
    // points on the surface of a sphere with radius `radius` using the Fibonacci
    // lattice technique.
    template<std::floating_point T = double>
    SimTK::Vec<3, T> generate_ith_fibonnaci_sphere_point(size_t i, size_t n, T radius = static_cast<T>(1.0))
    {
        const T fi = static_cast<T>(i);
        const T fn = static_cast<T>(n);

        // Map index linearly to y-axis from +radius (North Pole) to -radius (South Pole)
        // Using a slight offset to prevent points from sitting exactly on the mathematical poles
        const T y = radius * (T(1.0) - ((fi / (fn - T(1.0))) * T(2.0)));

        // Calculate radius of the horizontal cross-section circle at height y
        const T radius_at_y = std::sqrt(std::max(T(0.0), (radius * radius) - (y * y)));

        const T golden_ratio_conjugate_increment = std::numbers::pi_v<T> * (T(3.0) - std::sqrt(T(5.0)));
        const T theta = fi * golden_ratio_conjugate_increment;
        const T x = radius_at_y * std::cos(theta);
        const T z = radius_at_y * std::sin(theta);

        return {x, y, z};
    }

    // Returns a point that lies of the surface of a cube of `half_side_length`
    // dimensions. The point is created by projecting `p` onto the cube's
    // surface. `std::nullopt` is returned if the point is un-projectable.
    template<std::floating_point T>
    std::optional<SimTK::Vec<3, T>> project_onto_cube_surface(const SimTK::Vec<3, T>& p, T half_side_length = static_cast<T>(1.0))
    {
        const auto pa = p.abs();
        const T longest_axis = std::max({pa[0], pa[1], pa[2]});
        if (longest_axis <= 1e-6f) {
            return std::nullopt;  // Unprojectable
        }

        return (half_side_length / longest_axis) * p;
    }

    std::vector<LandmarkPair3D<double>> generate_identity_sphere_pairs(size_t n = 64)
    {
        std::vector<LandmarkPair3D<double>> rv;
        rv.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const auto p = generate_ith_fibonnaci_sphere_point(i, n);
            rv.push_back({.source = p, .destination = p});
        }
        return rv;
    }

    std::vector<LandmarkPair3D<double>> generate_sphere_to_cube_landmark_pairs(size_t n = 64)
    {
        std::vector<LandmarkPair3D<double>> rv;
        rv.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const auto sphere_point = generate_ith_fibonnaci_sphere_point(i, n);
            const auto cube_point = project_onto_cube_surface(sphere_point).value();
            rv.push_back({.source = sphere_point, .destination = cube_point});
        }
        return rv;
    }

    bool equal_within_absdiff(const SimTK::Vec3& lhs, const SimTK::Vec3& rhs, double absdiff)
    {
        for (int i = 0; i < SimTK::Vec3::size(); ++i) {
            if (not (std::abs(lhs[i] - rhs[i]) <= absdiff)) {
                return false;
            }
        }
        return true;
    }

    // Helper struct: pairs a rigid `SimTK::Transform` with
    // an orthogonal scale factor.
    struct SimbodyAffineTransform final {
        SimTK::Vec3 scale{0.0};
        SimTK::Transform transform;

        SimTK::Vec3 operator*(const SimTK::Vec3& rhs) const
        {
            SimTK::Vec3 rv{rhs};
            rv[0] = scale[0] * rv[0];
            rv[1] = scale[1] * rv[1];
            rv[2] = scale[2] * rv[2];
            rv = transform * rv;
            return rv;
        }
    };

    SimbodyAffineTransform extract_affine_transform(const TPSCoefficients3D<double>& coefs)
    {
        SimTK::Vec3 scale = {coefs.a2.norm(), coefs.a3.norm(), coefs.a4.norm()};
        SimTK::Mat33 rotation_matrix{};
        rotation_matrix.col(0) = coefs.a2.normalize();
        rotation_matrix.col(1) = coefs.a3.normalize();
        rotation_matrix.col(2) = coefs.a4.normalize();
        SimTK::Vec3 translation = coefs.a1;

        return {
            .scale = scale,
            .transform = {SimTK::Rotation{rotation_matrix}, translation},
        };
    }

    double angular_rotation_error(const SimTK::Rotation& truth, const SimTK::Rotation& model)
    {
        const auto delta_rotation = (model * truth.invert()).asMat33();
        const double trace = delta_rotation(0, 0) + delta_rotation(1, 1) + delta_rotation(2, 2);
        const double cos_theta = std::clamp((trace - 1.0) / 2.0, -1.0, 1.0);
        const double angle_radians = std::acos(cos_theta);
        return angle_radians * (180.0 / std::numbers::pi_v<double>);
    }

    // Represents the transform error between two affine transforms.
    struct ParameterSpaceError final {
        ParameterSpaceError(const SimbodyAffineTransform& truth, const SimbodyAffineTransform& model) :
            scale{(model.scale - truth.scale).norm()},
            rotation{angular_rotation_error(truth.transform.R(), model.transform.R())},
            translation((truth.transform.p() - model.transform.p()).norm())
        {}

        double scale;
        double rotation;
        double translation;
    };
}

TEST(tps3d, solving_coefficients_on_identical_points_produces_an_identity_warp)
{
    const TPSCoefficientSolverInputs3D<double> inputs{generate_identity_sphere_pairs()};
    const TPSCoefficients3D<double> coefficients = tps3d_solve_coefficients(inputs);

    for (size_t i = 0; i < inputs.landmarks.size(); ++i) {
        const auto input_point = generate_ith_fibonnaci_sphere_point(i + inputs.landmarks.size(), 2*inputs.landmarks.size());
        const auto output_point = opyn::tps3d_warp_point(coefficients, input_point);
        ASSERT_TRUE(equal_within_absdiff(input_point, output_point, 10.0*std::numeric_limits<double>::epsilon()));
    }
}

TEST(tps3d, solving_coefficients_on_sphere_to_cube_warp_can_warp_within_reasonable_r2)
{
    const TPSCoefficientSolverInputs3D<double> inputs{generate_sphere_to_cube_landmark_pairs()};
    const TPSCoefficients3D<double> coefficients = tps3d_solve_coefficients(inputs);

    double sse = 0.0;  // Sum of Squared Errors (SSE)
    double sst = 0.0;  // Total sum of squares (SST, relative to origin center)
    for (size_t i = 0; i < inputs.landmarks.size(); ++i) {
        const auto input         = generate_ith_fibonnaci_sphere_point(i + inputs.landmarks.size(), 2*inputs.landmarks.size());
        const auto output        = tps3d_warp_point(coefficients, input);
        const auto ideal_output  = project_onto_cube_surface(input).value();
        sse += (output - ideal_output).normSqr();
        sst += ideal_output.normSqr();  // i.e. "the dumb model's error if it just guessed (0,0,0) every time"
    }
    const double r2 = 1.0 - (sse / sst);

    ASSERT_GT(r2, 0.98) << "The coefficient of determination (R^2) for the warp is lower than expected!";
}

TEST(tps3d, adjusting_warping_penalty_reduces_point_r2_but_increases_affine_transform_accuracy)
{
    // The `warping_penalty` argument is a regularizer that punishes warping
    // (the `non_affine_terms` in the coefficients), but promotes the other
    // terms (the affine ones).
    //
    // The way to measure this effect is to apply a ground-truth affine
    // transform to the destination of a very-bendy warping pair
    // (e.g. sphere-to-cube). If you use TPS to calculate the affine
    // transform of the pair, the accuracy is linked to the `warping_penalty`
    // because a higher penalty weights the solver towards trying to
    // optimize the affine transform better (it will result in a worse
    // point-to-point R^2, but better affine-transform-to-affine-transform).

    // Ground-truth of the affine transform.
    const SimbodyAffineTransform actual_transform{
        .scale = {1.5, 0.2, 1.4},
        .transform = {
            SimTK::Rotation{33.0*osc::deg_to_rad_v<double>, SimTK::CoordinateAxis::XCoordinateAxis{}},
            SimTK::Vec3{-0.5, 0.3, 0.33}
        },
    };

    // Generate point pairs and apply the affine transform to destination.
    auto input_points = generate_sphere_to_cube_landmark_pairs();
    for (auto& [source, dest] : input_points) {
        dest = actual_transform * dest;
    }

    const TPSCoefficientSolverInputs3D<double> solver_inputs{input_points};
    TPSCoefficientSolverInputs3D<double> penalized_solver_inputs{input_points};
    penalized_solver_inputs.warping_penalty = 0.01;  // Higher --> more weight given to the affine coefficients

    // Solve coefficients with/without a `warping_penalty`
    const TPSCoefficients3D<double> coefs = tps3d_solve_coefficients(solver_inputs);
    const TPSCoefficients3D<double> penalized_coefs = tps3d_solve_coefficients(penalized_solver_inputs);

    // Unpack the coefficients into affine transforms.
    const auto transform = extract_affine_transform(coefs);
    const auto penalized_transform = extract_affine_transform(penalized_coefs);

    // Figure out the error between the unpacked transforms and the ground truth.
    const ParameterSpaceError transform_error{actual_transform, transform};
    const ParameterSpaceError penalized_transform_error{actual_transform, penalized_transform};

    // The penalized inputs should ideally produce a close/lower error (requires tweaking `warping_penalty`).
    ASSERT_LT(penalized_transform_error.scale, transform_error.scale);
    ASSERT_LT(penalized_transform_error.rotation, transform_error.rotation);
    ASSERT_LT(penalized_transform_error.translation, transform_error.translation);
}

TEST(tps3d, setting_warping_penalty_very_high_effectively_yields_an_affine_transform)
{
    // If `warping_penalty` is set very high then the TPS coefficient solver
    // should yield coefficients that only really consider the affine component
    // of the warp.

    // Ground-truth of the affine transform.
    const SimbodyAffineTransform actual_transform{
        .scale = {0.7, 1.1, 0.85},
        .transform = {
            SimTK::Rotation{45.0*osc::deg_to_rad_v<double>, SimTK::CoordinateAxis::YCoordinateAxis{}},
            SimTK::Vec3{-5.5, 3.3, 2.33}
        },
    };

    // Generate point pairs and apply the affine transform to destination.
    auto input_points = generate_sphere_to_cube_landmark_pairs();
    for (auto& [source, dest] : input_points) {
        dest = actual_transform * dest;
    }

    TPSCoefficientSolverInputs3D<double> solver_inputs{input_points};
    solver_inputs.warping_penalty = 100000.0;  // High: the solver should actively avoid warping.
    const TPSCoefficients3D<double> coefs = tps3d_solve_coefficients(solver_inputs);

    for (const auto& t : coefs.non_affine_terms) {  // Non-affine terms determine warping
        ASSERT_LT(t.weight.abs(), 0.00001);
    }
}
