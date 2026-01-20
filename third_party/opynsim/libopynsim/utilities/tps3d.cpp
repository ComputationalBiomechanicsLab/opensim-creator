#include "tps3d.h"

#include <libopynsim/utilities/assertions.h>

#include <simmath/LinearAlgebra.h>
#include <SimTKcommon/internal/Vec.h>

#include <concepts>
#include <cstddef>
#include <future>
#include <ostream>
#include <span>
#include <thread>
#include <vector>

using namespace opyn;

namespace
{
    // perform a parallelized and "Chunked" ForEach, where each thread receives an
    // independent chunk of data to process
    //
    // this is a poor-man's `std::execution::par_unseq`, because C++17's <execution>
    // isn't fully integrated into MacOS
    template<typename T, std::invocable<T&> UnaryFunction>
    void for_each_parallel_unsequenced(
        size_t min_chunk_size,
        std::span<T> values,
        UnaryFunction mutator)
    {
        const size_t chunk_size = std::max(min_chunk_size, values.size()/std::thread::hardware_concurrency());
        const size_t num_tasks = values.size()/chunk_size;

        if (num_tasks > 1) {
            std::vector<std::future<void>> tasks;
            tasks.reserve(num_tasks);

            for (size_t i = 0; i < num_tasks-1; ++i) {
                const size_t chunk_begin = i * chunk_size;
                const size_t chunk_end = (i+1) * chunk_size;
                tasks.push_back(std::async(std::launch::async, [values, mutator, chunk_begin, chunk_end]()
                {
                    for (size_t j = chunk_begin; j < chunk_end; ++j) {
                        mutator(values[j]);
                    }
                }));
            }

            // last worker must also handle the remainder
            {
                const size_t chunk_begin = (num_tasks-1) * chunk_size;
                const size_t chunk_end = values.size();
                tasks.push_back(std::async(std::launch::async, [values, mutator, chunk_begin, chunk_end]()
                {
                    for (size_t i = chunk_begin; i < chunk_end; ++i) {
                        mutator(values[i]);
                    }
                }));
            }

            for (std::future<void>& task : tasks) {
                task.get();
            }
        }
        else {
            // chunks would be too small if parallelized: just do it sequentially
            for (T& value : values) {
                mutator(value);
            }
        }
    }

    // this is effectively the "U" term in the TPS algorithm literature
    //
    // i.e. U(||pi - p||) in the literature is equivalent to `RadialBasisFunction3D(pi, p)` here
    template<std::floating_point T>
    float RadialBasisFunction3D(const SimTK::Vec<3, T>& controlPoint, const SimTK::Vec<3, T>& p)
    {
        // this implementation uses the U definition from the following (later) source:
        //
        // Chapter 3, "Semilandmarks in Three Dimensions" by Phillip Gunz, Phillip Mitteroecker,
        // and Fred L. Bookstein
        //
        // the original Bookstein paper uses U(v) = |v|^2 * log(|v|^2), but subsequent literature
        // (e.g. the above book) uses U(v) = |v|. The primary author (Gunz) claims that the original
        // basis function is not as good as just using the magnitude?

        return static_cast<float>((controlPoint - p).norm());
    }

    template<std::floating_point T>
    std::ostream& write_human_readable(std::ostream& o, const TPSCoefficientSolverInputs3D<T>& inputs)
    {
        o << "TPSCoefficientSolverInputs3D{landmarks = [";
        std::string_view delimiter;
        for (const landmark_pair_3d<T>& landmark : inputs.landmarks) {
            o << delimiter << landmark;
            delimiter = ", ";
        }
        o << "]}";
        return o;
    }

    template<std::floating_point T>
    std::ostream& write_human_readable(std::ostream& o, const TPSNonAffineTerm3D<T>& wt)
    {
        return o << "TPSNonAffineTerm3D{Weight = " << wt.weight << ", ControlPoint = " << wt.controlPoint << '}';
    }

    template<std::floating_point T>
    std::ostream& write_human_readable(std::ostream& o, const TPSCoefficients3D<T>& coefs)
    {
        o << "TPSCoefficients3D{a1 = " << coefs.a1 << ", a2 = " << coefs.a2 << ", a3 = " << coefs.a3 << ", a4 = " << coefs.a4;
        for (size_t i = 0; i < coefs.nonAffineTerms.size(); ++i) {
            o << ", w" << i << " = " << coefs.nonAffineTerms[i];
        }
        o << '}';
        return o;
    }

    template<std::floating_point T>
    TPSCoefficients3D<T> TPSCalcCoefficients(
        cpp23::mdspan<const T, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride> source_landmarks,
        cpp23::mdspan<const T, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride> destination_landmarks)
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
        //    can pump new inputs into (e.g. mesh points, muscle points)
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

