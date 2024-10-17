#include "ShapeFitters.h"

#include <OpenSimCreator/Utils/SimTKConverters.h>

#include <Simbody.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/numeric.h>
#include <oscar/Utils/Assertions.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <complex>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

// generic helpers
namespace
{
    // returns the contents of `vs` with `subtrahend` subtracted from each element
    std::vector<Vec3> Subtract(
        std::span<const Vec3> vs,
        const Vec3& subtrahend)
    {
        std::vector<Vec3> rv;
        rv.reserve(vs.size());
        for (const auto& v : vs) {
            rv.push_back(v - subtrahend);
        }
        return rv;
    }

    // returns the element-wise arithmetic mean of `vs`
    Vec3 CalcMean(std::span<const Vec3> vs)
    {
        Vec3d accumulator{};
        for (const auto& v : vs) {
            accumulator += v;
        }
        return Vec3{accumulator / static_cast<double>(vs.size())};
    }
}

// `m4s`: "MATLAB for SimTK" helpers
//
// helpers that provide a few MATLAB-like utility methods for SimTK, to
// make it easier to translate the code
namespace
{
    SimTK::Matrix Eye(int size)
    {
        SimTK::Matrix rv(size, size, 0.0);
        for (int i = 0; i < size; ++i)
        {
            rv(i)(i) = 1.0;
        }
        return rv;
    }

    template<int M, int N>
    SimTK::Mat<M, N> TopLeft(const SimTK::Matrix& m)
    {
        OSC_ASSERT(m.nrow() >= M);
        OSC_ASSERT(m.ncol() >= N);

        SimTK::Mat<M, N> rv;
        for (int row = 0; row < M; ++row)
        {
            for (int col = 0; col < N; ++col)
            {
                rv(row, col) = m(row, col);
            }
        }
        return rv;
    }

    template<int N>
    SimTK::Vec<N> Diag(const SimTK::Mat<N, N>& m)
    {
        SimTK::Vec<N> rv;
        for (int i = 0; i < N; ++i) {
            rv(i) = m(i, i);
        }
        return rv;
    }

    template<int N>
    SimTK::Vec<N> Sign(const SimTK::Vec<N>& vs)
    {
        SimTK::Vec<N> rv;
        for (int i = 0; i < N; ++i) {
            rv[i] = SimTK::sign(vs[i]);
        }
        return rv;
    }

    template<int N>
    SimTK::Vec<N> Multiply(const SimTK::Vec<N>& a, const SimTK::Vec<N>& b)
    {
        SimTK::Vec<N> rv;
        for (int i = 0; i < N; ++i) {
            rv[i] = a[i] * b[i];
        }
        return rv;
    }

    template<int N>
    SimTK::Vec<N> Reciporical(const SimTK::Vec<N>& v)
    {
        return {1.0/v[0], 1.0/v[1], 1.0/v[2]};
    }

    [[maybe_unused]] std::vector<std::vector<double>> DebuggableMatrix(const SimTK::Matrix& src)
    {
        std::vector<std::vector<double>> rv;
        rv.reserve(src.nrow());
        for (int row = 0; row < src.nrow(); ++row) {
            auto& cols = rv.emplace_back();
            cols.reserve(src.ncol());
            for (int col = 0; col < src.ncol(); ++col) {
                cols.push_back(src(row, col));
            }
        }
        return rv;
    }

    template<int M, int N>
    [[maybe_unused]] std::array<std::array<double, M>, N> DebuggableMatrix(const SimTK::Mat<M, N>& src)
    {
        std::array<std::array<double, M>, N> rv;
        for (int row = 0; row < src.nrow(); ++row) {
            auto& cols = rv[row];
            for (int col = 0; col < src.ncol(); ++col) {
                cols[col] = src(row, col);
            }
        }
        return rv;
    }

