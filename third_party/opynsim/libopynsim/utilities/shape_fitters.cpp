#include "shape_fitters.h"

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/vector.h>
#include <liboscar/shims/cpp23/numeric.h>
#include <liboscar/utilities/assertions.h>
#include <simmath/LinearAlgebra.h>
#include <SimTKcommon/internal/VectorMath.h>

#include <algorithm>
#include <array>
#include <complex>
#include <functional>
#include <span>
#include <vector>

namespace rgs = std::ranges;

namespace
{
    // Returns the contents of `lhs` with `rhs` subtracted from each element.
    std::vector<osc::Vector3> minus(
        std::span<const osc::Vector3> lhs,
        const osc::Vector3& rhs)
    {
        std::vector<osc::Vector3> rv;
        rv.reserve(lhs.size());
        for (const auto& v : lhs) {
            rv.push_back(v - rhs);
        }
        return rv;
    }

    // Returns the element-wise arithmetic mean of `vs`.
    osc::Vector3 mean_of(std::span<const osc::Vector3> vs)
    {
        osc::Vector3d accumulator{};
        for (const auto& v : vs) {
            accumulator += v;
        }
        return osc::Vector3{accumulator / static_cast<double>(vs.size())};
    }

    // Returns an `n` by `n` identity matrix.
    SimTK::Matrix eye(int n)
    {
        SimTK::Matrix rv(n, n, 0.0);
        for (int i = 0; i < n; ++i) {
            rv(i)(i) = 1.0;
        }
        return rv;
    }

    // Returns the top `M` rows and `N` columns of `m`.
    template<int M, int N>
    SimTK::Mat<M, N> top_left(const SimTK::Matrix& m)
    {
        OSC_ASSERT(m.nrow() >= M);
        OSC_ASSERT(m.ncol() >= N);

        SimTK::Mat<M, N> rv;
        for (int row = 0; row < M; ++row) {
            for (int col = 0; col < N; ++col) {
                rv(row, col) = m(row, col);
            }
        }
        return rv;
    }

    // Returns the diagonal elements of `m`.
    template<int N>
    SimTK::Vec<N> diag(const SimTK::Mat<N, N>& m)
    {
        SimTK::Vec<N> rv;
        for (int i = 0; i < N; ++i) {
            rv(i) = m(i, i);
        }
        return rv;
    }

    // Returns a vector that is the same size as `vs`, but where
    // each element of the vector is:
    // -  `1` if the corresponsing element in `vs` is  >0
    // -  `0` if the corresponding element in `vs` is ==0
    // - `-1` if the corresponding element in `vs` is  <0
    template<int N>
    SimTK::Vec<N> sign(const SimTK::Vec<N>& vs)
    {
        SimTK::Vec<N> rv;
        for (int i = 0; i < N; ++i) {
            rv[i] = SimTK::sign(vs[i]);
        }
        return rv;
    }

    // Returns a vector that is the same size as `vs`, but where
    // each element of the vector is the reciprocal of the
    // corresponding element from `vs`.
    template<int N>
    SimTK::Vec<N> reciprocal_of(const SimTK::Vec<N>& vs)
    {
        return {1.0/vs[0], 1.0/vs[1], 1.0/vs[2]};
    }