        OPYN_ASSERT_ALWAYS(source_landmarks.size() == destination_landmarks.size());

        const int numPairs = static_cast<int>(source_landmarks.extent(0));

        if (numPairs == 0) {
            // edge-case: there are no pairs, so return an identity-like transform
            return TPSCoefficients3D<T>{};
        }

        // construct matrix L
        SimTK::Matrix L(numPairs + 4, numPairs + 4);

        // populate the K part of matrix L (upper-left)
        for (int row = 0; row < numPairs; ++row) {
            for (int col = 0; col < numPairs; ++col) {
                const SimTK::Vec<3, T> pis = {source_landmarks(row, 0), source_landmarks(row, 1), source_landmarks(row, 2)};
                const SimTK::Vec<3, T> pj = {source_landmarks(col, 0), source_landmarks(col, 1), source_landmarks(col, 2)};

                L(row, col) = RadialBasisFunction3D(pis, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            const int pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row) {
                L(row, pStartColumn)     = 1.0;
                L(row, pStartColumn + 1) = source_landmarks(row, 0);
                L(row, pStartColumn + 2) = source_landmarks(row, 1);
                L(row, pStartColumn + 3) = source_landmarks(row, 2);
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            const int ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col) {
                L(ptStartRow, col)     = 1.0;
                L(ptStartRow + 1, col) = source_landmarks(col, 0);
                L(ptStartRow + 2, col) = source_landmarks(col, 1);
                L(ptStartRow + 3, col) = source_landmarks(col, 2);
            }
        }

        // populate the 0 part of matrix L (bottom-right)
        {
            const int zeroStartRow = numPairs;
            const int zeroStartCol = numPairs;

            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    L(zeroStartRow + row, zeroStartCol + col) = 0.0;
                }
            }
        }

        // construct "result" vectors Vx and Vy (these hold the landmark destinations)
        SimTK::Vector Vx(numPairs + 4, 0.0);
        SimTK::Vector Vy(numPairs + 4, 0.0);
        SimTK::Vector Vz(numPairs + 4, 0.0);
        for (int row = 0; row < numPairs; ++row) {
            Vx[row] = destination_landmarks(row, 0);
            Vy[row] = destination_landmarks(row, 1);
            Vz[row] = destination_landmarks(row, 2);
        }

        // create a linear solver that can be used to solve `L*Cn = Vn` for `Cn` (where `n` is a dimension)
        const SimTK::FactorQTZ F{L};

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

        TPSCoefficients3D<T> rv;

        // populate affine a1, a2, a3, and a4 terms
        rv.a1 = {T(Cx[numPairs]),   T(Cy[numPairs])  , T(Cz[numPairs])  };
        rv.a2 = {T(Cx[numPairs+1]), T(Cy[numPairs+1]), T(Cz[numPairs+1])};
        rv.a3 = {T(Cx[numPairs+2]), T(Cy[numPairs+2]), T(Cz[numPairs+2])};
        rv.a4 = {T(Cx[numPairs+3]), T(Cy[numPairs+3]), T(Cz[numPairs+3])};

        // populate `wi` coefficients (+ control points, needed at evaluation-time)
        rv.nonAffineTerms.reserve(numPairs);
        for (int i = 0; i < numPairs; ++i) {
            const SimTK::Vec<3, T> weight{T(Cx[i]), T(Cy[i]), T(Cz[i])};
            const SimTK::Vec<3, T> controlPoint{source_landmarks(i, 0), source_landmarks(i, 1), source_landmarks(i, 2)};
            rv.nonAffineTerms.emplace_back(weight, controlPoint);
        }

        return rv;
    }

    template<std::floating_point T>
    TPSCoefficients3D<T> TPSCalcCoefficients(const TPSCoefficientSolverInputs3D<T>& inputs)
    {
        if (inputs.landmarks.empty()) {
            // edge-case: there are no pairs, so return an identity-like transform
            return TPSCoefficients3D<T>{};
        }

        static_assert(sizeof(landmark_pair_3d<T>) == 6*sizeof(T));
        static_assert(alignof(landmark_pair_3d<T>) == alignof(T));
        const cpp23::extents<size_t, cpp23::dynamic_extent, 3> shape{inputs.landmarks.size()};
        const std::array<size_t, 2> strides = {6, 1};
        const cpp23::layout_stride::mapping mapping{shape, strides};

        auto rv = ::TPSCalcCoefficients<T>(
            {&inputs.landmarks.front().source[0], mapping},
            {&inputs.landmarks.front().destination[0], mapping}
        );

        // If required, modify the coefficients
        if (not inputs.applyAffineTranslation) {
            rv.a1 = SimTK::Vec<3, T>{T(0.0), T(0.0), T(0.0)};
        }
        if (not inputs.applyAffineScale) {
            rv.a2 = rv.a2.normalize();
            rv.a3 = rv.a3.normalize();
            rv.a4 = rv.a4.normalize();
        }
        if (not inputs.applyAffineRotation) {
            rv.a2 = {rv.a2.norm(), T(0.0),       T(0.0)};
            rv.a3 = {T(0.0),       rv.a3.norm(), T(0.0)};
            rv.a4 = {T(0.0),       T(0.0),       rv.a4.norm()};
        }
        if (not inputs.applyNonAffineWarp) {
            rv.nonAffineTerms.clear();
        }

        return rv;
    }

    template<std::floating_point T>
    SimTK::Vec<3, T> TPSWarpPoint(const TPSCoefficients3D<T>& coefs, SimTK::Vec<3, T> p)
    {
        // this implementation effectively evaluates `fx(x, y, z)`, `fy(x, y, z)`, and
        // `fz(x, y, z)` the same time, because `TPSCoefficients3D` stores the X, Y, and Z
        // variants of the coefficients together in memory (as `Vector3`s)

        // compute affine terms (a1 + a2*x + a3*y + a4*z)
        SimTK::Vec3 rv = SimTK::Vec3{coefs.a1} + SimTK::Vec3{coefs.a2*p[0]} + SimTK::Vec3{coefs.a3*p[1]} + SimTK::Vec3{coefs.a4*p[2]};

        // accumulate non-affine terms (effectively: wi * U(||controlPoint - p||))
        for (const TPSNonAffineTerm3D<T>& term : coefs.nonAffineTerms) {
            rv += term.weight * RadialBasisFunction3D(term.controlPoint, p);
        }

        return SimTK::Vec<3, T>{rv};
    }
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSCoefficientSolverInputs3D<float>& inputs)
{
    return write_human_readable(o, inputs);
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSCoefficientSolverInputs3D<double>& inputs)
{
    return write_human_readable(o, inputs);
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSNonAffineTerm3D<float>& wt)
{
    return write_human_readable(o, wt);
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSNonAffineTerm3D<double>& wt)
{
    return write_human_readable(o, wt);
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSCoefficients3D<float>& coefs)
{
    return write_human_readable(o, coefs);
}

