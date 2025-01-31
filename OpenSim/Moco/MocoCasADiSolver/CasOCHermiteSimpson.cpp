/* -------------------------------------------------------------------------- *
 * OpenSim Moco: CasOCHermiteSimpson.cpp                                      *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Nicholas Bianco                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */
#include "CasOCHermiteSimpson.h"

using casadi::DM;
using casadi::MX;
using casadi::Slice;

namespace CasOC {

DM HermiteSimpson::createQuadratureCoefficientsImpl() const {

    // The duration of each mesh interval.
    const DM mesh(m_solver.getMesh());
    const DM meshIntervals = mesh(Slice(1, m_numMeshPoints)) -
                             mesh(Slice(0, m_numMeshPoints - 1));
    // Simpson quadrature includes integrand evaluations at the midpoint.
    DM quadCoeffs(m_numGridPoints, 1);
    // Loop through each mesh interval and update the corresponding components
    // in the total coefficients vector.
    for (int i = 0; i < m_numMeshIntervals; ++i) {
        // The mesh interval quadrature coefficients overlap at the mesh grid
        // points in the total coefficients vector, so we slice at every other
        // index to update the coefficients vector.
        quadCoeffs(2 * i) += (1.0 / 6.0) * meshIntervals(i);
        quadCoeffs(2 * i + 1) += (2.0 / 3.0) * meshIntervals(i);
        quadCoeffs(2 * i + 2) += (1.0 / 6.0) * meshIntervals(i);
    }
    return quadCoeffs;
}

DM HermiteSimpson::createMeshIndicesImpl() const {
    DM indices = DM::zeros(1, m_numGridPoints);
    for (int i = 0; i < m_numGridPoints; i += 2) { indices(i) = 1; }
    return indices;
}

void HermiteSimpson::calcDefectsImpl(const casadi::MXVector& x,
        const casadi::MXVector& xdot, casadi::MX& defects) const {
    // For more information, see doxygen documentation for the class.

    const int NS = m_problem.getNumStates();
    for (int imesh = 0; imesh < m_numMeshIntervals; ++imesh) {
        // We enforce defect constraints on a mesh interval basis, so add
        // constraints until the number of mesh intervals is reached.
        const auto h = m_times(2 * imesh + 2) - m_times(2 * imesh);
        const auto x_i = x[imesh](Slice(), 0);
        const auto x_mid = x[imesh](Slice(), 1);
        const auto x_ip1 = x[imesh](Slice(), 2);
        const auto xdot_i = xdot[imesh](Slice(), 0);
        const auto xdot_mid = xdot[imesh](Slice(), 1);
        const auto xdot_ip1 = xdot[imesh](Slice(), 2);

        // Hermite interpolant defects.
        defects(Slice(0, NS), imesh) =
                x_mid - 0.5 * (x_ip1 + x_i) - (h / 8.0) * (xdot_i - xdot_ip1);

        // Simpson integration defects.
        defects(Slice(NS, 2 * NS), imesh) =
                x_ip1 - x_i - (h / 6.0) * (xdot_ip1 + 4.0 * xdot_mid + xdot_i);
    }
}

void HermiteSimpson::calcInterpolatingControlsImpl(
        const casadi::MX& controls, casadi::MX& interpControls) const {
    if (m_problem.getNumControls() &&
            m_solver.getInterpolateControlMeshInteriorPoints()) {
        int time_i;
        int time_mid;
        int time_ip1;
        for (int imesh = 0; imesh < m_numMeshIntervals; ++imesh) {
            time_i = 2 * imesh;
            time_mid = 2 * imesh + 1;
            time_ip1 = 2 * imesh + 2;
            const auto c_i = controls(Slice(), time_i);
            const auto c_mid = controls(Slice(), time_mid);
            const auto c_ip1 = controls(Slice(), time_ip1);
            interpControls(Slice(), imesh) = c_mid - 0.5 * (c_ip1 + c_i);
        }
    }
}

} // namespace CasOC