    // function that behaves "as if" the caller called MATLAB's `eig` function like
    // so:
    //
    //     [eigenVectors, eigenValuesInDiagonalMatrix] = eig(matrix);
    //
    // note: the returned vectors/values are not guaranteed to be in any particular
    //       order (same behavior as MATLAB).
    template<int N>
    std::pair<SimTK::Mat<N, N>, SimTK::Mat<N, N>> Eig(const SimTK::Mat<N, N>& m)
    {
        // the provided matrix must be re-packed as complex numbers (with no
        // complex part)
        //
        // this is entirely because SimTK's `eigen.cpp` implementation only provides
        // an Eigenanalysis implementation for complex numbers
        SimTK::ComplexMatrix packed(N, N);
        for (int row = 0; row < N; ++row)
        {
            for (int col = 0; col < N; ++col)
            {
                packed(row, col) = SimTK::Complex{m(row, col), 0.0};
            }
        }

        // perform Eigenanalysis
        SimTK::ComplexVector eigenValues(N);
        SimTK::ComplexMatrix eigenVectors(N, N);
        SimTK::Eigen(packed).getAllEigenValuesAndVectors(eigenValues, eigenVectors);

        // re-pack answer from SimTK's Eigenanalysis into a MATLAB-like form
        SimTK::Mat<N, N> repackedEigenVectors(0.0);
        SimTK::Mat<N, N> repackedEigenValues(0.0);
        for (int row = 0; row < N; ++row) {
            for (int col = 0; col < N; ++col) {
                OSC_ASSERT(eigenVectors(row, col).imag() == 0.0);
                repackedEigenVectors(row, col) = eigenVectors(row, col).real();
            }
            OSC_ASSERT(eigenValues(row).imag() == 0.0);
            repackedEigenValues(row, row) = eigenValues(row).real();
        }

        return {repackedEigenVectors, repackedEigenValues};
    }

    // returns the value returned by `Eig`, but re-sorted from smallest to largest
    // eigenvalue
    //
    // (similar idea to "sorted eigenvalues and eigenvectors" section in MATLAB
    //  documentation for `eig`)
    template<int N>
    std::pair<SimTK::Mat<N, N>, SimTK::Mat<N, N>> EigSorted(const SimTK::Mat<N, N>& m)
    {
        // perform unordered Eigenanalysis
        const auto unsorted = Eig(m);

        // create indices into the unordered result that are sorted by increasing Eigenvalue
        const auto sortedIndices = [&unsorted]()
        {
            std::array<int, N> indices{};
            cpp23::iota(indices, 0);
            rgs::sort(indices, rgs::less{}, [&unsorted](int v) { return unsorted.second(v, v); });
            return indices;
        }();

        // use the indices to create a sorted version of the result
        auto sorted = [&unsorted, &sortedIndices]()
        {
            auto copy = unsorted;
            for (int dest = 0; dest < N; ++dest) {
                const int src = sortedIndices[dest];
                copy.first.col(dest) = unsorted.first.col(src);
                copy.second(dest, dest) = unsorted.second(src, src);
            }
            return copy;
        }();

        return sorted;
    }

    // assuming `m` is an orthonormal matrix, ensures that the columns form
    // the vectors of a right-handed system
    void RightHandify(SimTK::Mat33& m)
    {
        const SimTK::Vec3 cp = SimTK::cross(m.col(0), m.col(1));
        if (SimTK::dot(cp, m.col(2)) < 0.0) {
            m.col(2) = -m.col(2);
        }
    }

    // solve systems of linear equations `Ax = B` for `x`
    SimTK::Vector SolveLinearLeastSquares(
        const SimTK::Matrix& A,
        const SimTK::Vector& B,
        std::optional<double> rcond = std::nullopt)
    {
        OSC_ASSERT(A.nrow() == B.nrow());
        SimTK::Vector result(A.ncol(), 0.0);
        if (rcond) {
            SimTK::FactorQTZ{A, *rcond}.solve(B, result);
        }
        else {
            SimTK::FactorQTZ{A}.solve(B, result);
        }

        return result;
    }
}

