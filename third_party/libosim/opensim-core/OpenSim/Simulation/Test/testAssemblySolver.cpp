/* -------------------------------------------------------------------------- *
 *                      OpenSim:  testAssemblySolver.cpp                      *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2017 Stanford University and the Authors                *
 * Author(s): Ajay Seth                                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

//=============================================================================
// testAssemblySolver loads models with constraints to verify that constraints
// are adequately satisfied or that an appropriate exception is thrown.
//
//=============================================================================
#include <OpenSim/Actuators/ModelFactory.h>
#include <OpenSim/Auxiliary/auxiliaryTestFunctions.h>
#include <OpenSim/Common/LoadOpenSimLibrary.h>
#include <OpenSim/Common/MultivariatePolynomialFunction.h>
#include <OpenSim/Simulation/osimSimulation.h>
#include <catch2/catch_all.hpp>

using namespace OpenSim;
using namespace std;

namespace {
    void instrumentSetStateValues(const string& modelFile)
    {
        // Setup OpenSim model
        Model model(modelFile);
        SimTK::State &s = model.initSystem();

        auto names = model.getStateVariableNames();
        SimTK::Vector stateValues = model.getStateVariableValues(s);

        int numLoop = 1000;

        std::clock_t testStartTime = std::clock();

        for (int i = 0; i < numLoop; ++i) {
            model.setStateVariableValues(s, stateValues);
            // Directly setting values for coordinates does not ensure they
            // satisfy kinematic constraints. Explicitly enforce constraints
            // by performing an assembly, now.
            model.assemble(s);
        }

        std::clock_t testEndTime = std::clock();
        double elapsed = testEndTime - testStartTime;
        cout << "model.setStateVariableValues elapsed time = "
            << elapsed / CLOCKS_PER_SEC << "s" << endl;
    }

    void testAssembleModelWithConstraints(string modelFile)
    {
        double accuracy = 1e-5;
        using namespace SimTK;

        cout << "\n****************************************************************************" << endl;
        cout << " testAssembleModelWithConstraints with "<< modelFile << endl;
        cout << "****************************************************************************\n" << endl;

        //==========================================================================================================
        // Setup OpenSim model
        Model oldmodel(modelFile);
        string newModelFile = "clone_" + modelFile;
        oldmodel.print(newModelFile);

        Model model(newModelFile);

        const CoordinateSet &coords = model.getCoordinateSet();

        cout << "*********** Coordinates before initSystem ******************** " << endl;
        for(int i=0; i< coords.getSize(); i++) {
            cout << "Coordinate " << coords[i].getName() << " default value = " << coords[i].getDefaultValue() << endl;
        }

        //model.setUseVisualizer(true);
        model.set_assembly_accuracy(accuracy);

        State state = model.initSystem();
        model.equilibrateMuscles(state);

        cout << "*********** Coordinates after initSystem ******************** "  << endl;
        for(int i=0; i< coords.getSize(); i++) {
            cout << "Coordinate " << coords[i].getName() << " get value = " << coords[i].getValue(state) << endl;
        }
        auto coordsInOrder = model.getCoordinateNamesInMultibodyTreeOrder();
        cout << coordsInOrder << std::endl;

        assert(coords.getSize()==coordsInOrder.size());
        // Initial coordinates after initial assembly
        Vector q0 = state.getQ();

        // do assembly again-
        model.assemble(state);

        Vector q0_1 = state.getQ();


        // verify the coordinates do not change within the desired accuracy
        // test the "do-no-harm" rule for assembly
        double qErr0 = (q0_1 - q0).norm();

        cout << "Norm change in q after initial assembly 0: " << qErr0 << endl;
        ASSERT_EQUAL(0.0, qErr0/q0.norm(), accuracy);

        //For debugging the assembled pose
        if (model.hasVisualizer()){
            SimTK::Visualizer& viz = model.updVisualizer().updSimbodyVisualizer();
            const JointSet& js = model.getJointSet();
            for (int i = 0; i < js.getSize(); ++i){
                const Joint& j = js[i];
                viz.addDecoration(j.getParentFrame().getMobilizedBodyIndex(),
                    j.getParentFrame().findTransformInBaseFrame(),
                    SimTK::DecorativeFrame(0.05));
                viz.addDecoration(j.getChildFrame().getMobilizedBodyIndex(),
                    j.getChildFrame().findTransformInBaseFrame(),
                    SimTK::DecorativeFrame(0.033));
                viz.addDecoration(j.getChildFrame().getMobilizedBodyIndex(),
                    Transform(),
                    SimTK::DecorativeSphere(0.033));
                if (j.getChildFrame().getName() == "pelvis"){
                    SimTK::DecorativeBrick geom(Vec3(0.10, 0.05, 0.20));
                    geom.setColor(Vec3(0.1, 1.0, 0.1));
                    viz.addDecoration(j.getChildFrame().getMobilizedBodyIndex(),
                        Transform(), geom);
                }
            }

            model.getVisualizer().show(state);
        }

        // Verify that the reaction forces at the constraints are not ridiculously large
        // They should sum to body-weight (more or less)
        model.getMultibodySystem().realize(state, Stage::Acceleration);

        /*Vec3 comVel = */model.calcMassCenterVelocity(state);
        Vec3 comAcc = model.calcMassCenterAcceleration(state);
        /*SpatialVec momentum = */model.getMatterSubsystem().calcSystemCentralMomentum(state);

        const ConstraintSet &constraints = model.getConstraintSet();

        Vector_<SpatialVec> constraintBodyForces(constraints.getSize());
        Vector mobilityForces(0);
        double totalYforce = 0;

        for(int i=0; i< constraints.getSize(); i++) {
            constraints[i].calcConstraintForces(state, constraintBodyForces, mobilityForces);
            cout << "Constraint " << i << ":  " << constraints[i].getName();
            cout << " Force = " << constraintBodyForces(1)(1)(1) << endl;
            //constraintBodyForces.dump("Constraint Body Forces");
            totalYforce += constraintBodyForces(1)(1)(1);
        }

        cout << "Total Vertical Constraint Force:" << totalYforce << " N " << endl;

        double mass = model.getTotalMass(state);
        double bw = -mass*(model.getGravity()[1]);

        double inertial = mass*comAcc[1];

        ASSERT_EQUAL((totalYforce - bw - inertial) / bw, 0.0, SimTK::SqrtEps,
            __FILE__, __LINE__,
            "Constraint force does not match body-weight plus inertial force (mg+ma).");

        //const CoordinateSet &coords = model.getCoordinateSet();
        double q_error = 0;
        for(int i=0; i< coords.getSize(); i++) {
            q_error += fabs(coords[i].getValue(state)-coords[i].getDefaultValue());
        }

        cout << "Average Change in  Default Configuration:" << q_error/coords.getSize() << endl;

        model.equilibrateMuscles(state);

        // set default (properties) which capture an accurate snapshot of the model
        // prior to simulation.
        model.setPropertiesFromState(state);
        state = model.initSystem();

        //==========================================================================================================
        // Integrate forward and init the state and update defaults to make sure
        // assembler is not affecting anything more than the pose.
        Manager manager(model);
        manager.setIntegratorAccuracy(accuracy);
        state.setTime(0.0);
        manager.initialize(state);

        // Simulate forward in time
        state = manager.integrate(0.05);
        model.getMultibodySystem().realize(state, SimTK::Stage::Velocity);

        Vector positionErr = state.getQErr();
        // int nPerr = positionErr.size();
        // double pErrMag = positionErr.norm();

        // get the configuration at the end of the simulation
        Vector q1 = state.getQ();

        model.updateAssemblyConditions(state);
        // Assemble after the simulation to see how much the assembly changes things
        model.assemble(state);
        Vector q1_1 = state.getQ();
        Vector q1ErrVec = (q1_1 - q1);
        //q1ErrVec.dump("Post simulation: q1_assembled - q1_sim");
        double q1Err = q1ErrVec.norm();

        cout << "Norm change in q after simulation assembly: " << q1Err << endl;
        ASSERT_EQUAL(0.0, q1Err/q1.norm(), accuracy);

        // recreate system with states from initial defaults
        State state0 = model.initSystem();

        // get the configuration after getting a new state from initial defaults
        // to verify that running a simulation doesn't wreck defaults
        Vector q0_2 = state0.getQ();

        // set default (properties) which capture an accurate snapshot of the model
        // post simulation.
        model.setPropertiesFromState(state);

        // recreate system with states from post simulation defaults
        const State& state1 = model.initSystem();
        // get the configuration from post simulation defaults (properties)
        Vector q1_2 = state1.getQ();

        // double q0Err = (q0_2 - q0_1).norm();
        // double q1Err_1 = (q1_2 - q1_1).norm();

        //cout << "******************* Init System Initial State *******************" << endl;
        for (int i = 0; i < q0_1.size(); i++) {
            cout << "Pre-simulation:" << i << " q0_1 = " << q0_1[i] << ", q0_2 = " << q0_2[i] << endl;
            ASSERT_EQUAL(q0_1[i], q0_2[i], 10*accuracy, __FILE__, __LINE__, "Initial state changed after 2nd call to initSystem");
        }

        cout << "******************* Init System Final State *******************" << endl;
        for (int i = 0; i < q1_1.size(); i++) {
            cout << "Post-simulation:" << i << " q1_1 = " << q1_1[i] << ", q1_2 = " << q1_2[i] << endl;
            ASSERT_EQUAL(q1_1[i], q1_2[i], 10 * accuracy, __FILE__, __LINE__, "State differed after a simulation from same init state.");
        }
        ASSERT(max(abs(q1_1 - q0_1)) > 1e-2, __FILE__, __LINE__, "Check that state changed after simulation FAILED");
    }

    double calcLigamentLengthError(const SimTK::State &s, const Model &model)
    {
        using namespace SimTK;
        double error = 0;

        ConstantDistanceConstraint* constraint =
            dynamic_cast<ConstantDistanceConstraint*>(&model.getConstraintSet()[0]);

        if(constraint){
            Vec3 p1inB1, p2inB2, p1inG, p2inG;
            p1inB1 = constraint->get_location_body_1();
            p2inB2 = constraint->get_location_body_2();

            const PhysicalFrame& b1 = constraint->getBody1();
            const PhysicalFrame& b2 = constraint->getBody2();

            p1inG = b1.getTransformInGround(s)*p1inB1;
            p2inG = b2.getTransformInGround(s)*p2inB2;

            double length = (p2inG-p1inG).norm();
            error = length - constraint->get_constant_distance();
        }

        return error;
    }

    void testAssemblySatisfiesConstraints(string modelFile)
    {
        using namespace SimTK;

        cout << "****************************************************************************" << endl;
        cout << " testAssemblySatisfiesConstraints :: " << modelFile << endl;
        cout << "****************************************************************************\n" << endl;
        //==========================================================================================================
        // Setup OpenSim model
        Model model(modelFile);
        model.print(modelFile + "_latest.osim");
        // In Simbody 3.4, rod constraints are handled differently than in Simbody
        // 3.3. This leads to a decrease in the accuracy that the assembly solver
        // achieves, even though the constraints are achieved to the same extent.
        // Therefore, it is reasonable to loosen the accuracy (increase the value
        // of assembly_accuracy) for assembly.
        model.set_assembly_accuracy(1e-8);

        const CoordinateSet &modelcoords = model.getCoordinateSet();
        cout << "*********** Coordinate defaults (before initSystem) ******************** " << endl;
        for(int i=0; i< modelcoords.getSize(); i++) {
            cout << "Coordinate " << modelcoords[i].getName()
                << " default value = " << modelcoords[i].getDefaultValue() << endl
                << " is_free to_satisfy_constraints = " << modelcoords[i].get_is_free_to_satisfy_constraints()
                << endl;
        }

        //model.setUseVisualizer(true);
        State& state = model.initSystem();

        const CoordinateSet &coords = model.getCoordinateSet();
        cout << "***** Coordinate values (after initSystem including Assembly ********* " << endl;
        for(int i=0; i< coords.getSize(); i++) {
            cout << "Coordinate " << coords[i].getName() << " value = "
                << coords[i].getValue(state) << endl;
        }

        double cerr = SimTK::Infinity;
        double kneeAngle = -Pi/3;

        int N = 100;
        double lower = -2*Pi/3, upper = Pi/18;
        double delta = (upper-lower)/N;

        // double qerr = 0;

        for(int i=0; i<N; ++i){
            kneeAngle = upper-i*delta;
            coords[0].setValue(state, kneeAngle, true);
            //        model.getVisualizer().show(state);
            cerr = calcLigamentLengthError(state, model);
            // qerr = coords[0].getValue(state)-kneeAngle;
            //        cout << "Assembly errors:: cerr = " << cerr << " m,  qerr = "
            //          << convertRadiansToDegrees(qerr) << " degrees" << endl;
            ASSERT_EQUAL(0.0, cerr, model.get_assembly_accuracy(),
                __FILE__, __LINE__, "Constraints NOT satisfied to within assembly accuracy");
        }
    }
}


