#include "ShapeFitters.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/Utils/Assertions.hpp>

#include <Simbody.h>

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
    auto const indexedVerts = mesh.getVerts();
    auto const indices = mesh.getIndices();
    int const numPoints = static_cast<int>(indices.size());

    // handle edge-case (no points)
    if (numPoints <= 0)
    {
        return Sphere{{}, 1.0f};
    }

    // create `f` and `A` (explained above)
    SimTK::Vector f(numPoints, 0.0);
    SimTK::Matrix A(numPoints, 4);
    for (int i = 0; i < numPoints; ++i)
    {
        glm::vec3 const vert = indexedVerts[indices[i]];

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