// shape-fitting specific helper functions
namespace
{
    // returns a covariance matrix by multiplying:
    //
    // - lhs: 3xN matrix (rows are x y z, and columns are each point in `vs`)
    // - rhs: Nx3 matrix (rows are each point in `vs`, columns are x, y, z)
    SimTK::Mat33 CalcCovarianceMatrix(std::span<const Vec3> vs)
    {
        SimTK::Mat33 rv;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                double accumulator = 0.0;
                for (size_t i = 0; i < vs.size(); ++i)
                {
                    const float lhs = vs[i][row];
                    const float rhs = vs[i][col];
                    accumulator += lhs * rhs;
                }
                rv(row, col) = accumulator;
            }
        }
        return rv;
    }

    // returns `v` projected onto a plane's 2D surface, where the
    // plane's surface has basis vectors `basis1` and `basis2`
    Vec2 Project3DPointOntoPlane(
        const Vec3& v,
        const Vec3& basis1,
        const Vec3& basis2)
    {
        return {dot(v, basis1), dot(v, basis2)};
    }

    // returns `surfacePoint` un-projected from the 2D surface of a plane, where
    // the plane's surface has basis vectors `basis1` and `basis2`
    Vec3 Unproject2DPlanePointInto3D(
        Vec2 planeSurfacePoint,
        const Vec3& basis1,
        const Vec3& basis2)
    {
        return planeSurfacePoint.x*basis1 + planeSurfacePoint.y*basis2;
    }

    // part of solving this algeberic form for an ellipsoid:
    //
    //     - Ax^2 + By^2 + Cz^2 + 2Dxy + 2Exz + 2Fyz + 2Gx + 2Hy + 2Iz + J = 0
    //
    // see: https://nl.mathworks.com/matlabcentral/fileexchange/24693-ellipsoid-fit
    std::array<double, 9> SolveEllipsoidAlgebraicForm(std::span<const Vec3> vs)
    {
        // this code is translated like-for-like with the MATLAB version
        // and was checked by comparing debugger output in MATLAB from
        // `ellipsoid_fit.m` to this version
        //
        // which is to say, you should read the "How to Build a Dinosaur"
        // version if something doesn't make sense here

        // the "How to Build a Dinosaur" version only ever calls `ellipsoid_fit`
        // with `equals` set to `''`, which means "unique fit" (no constraints)

        const int nRows = static_cast<int>(vs.size());
        const int nCols = 9;

        SimTK::Matrix D(nRows, nCols);
        SimTK::Vector d2(nRows);
        for (int row = 0; row < nRows; ++row) {
            const double x = vs[row].x;
            const double y = vs[row].y;
            const double z = vs[row].z;

            D(row, 0) = x*x + y*y - 2.0*z*z;
            D(row, 1) = x*x + z*z - 2.0*y*y;
            D(row, 2) = 2.0*x*y;
            D(row, 3) = 2.0*x*z;
            D(row, 4) = 2.0*y*z;
            D(row, 5) = 2.0*x;
            D(row, 6) = 2.0*y;
            D(row, 7) = 2.0*z;
            D(row, 8) = 1.0 + 0.0*x;

            d2(row) = x*x + y*y + z*z;
        }

        // note: SimTK and MATLAB behave slightly different when given inputs
        //       that are signular or badly scaled.
        //
        //       I'm using a hard-coded rcond here to match MATLAB's error message,
        //       so that I can verify that SimTK's behavior can be modified to yield
        //       identical results to MATLAB
        constexpr double c_RCondReportedByMatlab = 1.202234e-16;

        // solve the normal system of equations
        SimTK::Vector u = SolveLinearLeastSquares(
            D.transpose() * D,  // lhs * u = ...
            D.transpose() * d2, // ... rhs
            c_RCondReportedByMatlab
        );

        // repack vector into compile-time-known array
        OSC_ASSERT(u.size() == 9);
        std::array<double, 9> rv{};
        std::copy(u.begin(), u.end(), rv.begin());
        return rv;
    }

    // like-for-like translation from original MATLAB version of the code
    //
    // (I didn't have time to figure out what V is in this context)
    std::array<double, 10> SolveV(const std::array<double, 9>& u)
    {
        return{
            u[0] + u[1] - 1.0f,
            u[0] - 2.0f*u[1] - 1.0f,
            u[1] - 2.0f*u[0] - 1.0f,
            u[2],
            u[3],
            u[4],
            u[5],
            u[6],
            u[7],
            u[8],
        };
    }

    // forms the algebraic form of the ellipsoid
    SimTK::Mat44 CalcA(const std::array<double, 10>& v)
    {
        SimTK::Mat44 A;

        A(0, 0) = v[0];
        A(0, 1) = v[3];
        A(0, 2) = v[4];
        A(0, 3) = v[6];

        A(1, 0) = v[3];
        A(1, 1) = v[1];
        A(1, 2) = v[5];
        A(1, 3) = v[7];

        A(2, 0) = v[4];
        A(2, 1) = v[5];
        A(2, 2) = v[2];
        A(2, 3) = v[8];

        A(3, 0) = v[6];
        A(3, 1) = v[7];
        A(3, 2) = v[8];
        A(3, 3) = v[9];

        return A;
    }

    // calculates the center of the ellipsoid (see original MATLAB code)
    SimTK::Vec3 CalcEllipsoidOrigin(
        const SimTK::Mat44& A,
        const std::array<double, 10>& v)
    {
        SimTK::Matrix topLeft(3, 3);
        topLeft(0, 0) = A(0, 0);
        topLeft(0, 1) = A(0, 1);
        topLeft(0, 2) = A(0, 2);
        topLeft(1, 0) = A(1, 0);
        topLeft(1, 1) = A(1, 1);
        topLeft(1, 2) = A(1, 2);
        topLeft(2, 0) = A(2, 0);
        topLeft(2, 1) = A(2, 1);
        topLeft(2, 2) = A(2, 2);

        SimTK::Vector rhs(3);
        rhs(0) = v[6];
        rhs(1) = v[7];
        rhs(2) = v[8];
        const SimTK::Vector center = SolveLinearLeastSquares(-topLeft, rhs);

        // pack return value into a Vec3
        OSC_ASSERT(center.size() == 3);
        return SimTK::Vec3{center(0), center(1), center(2)};
    }

    std::pair<SimTK::Mat33, SimTK::Mat33> SolveEigenProblem(
        const SimTK::Mat44& A,
        const SimTK::Vec3& center)
    {
        SimTK::Matrix T = Eye(4);
        T(3, 0) = center[0];
        T(3, 1) = center[1];
        T(3, 2) = center[2];

        const SimTK::Matrix R = T * SimTK::Matrix{A} * T.transpose();
        return EigSorted(TopLeft<3, 3>(R) / -R(3, 3));
    }
}