// Measure how long it takes to perform model.setStateVariableValues() on a
// model with constraints to evaluate the effect of assembly in the process.
//~3.5s for CoordinateStateVariable::setValue() enforcing constraints
//~0.18s for CoordinateStateVariable::setValue() NOT enforcing constraints
//       plus explicit Model::assemble() after model.setStateVariableValues()
TEST_CASE("SetStateValues") {
    instrumentSetStateValues("PushUpToesOnGroundLessPreciseConstraints.osim");
}

TEST_CASE("AssemblySatifiesConstraints") {
    testAssemblySatisfiesConstraints("knee_patella_ligament.osim");
}

TEST_CASE("AssembleModelWithConstraints") {
    SECTION("PushUpToesOnGroundExactConstraints") {
        testAssembleModelWithConstraints("PushUpToesOnGroundExactConstraints.osim");
    }
    SECTION("PushUpToesOnGroundLessPreciseConstraints") {
        testAssembleModelWithConstraints("PushUpToesOnGroundLessPreciseConstraints.osim");
    }
    SECTION("PushUpToesOnGroundWithMuscles") {
        testAssembleModelWithConstraints("PushUpToesOnGroundWithMuscles.osim");
    }
}

TEST_CASE("CoordinateCouplerCompoundFunction") {

    // Test that CoordinateCouplerConstraint properly handles functions with
    // multiple independent variables.

    // Helper function.
    auto createConstrainedPendulumModel = [](const Function& f) {
        Model model = ModelFactory::createNLinkPendulum(3);

        auto* constraint = new CoordinateCouplerConstraint();
        constraint->setFunction(f);
        constraint->setDependentCoordinateName("q2");
        Array<std::string> independentCoordinateNames;
        independentCoordinateNames.append("q0");
        independentCoordinateNames.append("q1");
        constraint->setIndependentCoordinateNames(independentCoordinateNames);

        model.addConstraint(constraint);
        model.finalizeConnections();

        return model;
    };

    // Linear function.
    // ----------------
    {
        Array<double> coeffs;
        coeffs.append(1.0);
        coeffs.append(2.0);
        coeffs.append(3.0);
        LinearFunction function(coeffs);
        auto model = createConstrainedPendulumModel(function);
        // Set the initial state to a random value.
        auto state = model.initSystem();
        SimTK::Vector q_rand = SimTK::Test::randVector(model.getNumStateVariables());
        model.setStateVariableValues(state, q_rand);
        model.assemble(state);
        // Compute the constraint error.
        const auto& q = state.getQ();
        auto error = q[2] - (coeffs[0]*q[0] + coeffs[1]*q[1] + coeffs[2]);
        ASSERT_EQUAL(0.0, error, 1e-10, __FILE__, __LINE__,
                "CoordinateCouplerConstraint failed to constrain the dependent "
                "coordinate based on a LinearFunction.");
    }

    // Multi-variate polynomial function.
    // ----------------------------------
    {
        // q2 = c0 + c1*q1 + c2*q1^2 + c3*q0 + c4*q0*q1 + c5*q0^2.
        SimTK::Vector coeffs(6, 1.0);
        coeffs[1] = 2.0;
        coeffs[2] = 3.0;
        coeffs[3] = 4.0;
        coeffs[4] = 5.0;
        coeffs[5] = 6.0;
        MultivariatePolynomialFunction multiPoly(coeffs, 2, 2);
        auto model = createConstrainedPendulumModel(multiPoly);
        // Set the initial state to a random value.
        auto state = model.initSystem();
        SimTK::Vector q_rand = SimTK::Test::randVector(model.getNumStateVariables());
        model.setStateVariableValues(state, q_rand);
        model.assemble(state);
        // Compute the constraint error.
        const auto& q = state.getQ();
        auto polyValue = coeffs[0] + coeffs[1]*q[1] + coeffs[2]*q[1]*q[1] +
            coeffs[3]*q[0] + coeffs[4]*q[0]*q[1] + coeffs[5]*q[0]*q[0];
        auto error = q[2] - polyValue;
        ASSERT_EQUAL(0.0, error, 1e-10, __FILE__, __LINE__,
                "CoordinateCouplerConstraint failed to constrain the dependent "
                "coordinate based on a MultivariatePolynomialFunction.");
    }
}