    // Returns a pair that contains eigenvectors (first) and eigenvalues (second)
    //
    // This function behaves similarly to MATLAB's `eig` function:
    //
    //     [eigenVectors, eigenValuesInDiagonalMatrix] = eig(matrix);
    //
    // Note: The returned vectors/values are not guaranteed to be in any particular
    //       order (this is the same behavior as MATLAB).
    template<int N>
    std::pair<SimTK::Mat<N, N>, SimTK::Mat<N, N>> eig(const SimTK::Mat<N, N>& m)
    {
        // The provided matrix must be re-packed as complex numbers (with no
        // complex part).
        //
        // This is entirely because SimTK's `eigen.cpp` implementation only provides
        // an Eigenanalysis implementation for complex numbers.
        SimTK::ComplexMatrix packed(N, N);
        for (int row = 0; row < N; ++row) {
            for (int col = 0; col < N; ++col) {
                packed(row, col) = SimTK::Complex{m(row, col), 0.0};
            }
        }

        // Perform Eigenanalysis.
        SimTK::ComplexVector eigenvalues(N);
        SimTK::ComplexMatrix eigenvectors(N, N);
        SimTK::Eigen(packed).getAllEigenValuesAndVectors(eigenvalues, eigenvectors);

        // Re-pack answer from SimTK's Eigenanalysis into a MATLAB-like form.
        SimTK::Mat<N, N> repacked_eigenvectors(0.0);
        SimTK::Mat<N, N> repacked_eigenvalues(0.0);
        for (int row = 0; row < N; ++row) {
            for (int col = 0; col < N; ++col) {
                OSC_ASSERT(eigenvectors(row, col).imag() == 0.0);
                repacked_eigenvectors(row, col) = eigenvectors(row, col).real();
            }
            OSC_ASSERT(eigenvalues(row).imag() == 0.0);
            repacked_eigenvalues(row, row) = eigenvalues(row).real();
        }

        return {repacked_eigenvectors, repacked_eigenvalues};
    }

    // Returns the value returned by `eig`, but re-sorted from smallest to largest
    // eigenvalue.
    //
    // (similar idea to "sorted eigenvalues and eigenvectors" section in MATLAB
    //  documentation for `eig`)
    template<int N>
    std::pair<SimTK::Mat<N, N>, SimTK::Mat<N, N>> eig_sorted(const SimTK::Mat<N, N>& m)
    {
        // perform unordered Eigenanalysis
        const auto unsorted = eig(m);

        // create indices into the unordered result that are sorted by increasing Eigenvalue
        const auto sorted_indices = [&unsorted]()
        {
            std::array<int, N> indices{};
            osc::cpp23::iota(indices, 0);
            rgs::sort(indices, rgs::less{}, [&unsorted](int v) { return unsorted.second(v, v); });
            return indices;
        }();

        // use the indices to create a sorted version of the result
        auto sorted = [&unsorted, &sorted_indices]()
        {
            auto copy = unsorted;
            for (int dest = 0; dest < N; ++dest) {
                const int src = sorted_indices[dest];
                copy.first.col(dest) = unsorted.first.col(src);
                copy.second(dest, dest) = unsorted.second(src, src);
            }
            return copy;
        }();

        return sorted;
    }

    // assuming `m` is an orthonormal matrix, ensures that the columns form
    // the vectors of a right-handed system
    void right_handify(SimTK::Mat33& m)
    {
        const SimTK::Vec3 cp = SimTK::cross(m.col(0), m.col(1));
        if (SimTK::dot(cp, m.col(2)) < 0.0) {
            m.col(2) = -m.col(2);
        }
    }

    // solve systems of linear equations `Ax = B` for `x`
    SimTK::Vector solve_linear_least_squares(
        const SimTK::Matrix& a,
        const SimTK::Vector& b,
        std::optional<double> rcond = std::nullopt)
    {
        OSC_ASSERT(a.nrow() == b.nrow());
        SimTK::Vector result(a.ncol(), 0.0);
        if (rcond) {
            SimTK::FactorQTZ{a, *rcond}.solve(b, result);
        }
        else {
            SimTK::FactorQTZ{a}.solve(b, result);
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
    SimTK::Mat33 calc_covariance_matrix(std::span<const osc::Vector3> vs)
    {
        SimTK::Mat33 rv;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                double accumulator = 0.0;
                for (const osc::Vector3& v : vs) {
                    accumulator += v[row] * v[col];
                }
                rv(row, col) = accumulator;
            }
        }
        return rv;
    }

    // returns `v` projected onto a plane's 2D surface, where the
    // plane's surface has basis vectors `basis1` and `basis2`
    osc::Vector2 project_3d_point_onto_plane(
        const osc::Vector3& v,
        const osc::Vector3& basis1,
        const osc::Vector3& basis2)
    {
        return {dot(v, basis1), dot(v, basis2)};
    }