std::ostream& opyn::operator<<(std::ostream& o, const TPSCoefficients3D<double>& coefs)
{
    return write_human_readable(o, coefs);
}

TPSCoefficients3D<float> opyn::TPSCalcCoefficients(const TPSCoefficientSolverInputs3D<float>& inputs)
{
    return ::TPSCalcCoefficients<float>(inputs);
}

TPSCoefficients3D<double> opyn::TPSCalcCoefficients(const TPSCoefficientSolverInputs3D<double>& inputs)
{
    return ::TPSCalcCoefficients<double>(inputs);
}

TPSCoefficients3D<double> opyn::TPSCalcCoefficients(
    cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride> source_landmarks,
    cpp23::mdspan<const double, cpp23::extents<size_t, std::dynamic_extent, 3>, cpp23::layout_stride> destination_landmarks)
{
    return ::TPSCalcCoefficients<double>(source_landmarks, destination_landmarks);
}

SimTK::Vec<3, float> opyn::TPSWarpPoint(const TPSCoefficients3D<float>& coefs, SimTK::Vec<3, float> p)
{
    return ::TPSWarpPoint<float>(coefs, p);
}

SimTK::Vec<3, double> opyn::TPSWarpPoint(const TPSCoefficients3D<double>& coefs, SimTK::Vec<3, double> p)
{
    return ::TPSWarpPoint<double>(coefs, p);
}

SimTK::Vec<3, float> opyn::TPSWarpPoint(const TPSCoefficients3D<float>& coefs, SimTK::Vec<3, float> vert, float blendingFactor)
{
    const SimTK::Vec<3, float> warped = TPSWarpPoint(coefs, vert);
    return {
        std::lerp(vert[0], warped[0], blendingFactor),
        std::lerp(vert[1], warped[1], blendingFactor),
        std::lerp(vert[2], warped[2], blendingFactor),
    };
}

std::vector<SimTK::Vec<3, float>> opyn::TPSWarpPoints(
    const TPSCoefficients3D<float>& coefs,
    std::span<const SimTK::Vec<3, float>> points,
    float blendingFactor)
{
    std::vector<SimTK::Vec<3, float>> rv(points.begin(), points.end());
    TPSWarpPointsInPlace(coefs, rv, blendingFactor);
    return rv;
}

void opyn::TPSWarpPointsInPlace(
    const TPSCoefficients3D<float>& coefs,
    std::span<SimTK::Vec<3, float>> points,
    float blendingFactor)
{
    for_each_parallel_unsequenced(8192, points, [&coefs, blendingFactor](SimTK::Vec<3, float>& vert)
    {
        vert = TPSWarpPoint(coefs, vert, blendingFactor);
    });
}