Sphere osc::FitSphere(const Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplamentary information of:
    //
    //     Bishop, P., Cuff, A., & Hutchinson, J. (2021). How to build a dinosaur: Musculoskeletal modeling and simulation of locomotor biomechanics in extinct animals. Paleobiology, 47(1), 1-38. doi:10.1017/pab.2020.46
    //         https://datadryad.org/stash/dataset/doi:10.5061/dryad.73n5tb2v9
    //
    // the sphere-fitting source code in that implementation is cited as being
    // originally written by "Alan Jennings, University of Dayton", which means
    // that the primary source for the algorithm is *probably*:
    //
    //     Alan Jennings, MATLAB Central, "Sphere Fit (least squared)"
    //         https://nl.mathworks.com/matlabcentral/fileexchange/34129-sphere-fit-least-squared?s_tid=prof_contriblnk
    //
    // but I (AK) found the explanation of the algorithm, plus how it's implemented
    // in MATLAB, inelegant, because it relies on taking differences to means of
    // differences to means, etc. etc. and the explanation isn't clear (imo), so I
    // instead opted for porting this implementation:
    //
    //     Charles F. Jekel, "Digital Image Correlation on Steel Ball" (not the blog post's title)
    //         https://jekel.me/2015/Least-Squares-Sphere-Fit/
    //
    // and I found his explanation of it to be much clearer--and therefore, easier to
    // review.
    //
    //
    // # Maths:
    //
    // - this is a simplified in-source explanation of https://jekel.me/2015/Least-Squares-Sphere-Fit/
    //
    //     the blog post is better than this comment, the comment is here only for archival purposes
    //     in case the blog goes down etc.
    //
    // - each point on a parametric sphere must obey: `r^2 = (x - x0)^2 + (y - y0)^2 + (z - z0)^2`
    //     `r` is radius
    //     `x`, `y`, and `z` are cartesian coordinates of a point on the surface of the sphere
    //     `x0`, `y0`, and `x0` are the cartesian coordinates of the sphere's origin
    //
    // - this expands out to `x^2 + y^2 + z^2 = 2xx0 + 2yy0 + 2zz0 + r^2 + x0^2 + y0^2 + z0^2`
    //
    // - for each mesh point (`xi`, `yi`, and `zi`), `r`, `x0`, `y0`, and `z0` must be chosen to
    //   minimize the difference between the rhs of the above equation with the lhs
    //
    // - which is a really fancy way of saying "use least-squares on the following relationship to
    //   compute coefficients that minimize the distance between the analytic result and the mesh
    //   points":
    //
    //     f = [x1^2 + y1^2 + z1^2 ... xi^2 + yi^2 + zi^2]
    //     A = [[2x1 2y1 2z1 1] ... [2xi 2yi 2zi 1]]
    //     c = [x0 y0 z0 (r^2 - x0^2 - y0^2 - z0^2)]
    //
    //     f = Ac  (matrix equivalent to the equation expanded earlier)
    //
    //     use least-squares to solve for `c`

    // get mesh data (care: `osc::Mesh`es are indexed)
    const std::vector<Vec3> points = mesh.indexed_vertices();
    if (points.empty()) {
        return Sphere{{}, 1.0f};  // edge-case: no points in input mesh
    }

    // create `f` and `A` (explained above)
    const int numPoints = static_cast<int>(points.size());
    SimTK::Vector f(numPoints, 0.0);
    SimTK::Matrix A(numPoints, 4);
    for (int i = 0; i < numPoints; ++i) {
        const Vec3 vert = points[i];

        f(i) = dot(vert, vert);  // x^2 + y^2 + z^2
        A(i, 0) = 2.0f*vert[0];
        A(i, 1) = 2.0f*vert[1];
        A(i, 2) = 2.0f*vert[2];
        A(i, 3) = 1.0f;
    }

    // solve `f = Ac` for `c`
    const SimTK::Vector c = SolveLinearLeastSquares(A, f);
    OSC_ASSERT(c.size() == 4);

    // unpack `c` into sphere parameters (explained above)
    const double x0 = c[0];
    const double y0 = c[1];
    const double z0 = c[2];
    const double r2 = c[3] + x0*x0 + y0*y0 + z0*z0;

    const Vec3 origin{Vec3d{x0, y0, z0}};
    const auto radius = static_cast<float>(sqrt(r2));

    return Sphere{origin, radius};
}

