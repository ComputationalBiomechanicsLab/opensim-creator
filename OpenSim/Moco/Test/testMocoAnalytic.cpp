/* -------------------------------------------------------------------------- *
 * OpenSim Moco: testMocoAnalytic.cpp                                         *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
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

#include <OpenSim/Actuators/CoordinateActuator.h>
#include <OpenSim/Actuators/SpringGeneralizedForce.h>
#include <OpenSim/Moco/osimMoco.h>

#include <catch2/catch_all.hpp>
#include "Testing.h"

using namespace OpenSim;

/// Taken from Kirk 1998 equations 5.1-69 and 5.1-70, page 199.
SimTK::Matrix expectedSolution(const SimTK::Vector& time) {
    using std::exp;
    SimTK::Mat22 A(-2 - 0.5 * exp(-2) + 0.5 * exp(2),
            1 - 0.5 * exp(-2) - 0.5 * exp(2), -1 + 0.5 * exp(-2) + 0.5 * exp(2),
            0.5 * exp(-2) - 0.5 * exp(2));
    SimTK::Vec2 b(5, 2);
    SimTK::Vec2 c = A.invert() * b;
    const double c2 = c[0];
    const double c3 = c[1];

    auto x0_func = [&c2, &c3](const double& t) -> double {
        return c2 * (-t - 0.5 * exp(-t) + 0.5 * exp(t)) +
               c3 * (1 - 0.5 * exp(-t) - 0.5 * exp(t));
    };

    auto x1_func = [&c2, &c3](const double& t) -> double {
        return c2 * (-1 + 0.5 * exp(-t) + 0.5 * exp(t)) +
               c3 * (0.5 * exp(-t) - 0.5 * exp(t));
    };

    SimTK::Matrix expectedStatesTrajectory(time.size(), 2);
    for (int itime = 0; itime < time.size(); ++itime) {
        expectedStatesTrajectory(itime, 0) = x0_func(time[itime]);
        expectedStatesTrajectory(itime, 1) = x1_func(time[itime]);
    }
    return expectedStatesTrajectory;
}

/// Kirk 1998, Example 5.1-1, page 198.
template <typename SolverType>
MocoStudy createSecondOrderLinearMinimumEffortStudy(
        const std::string& transcription_scheme) {
    Model model;
    auto* body = new Body("b", 1, SimTK::Vec3(0), SimTK::Inertia(0));
    model.addBody(body);

    auto* joint = new SliderJoint("j", model.getGround(), *body);
    joint->updCoordinate().setName("coord");
    model.addJoint(joint);

    auto* damper = new SpringGeneralizedForce("coord");
    damper->setViscosity(-1.0);
    model.addForce(damper);

    auto* actu = new CoordinateActuator("coord");
    model.addForce(actu);
    model.finalizeConnections();

    MocoStudy study;
    auto& problem = study.updProblem();

    problem.setModelAsCopy(model);
    problem.setTimeBounds(0, 2);
    problem.setStateInfo("/jointset/j/coord/value", {-10, 10}, 0, 5);
    problem.setStateInfo("/jointset/j/coord/speed", {-10, 10}, 0, 2);
    problem.setControlInfo("/forceset/coordinateactuator", {-50, 50});

    problem.addGoal<MocoControlGoal>("effort", 0.5);

    auto& solver = study.initSolver<SolverType>();
    solver.set_num_mesh_intervals(50);
    solver.set_transcription_scheme(transcription_scheme);

    return study;
}

TEST_CASE("Second order linear min effort - MocoTropterSolver") {
    MocoStudy study =
            createSecondOrderLinearMinimumEffortStudy<MocoTropterSolver>(
                    "hermite-simpson");
    MocoSolution solution = study.solve();
    const auto expected = expectedSolution(solution.getTime());
    OpenSim_CHECK_MATRIX_ABSTOL(solution.getStatesTrajectory(), expected, 1e-5);
}

TEST_CASE("Second order linear min effort - MocoCasADiSolver") {
    auto transcription_scheme =
                GENERATE(as<std::string>{}, "hermite-simpson",
                    "legendre-gauss-3", "legendre-gauss-7",
                    "legendre-gauss-radau-3", "legendre-gauss-radau-7");
    MocoStudy study =
            createSecondOrderLinearMinimumEffortStudy<MocoCasADiSolver>(
                    transcription_scheme);
    MocoSolution solution = study.solve();
    const auto expected = expectedSolution(solution.getTime());
    OpenSim_CHECK_MATRIX_ABSTOL(solution.getStatesTrajectory(), expected, 1e-5);
}

/// In the "linear tangent steering" problem, we control the direction to apply
/// a constant thrust to a point mass to move the mass a given vertical distance
/// and maximize its final horizontal speed. This problem is described in
/// Section 2.4 of Bryson and Ho [1].
/// Bryson, A. E., Ho, Y.‐C., Applied Optimal Control, Optimization, Estimation,
/// and Control. New York‐London‐Sydney‐Toronto. John Wiley & Sons. 1975.
TEMPLATE_TEST_CASE("Linear tangent steering",
        "[casadi]", /*MocoTropterSolver, TODO*/
        MocoCasADiSolver) {

    // The pseudospectral schemes have higher accuracy and therefore require
    // fewer mesh intervals compare to the Hermite-Simpson scheme. The LG scheme
    // seems to require more mesh intervals than the LGR scheme to achieve the
    // same performance. This may be because the LG scheme does not have a
    // collocation point at the final time point, whereas the LGR scheme does,
    // and this problem has a cost function based solely on the state at the
    // final time point.
    using record = std::tuple<std::string, int>;
    auto settings = GENERATE(table<std::string, int>({
        record{"hermite-simpson", 100},
        // record{"legendre-gauss-3", 75}, Does not pass consistently.
        // record{"legendre-gauss-7", 75},
        record{"legendre-gauss-radau-3", 50},
        record{"legendre-gauss-radau-7", 50}
    }));

    auto transcription_scheme = std::get<0>(settings);
    auto num_mesh_intervals = std::get<1>(settings);

    // The problem is parameterized by a, T, and h, with 0 < 4h/(aT^2) < 1.
    const double a = 5;
    const double finalTime = 1; // "T"
    const double finalHeight = 1.0; // "h"
    auto residual = [a, finalHeight, finalTime](const double& angle) {
        const double secx = 1.0 / cos(angle);
        const double tanx = tan(angle);
        const double& h = finalHeight;
        const double& T = finalTime;
        return 1.0 / sin(angle) -
               log((secx + tanx) / (secx - tanx)) / (2.0 * tanx * tanx) -
               4 * h / (a * T * T);
    };
    const double initialAngle = solveBisection(
            residual, 0.01, 0.99 * 0.5 * SimTK::Pi, 0.0001, 100);
    const double tanInitialAngle = tan(initialAngle);
    const double c = 2 * tanInitialAngle / finalTime;
    auto txvalue = [a, c, initialAngle](const double& angle) {
        const double seci = 1.0 / cos(initialAngle);
        const double tani = tan(initialAngle);
        const double secx = 1.0 / cos(angle);
        const double tanx = tan(angle);
        return a / (c * c) *
               (seci - secx - tanx * log((tani + seci) / (tanx + secx)));
    };
    auto tyvalue = [a, c, initialAngle](const double& angle) {
        const double seci = 1.0 / cos(initialAngle);
        const double tani = tan(initialAngle);
        const double secx = 1.0 / cos(angle);
        const double tanx = tan(angle);
        return a / (2 * c * c) *
               ((tani - tanx) * seci - (seci - secx) * tanx -
                       log((tani + seci) / (tanx + secx)));
    };
    auto txspeed = [a, c, initialAngle](const double& angle) {
        const double seci = 1.0 / cos(initialAngle);
        const double secx = 1.0 / cos(angle);
        const double tanx = tan(angle);
        return a / c * log((tan(initialAngle) + seci) / (tanx + secx));
    };
    auto tyspeed = [a, c, initialAngle](const double& angle) {
        return a / c * (1.0 / cos(initialAngle) - 1.0 / cos(angle));
    };
    TimeSeriesTable expected;
    const auto expectedTime = createVectorLinspace(100, 0, finalTime);
    expected.setColumnLabels({"/forceset/actuator", "/jointset/tx/tx/value",
            "/jointset/ty/ty/value", "/jointset/tx/tx/speed",
            "/jointset/ty/ty/speed"});
    for (int itime = 0; itime < expectedTime.size(); ++itime) {
        const double& time = expectedTime[itime];
        const double angle = atan(tanInitialAngle - c * time);
        expected.appendRow(
                expectedTime[itime], {angle, txvalue(angle), tyvalue(angle),
                                             txspeed(angle), tyspeed(angle)});
    }
    STOFileAdapter::write(expected,
            "testMocoAnalytic_LinearTangentSteering_expected.sto");

    MocoStudy study = MocoStudyFactory::createLinearTangentSteeringStudy(
            a, finalTime, finalHeight);
    auto& solver = study.initCasADiSolver();
    solver.set_transcription_scheme(transcription_scheme);
    solver.set_optim_finite_difference_scheme("forward");
    solver.set_num_mesh_intervals(num_mesh_intervals);
    solver.set_scale_variables_using_bounds(true);
    solver.set_optim_convergence_tolerance(1e-5);

    MocoSolution solution = study.solve().unseal();
    solution.write(
            fmt::format("testMocoAnalytic_LinearTangentSteering_{}_solution.sto",
                    transcription_scheme));

    const SimTK::Vector time = solution.getTime();
    SimTK::Vector expectedAngle(time.size());
    SimTK::Vector expected_txvalue(time.size());
    SimTK::Vector expected_tyvalue(time.size());
    SimTK::Vector expected_txspeed(time.size());
    SimTK::Vector expected_tyspeed(time.size());
    for (int i = 0; i < expectedAngle.size(); ++i) {
        expectedAngle[i] = atan(tanInitialAngle - c * time[i]);
        expected_txvalue[i] = txvalue(expectedAngle[i]);
        expected_tyvalue[i] = tyvalue(expectedAngle[i]);
        expected_txspeed[i] = txspeed(expectedAngle[i]);
        expected_tyspeed[i] = tyspeed(expectedAngle[i]);
    }
    OpenSim_CHECK_MATRIX_ABSTOL(
            solution.getControl("/forceset/actuator"), expectedAngle, 1e-3);
    OpenSim_CHECK_MATRIX_ABSTOL(
            solution.getState("/jointset/tx/tx/value"), expected_txvalue, 1e-3);
    OpenSim_CHECK_MATRIX_ABSTOL(
            solution.getState("/jointset/ty/ty/value"), expected_tyvalue, 1e-3);
    OpenSim_CHECK_MATRIX_ABSTOL(
            solution.getState("/jointset/tx/tx/speed"), expected_txspeed, 1e-3);
    OpenSim_CHECK_MATRIX_ABSTOL(
            solution.getState("/jointset/ty/ty/speed"), expected_tyspeed, 1e-3);
}
