#include "ShapeFitters.hpp"

#include <OpenSimCreator/Bindings/SimTKHelpers.hpp>

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <Simbody.h>

namespace
{
    std::vector<glm::vec3> Subtract(nonstd::span<glm::vec3 const> vs, glm::vec3 const& subtrahend)
    {
        std::vector<glm::vec3> rv;
        rv.reserve(vs.size());
        for (auto const& v : vs)
        {
            rv.push_back(v - subtrahend);
        }
        return rv;
    }

    glm::vec3 CalcMean(nonstd::span<glm::vec3 const> vs)
    {
        glm::dvec3 accumulator{};
        for (auto const& v : vs)
        {
            accumulator += v;
        }
        return glm::vec3{accumulator / static_cast<double>(vs.size())};
    }

    // returns a covariance matrix, mathematically, by multiplying:
    //
    // - lhs: 3xN matrix (rows are x y z, columns are each point)
    // - rhs: Nx3 matrix (rows are each point, columns are x, y, z)
    SimTK::Mat33 CalcCovarianceMatrix(nonstd::span<glm::vec3 const> vs)
    {
        // pack the vertices into a covariance matrix, ready for principal component analysis (PCA)
        SimTK::Mat33 covarianceMatrix;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                double acc = 0.0;
                for (size_t i = 0; i < vs.size(); ++i)
                {
                    float lhs = vs[i][row];
                    float rhs = vs[i][col];
                    acc += lhs * rhs;
                }
                covarianceMatrix(row, col) = acc;
            }
        }
        return covarianceMatrix;
    }

    // function that behaves "as if" the caller called MATLAB's `eig` function like
    // so:
    //
    //     [V, ~] = eig(covarianceMatrix);
    //
    // because that's how it was called in the original shape fitting code
    SimTK::Mat33 CalcEigenVectors(SimTK::Mat33 const& m)
    {
        // the provided matrix must be re-packed as complex numbers (with no
        // complex part) because SimTK's `eigen.cpp` implementation only provides
        // implementations for complex numbers
        SimTK::ComplexMatrix packed(3, 3);
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                packed(row, col) = SimTK::Complex{m(row, col), 0.0};
            }
        }
        SimTK::ComplexVector eigenValues(3);
        SimTK::ComplexMatrix eigenVectors(3, 3);
        SimTK::Eigen(packed).getAllEigenValuesAndVectors(eigenValues, eigenVectors);
        OSC_ASSERT(eigenVectors.nrow() == 3 && eigenVectors.ncol() == 3);

        SimTK::Mat33 unpacked;
        for (int row = 2; row >= 0; --row)
        {
            for (int col = 0; col < 3; ++col)
            {
                // note: the columns are swizzled into _reverse_ order in order to yield
                // a matrix that has the same column layout as MATLAB's `eig` function
                //
                // the significance of the column ordering is that, in Principle Component
                // Analysis (PCA), the first column eigen vector corresponds to 'the most/least
                // varience'
                unpacked(row, 2-col) = eigenVectors(row, col).real();
            }
        }

        return unpacked;
    }

    std::vector<glm::vec2> Project3DPointsOnto2DSurface(
        nonstd::span<glm::vec3 const> vs,
        glm::vec3 const& basis1,
        glm::vec3 const& basis2)
    {
        std::vector<glm::vec2> rv;
        rv.reserve(vs.size());
        for (glm::vec3 const& v : vs)
        {
            rv.emplace_back(glm::dot(v, basis1), glm::dot(v, basis2));
        }
        return rv;
    }

    glm::vec3 Unproject2DSurfacePointInto3D(
        glm::vec2 surfacePoint,
        glm::vec3 const& basis1,
        glm::vec3 const& basis2)
    {
        return surfacePoint.x*basis1 + surfacePoint.y*basis2;
    }

    osc::Rect CalcBounds(nonstd::span<glm::vec2 const> vs)
    {
        if (vs.empty())
        {
            return osc::Rect{};  // edge-case
        }

        osc::Rect rv{vs.front(), vs.front()};
        for (auto it = vs.begin()+1; it != vs.end(); ++it)
        {
            rv.p1 = osc::Min(rv.p1, *it);
            rv.p2 = osc::Max(rv.p2, *it);
        }
        return rv;
    }
}