Plane osc::FitPlane(const Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplamentary information of:
    //
    //     Bishop, P., Cuff, A., & Hutchinson, J. (2021). How to build a dinosaur: Musculoskeletal modeling and simulation of locomotor biomechanics in extinct animals. Paleobiology, 47(1), 1-38. doi:10.1017/pab.2020.46
    //         https://datadryad.org/stash/dataset/doi:10.5061/dryad.73n5tb2v9
    //     (hereafter referred to as "PB's implementation")
    //
    // The plane-fitting source code in PB's implementation is cited as being
    // "adapted from `affine_fit` function contributed by Audrien Leygue in
    // the MATLAB file exchange", which is probably this:
    //
    //      Adrien Leygue (2023). Plane fit (https://www.mathworks.com/matlabcentral/fileexchange/43305-plane-fit), MATLAB Central File Exchange. Retrieved October 10, 2023.
    //      (hereafter referred to as "AL's implementation")
    //
    // AL's implementation computes the normal and an orthonormal basis for the plane
    // but only explains it as "principal directions". Some googleing reveals that
    // a nice source that explains Principal Component Analysis (PCA):
    //
    //     https://en.wikipedia.org/wiki/Principal_component_analysis
    //
    // that article is long, but contains a crucial quote:
    //
    //   > PCA is used in exploratory data analysis and for making predictive
    //   > models. It is commonly used for dimensionality reduction by projecting
    //   > each data point onto only the first few principal components to obtain
    //   > lower-dimensional data while preserving as much of the data's variation
    //   > as possible. The first principal component can equivalently be defined
    //   > as a direction that maximizes the variance of the projected data. The
    //   > i i-th principal component can be taken as a direction orthogonal to
    //   > the first i − 1 i-1 principal components that maximizes the variance
    //   > of the projected data.
    //   >
    //   >  For either objective, it can be shown that the principal components are
    //   >  eigenvectors of the data's covariance matrix.
    //
    // So AL's implementation yields three vectors where the first one (used as the
    // normal) is "the direction that maximizes the variance of the projected data", and
    // the other two are used as the basis vectors of the plane
    //
    // PB's implementation takes AL's one step further, in that it _also_ computes a
    // reasonable origin for the plane by:
    //
    //    - Projecting the mesh's points onto the basis vectors to yield a sequence of
    //      plane-space 2D points
    //    - Computing the midpoint of the 2D bounding rectangle (in plane-space) around
    //      those points in plane-space
    //    - Un-projecting the plane-space points back into the original space
    //
    // I can't read minds, but I (AK) guess the reason why the midpoint's location is used
    // is because it is computed in an along-the-normal-ignoring way. However, I can't say
    // why the centroid of a bounding rectangle on the plane surface is superior to (e.g.)
    // the mean, or just picking one point and projecting-then-unprojecting it to some
    // point on the plane's surface: mathematically, they're all the same plane

    // extract point cloud from mesh (osc::Meshes are indexed)
    const std::vector<Vec3> vertices = mesh.indexed_vertices();

    if (vertices.empty()) {
        return Plane{{}, {0.0f, 1.0f, 0.0f}};  // edge-case: return unit plane
    }

    // determine the xyz centroid of the point cloud
    const Vec3 mean = CalcMean(vertices);

    // shift point cloud such that the centroid is at the origin
    const std::vector<Vec3> verticesReduced = Subtract(vertices, mean);

    // pack the vertices into a covariance matrix, ready for principal component analysis (PCA)
    const SimTK::Mat33 covarianceMatrix = CalcCovarianceMatrix(verticesReduced);

    // eigen analysis to yield [N, B1, B2]
    const SimTK::Mat33 eigenVectors = EigSorted(covarianceMatrix).first;
    const Vec3 normal = to<Vec3>(eigenVectors.col(0));
    const Vec3 basis1 = to<Vec3>(eigenVectors.col(1));
    const Vec3 basis2 = to<Vec3>(eigenVectors.col(2));

    // project points onto B1 and B2 (plane-space) and calculate the 2D bounding box
    // of them in plane-spae
    const Rect bounds = bounding_rect_of(verticesReduced, [&basis1, &basis2](const Vec3& v)
    {
        return Project3DPointOntoPlane(v, basis1, basis2);
    });

    // calculate the midpoint of those bounds in plane-space
    const Vec2 boundsMidpointInPlaneSpace = centroid_of(bounds);

    // un-project the plane-space midpoint back into mesh-space
    const Vec3 boundsMidPointInReducedSpace = Unproject2DPlanePointInto3D(
        boundsMidpointInPlaneSpace,
        basis1,
        basis2
    );
    const Vec3 boundsMidPointInMeshSpace = boundsMidPointInReducedSpace + mean;

    // return normal and boundsMidPointInMeshSpace
    return Plane{boundsMidPointInMeshSpace, normal};
}