    // returns `surfacePoint` un-projected from the 2D surface of a plane, where
    // the plane's surface has basis vectors `basis1` and `basis2`
    osc::Vector3 unproject_2d_plane_point_into_3d(
        osc::Vector2 plane_surface_point,
        const osc::Vector3& basis1,
        const osc::Vector3& basis2)
    {
        return plane_surface_point.x()*basis1 + plane_surface_point.y()*basis2;
    }

    // part of solving this algebraic form for an ellipsoid:
    //
    //     - Ax^2 + By^2 + Cz^2 + 2Dxy + 2Exz + 2Fyz + 2Gx + 2Hy + 2Iz + J = 0
    //
    // see: https://nl.mathworks.com/matlabcentral/fileexchange/24693-ellipsoid-fit
    std::array<double, 9> solve_ellipsoid_algebraic_form(std::span<const osc::Vector3> vs)
    {
        // this code is translated like-for-like with the MATLAB version
        // and was checked by comparing debugger output in MATLAB from
        // `ellipsoid_fit.m` to this version
        //
        // which is to say, you should read the "How to Build a Dinosaur"
        // version if something doesn't make sense here

        // the "How to Build a Dinosaur" version only ever calls `ellipsoid_fit`
        // with `equals` set to `''`, which means "unique fit" (no constraints)

        const int n_rows = static_cast<int>(vs.size());
        const int n_cols = 9;

        SimTK::Matrix d(n_rows, n_cols);
        SimTK::Vector d2(n_rows);
        for (int row = 0; row < n_rows; ++row) {
            const double x = vs[row].x();
            const double y = vs[row].y();
            const double z = vs[row].z();

            d(row, 0) = x*x + y*y - 2.0*z*z;
            d(row, 1) = x*x + z*z - 2.0*y*y;
            d(row, 2) = 2.0*x*y;
            d(row, 3) = 2.0*x*z;
            d(row, 4) = 2.0*y*z;
            d(row, 5) = 2.0*x;
            d(row, 6) = 2.0*y;
            d(row, 7) = 2.0*z;
            d(row, 8) = 1.0 + 0.0*x;

            d2(row) = x*x + y*y + z*z;
        }

        // note: SimTK and MATLAB behave slightly different when given inputs
        //       that are singular or badly scaled.
        //
        //       I'm using a hard-coded rcond here to match MATLAB's error message,
        //       so that I can verify that SimTK's behavior can be modified to yield
        //       identical results to MATLAB
        constexpr double c_r_cond_reported_by_matlab = 1.202234e-16;

        // solve the normal system of equations
        SimTK::Vector u = solve_linear_least_squares(
            d.transpose() * d,  // lhs * u = ...
            d.transpose() * d2, // ... rhs
            c_r_cond_reported_by_matlab
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
    std::array<double, 10> solve_v(const std::array<double, 9>& u)
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
    SimTK::Mat44 calc_a(const std::array<double, 10>& v)
    {
        SimTK::Mat44 a;

        a(0, 0) = v[0];
        a(0, 1) = v[3];
        a(0, 2) = v[4];
        a(0, 3) = v[6];

        a(1, 0) = v[3];
        a(1, 1) = v[1];
        a(1, 2) = v[5];
        a(1, 3) = v[7];

        a(2, 0) = v[4];
        a(2, 1) = v[5];
        a(2, 2) = v[2];
        a(2, 3) = v[8];

        a(3, 0) = v[6];
        a(3, 1) = v[7];
        a(3, 2) = v[8];
        a(3, 3) = v[9];

        return a;
    }

    // calculates the center of the ellipsoid (see original MATLAB code)
    SimTK::Vec3 calc_ellipsoid_origin(
        const SimTK::Mat44& a,
        const std::array<double, 10>& v)
    {
        SimTK::Matrix top_left(3, 3);
        top_left(0, 0) = a(0, 0);
        top_left(0, 1) = a(0, 1);
        top_left(0, 2) = a(0, 2);
        top_left(1, 0) = a(1, 0);
        top_left(1, 1) = a(1, 1);
        top_left(1, 2) = a(1, 2);
        top_left(2, 0) = a(2, 0);
        top_left(2, 1) = a(2, 1);
        top_left(2, 2) = a(2, 2);

        SimTK::Vector rhs(3);
        rhs(0) = v[6];
        rhs(1) = v[7];
        rhs(2) = v[8];
        const SimTK::Vector center = solve_linear_least_squares(-top_left, rhs);

        // pack return value into a Vec3
        OSC_ASSERT(center.size() == 3);
        return SimTK::Vec3{center(0), center(1), center(2)};
    }

    std::pair<SimTK::Mat33, SimTK::Mat33> solve_eigen_problem(
        const SimTK::Mat44& a,
        const SimTK::Vec3& center)
    {
        SimTK::Matrix t = eye(4);
        t(3, 0) = center[0];
        t(3, 1) = center[1];
        t(3, 2) = center[2];

        const SimTK::Matrix r = t * SimTK::Matrix{a} * t.transpose();
        return eig_sorted(top_left<3, 3>(r) / -r(3, 3));
    }
}

osc::Sphere opyn::fit_sphere_htbad(const osc::Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplementary information of:
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
    const std::vector<osc::Vector3> points = mesh.indexed_vertices();
    if (points.empty()) {
        return osc::Sphere{{}, 1.0f};  // edge-case: no points in input mesh
    }

    // create `f` and `A` (explained above)
    const int num_points = static_cast<int>(points.size());
    SimTK::Vector f(num_points, 0.0);
    SimTK::Matrix a(num_points, 4);
    for (int i = 0; i < num_points; ++i) {
        const osc::Vector3 vert = points[i];

        f(i) = osc::dot(vert, vert);  // x^2 + y^2 + z^2
        a(i, 0) = 2.0f*vert[0];
        a(i, 1) = 2.0f*vert[1];
        a(i, 2) = 2.0f*vert[2];
        a(i, 3) = 1.0f;
    }

    // solve `f = Ac` for `c`
    const SimTK::Vector c = solve_linear_least_squares(a, f);
    OSC_ASSERT(c.size() == 4);

    // unpack `c` into sphere parameters (explained above)
    const double x0 = c[0];
    const double y0 = c[1];
    const double z0 = c[2];
    const double r2 = c[3] + x0*x0 + y0*y0 + z0*z0;

    const osc::Vector3 origin{osc::Vector3d{x0, y0, z0}};
    const auto radius = static_cast<float>(sqrt(r2));

    return osc::Sphere{origin, radius};
}

osc::Plane opyn::fit_plane_htbad(const osc::Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplementary information of:
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
    // is that it is computed in an along-the-normal-ignoring way. However, I can't say
    // why the centroid of a bounding rectangle on the plane surface is superior to (e.g.)
    // the mean, or just picking one point and projecting-then-unprojecting it to some
    // point on the plane's surface: mathematically, they're all the same plane

    // extract point cloud from mesh (osc::Meshes are indexed)
    const std::vector<osc::Vector3> vertices = mesh.indexed_vertices();

    if (vertices.empty()) {
        return osc::Plane{{}, {0.0f, 1.0f, 0.0f}};  // edge-case: return unit plane
    }

    // determine the xyz centroid of the point cloud
    const osc::Vector3 mean = mean_of(vertices);

    // shift point cloud such that the centroid is at the origin
    const std::vector<osc::Vector3> vertices_reduced = minus(vertices, mean);

    // pack the vertices into a covariance matrix, ready for principal component analysis (PCA)
    const SimTK::Mat33 covariance_matrix = calc_covariance_matrix(vertices_reduced);

    // eigen analysis to yield [N, B1, B2]
    const SimTK::Mat33 eigen_vectors = eig_sorted(covariance_matrix).first;
    const auto normal = osc::to<osc::Vector3>(eigen_vectors.col(0));
    const auto basis1 = osc::to<osc::Vector3>(eigen_vectors.col(1));
    const auto basis2 = osc::to<osc::Vector3>(eigen_vectors.col(2));

    // project points onto B1 and B2 (plane-space) and calculate the 2D bounding box
    // of them in plane-spae
    const osc::Rect bounds = bounding_rect_of(vertices_reduced, [&basis1, &basis2](const osc::Vector3& v)
    {
        return project_3d_point_onto_plane(v, basis1, basis2);
    });

    // calculate the midpoint of those bounds in plane-space
    const osc::Vector2 bounds_midpoint_in_plane_space = bounds.origin();

    // un-project the plane-space midpoint back into mesh-space
    const osc::Vector3 bounds_mid_point_in_reduced_space = unproject_2d_plane_point_into_3d(
        bounds_midpoint_in_plane_space,
        basis1,
        basis2
    );
    const osc::Vector3 bounds_mid_point_in_mesh_space = bounds_mid_point_in_reduced_space + mean;

    return osc::Plane{bounds_mid_point_in_mesh_space, normal};
}

osc::Ellipsoid opyn::fit_ellipsoid_htbad(const osc::Mesh& mesh)
{
    // # Background Reading:
    //
    // the original inspiration for this implementation came from the
    // shape fitting code found in the supplementary information of:
    //
    //     Bishop, P., Cuff, A., & Hutchinson, J. (2021). How to build a dinosaur: Musculoskeletal modeling and simulation of locomotor biomechanics in extinct animals. Paleobiology, 47(1), 1-38. doi:10.1017/pab.2020.46
    //         https://datadryad.org/stash/dataset/doi:10.5061/dryad.73n5tb2v9
    //
    // The ellipsoid-fitting code in that implementation is cited as being
    // authored by Yury Petrov, and it's probably this:
    //
    //      Yury (2023). Ellipsoid fit (https://www.mathworks.com/matlabcentral/fileexchange/24693-ellipsoid-fit), MATLAB Central File Exchange. Retrieved October 12, 2023.
    //
    // Yury's implementation refers to using a 10-parameter algebraic description
    // of an ellipsoid, and the implementation solved an eigen problem at some point,
    // but it isn't clear why. A 10-parameter description of an ellipsoid is mentioned
    // in this paper:
    //
    //     LEAST SQUARES FITTING OF ELLIPSOID USING ORTHOGONAL DISTANCES
    //     http://dx.doi.org/10.1590/S1982-21702015000200019
    //
    // but that doesn't mention using eigen analysis, which I imagine Yury is using
    // as a form of PCA?

    const std::vector<osc::Vector3> mesh_vertices = mesh.indexed_vertices();
    OSC_ASSERT_ALWAYS(mesh_vertices.size() >= 9 && "there must be >= 9 indexed vertices in the mesh in order to solve the ellipsoid's algebreic form");
    const auto u = solve_ellipsoid_algebraic_form(mesh_vertices);
    const auto v = solve_v(u);
    const auto a = calc_a(v);  // form the algebraic form of the ellipsoid

    // solve for ellipsoid origin
    const auto ellipsoid_origin = calc_ellipsoid_origin(a, v);

    // use Eigenanalysis to solve for the ellipsoid's radii and frame
    auto [evecs, evals] = solve_eigen_problem(a, ellipsoid_origin);

    // OpenSimCreator modification (this is slightly different behavior from "How to Build a Dinosaur"'s MATLAB code)
    //
    // the original code allows negative radii to come out of the algorithm, but
    // OSC's implementation ensures radii are always positive by negating the
    // corresponding Eigenvector
    {
        const SimTK::Vec3 signs = sign(diag(evals));
        for (int i = 0; i < 3; ++i) {
            evecs.col(i) *= signs[i];
            evals.col(i) *= signs[i];
        }
    }

    // OpenSimCreator modification: also ensure that the Eigen vectors form a _right handed_ coordinate
    // system, because that's what SimTK etc. use
    right_handify(evecs);

    return osc::Ellipsoid{
        osc::to<osc::Vector3>(ellipsoid_origin),
        osc::to<osc::Vector3>(SimTK::sqrt(reciprocal_of(diag(evals)))),
        osc::quaternion_cast(osc::to<osc::Matrix3x3>(evecs)),
    };
}