osc::Sphere osc::FitSphere(Mesh const& mesh)
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
    std::vector<glm::vec3> const points = GetAllIndexedVerts(mesh);
    if (points.empty())
    {
        return Sphere{{}, 1.0f};  // edge-case: no points in input mesh
    }

    // create `f` and `A` (explained above)
    int const numPoints = static_cast<int>(points.size());
    SimTK::Vector f(numPoints, 0.0);
    SimTK::Matrix A(numPoints, 4);
    for (int i = 0; i < numPoints; ++i)
    {
        glm::vec3 const vert = points[i];

        f(i) = glm::dot(vert, vert);  // x^2 + y^2 + z^2
        A(i, 0) = 2.0f*vert[0];
        A(i, 1) = 2.0f*vert[1];
        A(i, 2) = 2.0f*vert[2];
        A(i, 3) = 1.0f;
    }

    // solve `f = Ac` for `c`
    SimTK::Vector c(4, 0.0);
    SimTK::FactorQTZ const factor{A};
    factor.solve(f, c);

    // unpack `c` into sphere parameters (explained above)
    double const x0 = c[0];
    double const y0 = c[1];
    double const z0 = c[2];
    double const r2 = c[3] + x0*x0 + y0*y0 + z0*z0;

    glm::vec3 const origin{glm::dvec3{x0, y0, z0}};
    float const radius = static_cast<float>(glm::sqrt(r2));

    return Sphere{origin, radius};
}

osc::Plane osc::FitPlane(Mesh const& mesh)
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
    std::vector<glm::vec3> const vertices = GetAllIndexedVerts(mesh);

    if (vertices.empty())
    {
        return osc::Plane{{}, {0.0f, 1.0f, 0.0f}};  // edge-case: return unit plane
    }

    // determine the xyz centroid of the point cloud
    glm::vec3 const centroid = CalcMean(vertices);

    // shift point cloud such that the centroid is at the origin
    std::vector<glm::vec3> const verticesReduced = Subtract(vertices, centroid);

    // pack the vertices into a covariance matrix, ready for principal component analysis (PCA)
    SimTK::Mat33 const covarianceMatrix = CalcCovarianceMatrix(verticesReduced);

    // eigen analysis to yield [N, B1, B2]
    SimTK::Mat33 const eigenVectors = CalcEigenVectors(covarianceMatrix);
    SimTK::Vec3 const normal = eigenVectors.col(0);
    SimTK::Vec3 const basis1 = eigenVectors.col(1);
    SimTK::Vec3 const basis2 = eigenVectors.col(2);

    // project points onto B1 and B2 (plane-space)
    std::vector<glm::vec2> const projectedPoints = ::Project3DPointsOnto2DSurface(
        verticesReduced,
        ToVec3(basis1),
        ToVec3(basis2)
    );

    // calculate the 2D bounding box in plane-space of all projected vertices
    Rect const bounds = CalcBounds(projectedPoints);

    // calculate the midpoint of those bounds in plane-space
    glm::vec2 const boundsMidpointInPlaneSpace = Midpoint(bounds);

    // un-project the plane-space midpoint back into mesh-space
    glm::vec3 const boundsMidPointInReducedSpace = Unproject2DSurfacePointInto3D(
        boundsMidpointInPlaneSpace,
        ToVec3(basis1),
        ToVec3(basis2)
    );
    glm::vec3 const boundsMidPointInMeshSpace = boundsMidPointInReducedSpace + centroid;

    // return normal and boundsMidPointInMeshSpace
    return Plane{boundsMidPointInMeshSpace, ToVec3(normal)};
}