Ellipsoid osc::FitEllipsoid(const Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplamentary information of:
    //
    //     Bishop, P., Cuff, A., & Hutchinson, J. (2021). How to build a dinosaur: Musculoskeletal modeling and simulation of locomotor biomechanics in extinct animals. Paleobiology, 47(1), 1-38. doi:10.1017/pab.2020.46
    //         https://datadryad.org/stash/dataset/doi:10.5061/dryad.73n5tb2v9
    //
    // The ellipsoid-fitting code in that implementation is cited as being
    // authored by Yury Petrov, and it's probably this:
    //
    //      Yury (2023). Ellipsoid fit (https://www.mathworks.com/matlabcentral/fileexchange/24693-ellipsoid-fit), MATLAB Central File Exchange. Retrieved October 12, 2023.
    //
    // Yury's implementation refers to using a 10-parameter algebreic description
    // of an ellipsoid, and the implementation solved an eigen problem at some point,
    // but it isn't clear why. A 10-parameter description of an ellipsoid is mentioned
    // in this paper:
    //
    //     LEAST SQUARES FITTING OF ELLIPSOID USING ORTHOGONAL DISTANCES
    //     http://dx.doi.org/10.1590/S1982-21702015000200019
    //
    // but that doesn't mention using eigen analysis, which I imagine Yury is using
    // as a form of PCA?

    const std::vector<Vec3> meshVertices = mesh.indexed_vertices();
    OSC_ASSERT_ALWAYS(meshVertices.size() >= 9 && "there must be >= 9 indexed vertices in the mesh in order to solve the ellipsoid's algebreic form");
    const auto u = SolveEllipsoidAlgebraicForm(meshVertices);
    const auto v = SolveV(u);
    const auto A = CalcA(v);  // form the algebraic form of the ellipsoid

    // solve for ellipsoid origin
    const auto ellipsoidOrigin = CalcEllipsoidOrigin(A, v);

    // use Eigenanalysis to solve for the ellipsoid's radii and and frame
    auto [evecs, evals] = SolveEigenProblem(A, ellipsoidOrigin);

    // OpenSimCreator modification (this is slightly different behavior from "How to Build a Dinosaur"'s MATLAB code)
    //
    // the original code allows negative radii to come out of the algorithm, but
    // OSC's implementation ensures radii are always positive by negating the
    // corresponding Eigenvector
    {
        const SimTK::Vec3 signs = Sign(Diag(evals));
        for (int i = 0; i < 3; ++i) {
            evecs.col(i) *= signs[i];
            evals.col(i) *= signs[i];
        }
    }

    // OpenSimCreator modification: also ensure that the Eigen vectors form a _right handed_ coordinate
    // system, because that's what SimTK etc. use
    RightHandify(evecs);

    return Ellipsoid{
        to<Vec3>(ellipsoidOrigin),
        to<Vec3>(SimTK::sqrt(Reciporical(Diag(evals)))),
        quat_cast(to<Mat3>(evecs)),
    };
}
