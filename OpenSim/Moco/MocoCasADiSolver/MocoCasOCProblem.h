#ifndef OPENSIM_MOCOCASOCPROBLEM_H
#define OPENSIM_MOCOCASOCPROBLEM_H
/* -------------------------------------------------------------------------- *
 * OpenSim: MocoCasOCProblem.h                                                *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2024 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia, Nicholas Bianco                             *
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

#include "CasOCProblem.h"
#include "MocoCasADiSolver.h"

#include <OpenSim/Moco/Components/AccelerationMotion.h>
#include <OpenSim/Moco/Components/ControlDistributor.h>
#include <OpenSim/Moco/Components/DiscreteForces.h>
#include <OpenSim/Moco/MocoBounds.h>
#include <OpenSim/Moco/MocoProblemRep.h>

namespace OpenSim {

using VectorDM = std::vector<casadi::DM>;

inline CasOC::Bounds convertBounds(const MocoBounds& mb) {
    return {mb.getLower(), mb.getUpper()};
}
inline CasOC::Bounds convertBounds(const MocoInitialBounds& mb) {
    return {mb.getLower(), mb.getUpper()};
}
inline CasOC::Bounds convertBounds(const MocoFinalBounds& mb) {
    return {mb.getLower(), mb.getUpper()};
}

/// This converts a SimTK::Matrix to a casadi::DM matrix, transposing the
/// data in the process.
inline casadi::DM convertToCasADiDMTranspose(const SimTK::Matrix& simtkMatrix) {
    casadi::DM out(simtkMatrix.ncol(), simtkMatrix.nrow());
    for (int irow = 0; irow < simtkMatrix.nrow(); ++irow) {
        for (int icol = 0; icol < simtkMatrix.ncol(); ++icol) {
            out(icol, irow) = simtkMatrix(irow, icol);
        }
    }
    return out;
}

template <typename T> casadi::DM convertToCasADiDMTemplate(const T& simtk) {
    casadi::DM out(casadi::Sparsity::dense(simtk.size(), 1));
    std::copy_n(simtk.getContiguousScalarData(), simtk.size(), out.ptr());
    return out;
}
/// This converts a SimTK::RowVector to a casadi::DM column vector.
inline casadi::DM convertToCasADiDMTranspose(const SimTK::RowVector& simtkRV) {
    return convertToCasADiDMTemplate(simtkRV);
}
/// This converts a SimTK::Vector to a casadi::DM column vector.
inline casadi::DM convertToCasADiDM(const SimTK::Vector& simtkVec) {
    return convertToCasADiDMTemplate(simtkVec);
}

/// This converts a MocoTrajectory to a CasOC::Iterate.
inline CasOC::Iterate convertToCasOCIterate(const MocoTrajectory& mocoTraj,
        const std::vector<std::string>& expectedSlackNames,
        bool appendProjectionStates = false,
        std::vector<int> inputControlIndexes = {}) {
    CasOC::Iterate casIt;
    CasOC::VariablesDM& casVars = casIt.variables;
    using CasOC::Var;
    casVars[Var::initial_time] = mocoTraj.getInitialTime();
    casVars[Var::final_time] = mocoTraj.getFinalTime();
    casVars[Var::states] =
            convertToCasADiDMTranspose(mocoTraj.getStatesTrajectory());

    casadi::DM controls =
            convertToCasADiDMTranspose(mocoTraj.getControlsTrajectory());
    casadi::DM input_controls =
            convertToCasADiDMTranspose(mocoTraj.getInputControlsTrajectory());
    std::vector<std::string> controlNames = mocoTraj.getControlNames();
    std::vector<std::string> inputControlNames = mocoTraj.getInputControlNames();
    std::vector<std::string> casControlNames;
    int numTotalControls = static_cast<int>(controls.rows()) +
            static_cast<int>(input_controls.rows());
    casadi::DM casControls(numTotalControls, controls.columns());

    int ic = 0;
    int iic = 0;
    std::sort(inputControlIndexes.begin(), inputControlIndexes.end());
    int numInputControls = static_cast<int>(inputControlIndexes.size());
    for (int i = 0; i < numTotalControls; ++i) {
        if (iic < numInputControls && inputControlIndexes[iic] == i) {
            casControls(i, casadi::Slice()) =
                    input_controls(iic, casadi::Slice());
            casControlNames.push_back(inputControlNames[iic]);
            ++iic;
        } else {
            casControls(i, casadi::Slice()) = controls(ic, casadi::Slice());
            casControlNames.push_back(controlNames[ic]);
            ++ic;
        }
    }
    casVars[Var::controls] = casControls;

    casVars[Var::multipliers] =
            convertToCasADiDMTranspose(mocoTraj.getMultipliersTrajectory());

    if (!mocoTraj.getDerivativeNames().empty()) {
        casVars[Var::derivatives] =
                convertToCasADiDMTranspose(mocoTraj.getDerivativesTrajectory());
    }
    casVars[Var::parameters] =
            convertToCasADiDMTranspose(mocoTraj.getParameters());
    if (appendProjectionStates) {
            casVars[Var::projection_states] = convertToCasADiDMTranspose(
                mocoTraj.getMultibodyStatesTrajectory());
    }
    casIt.times = convertToCasADiDMTranspose(mocoTraj.getTime());
    casIt.state_names = mocoTraj.getStateNames();
    casIt.control_names = casControlNames;
    casIt.multiplier_names = mocoTraj.getMultiplierNames();
    casIt.derivative_names = mocoTraj.getDerivativeNames();
    casIt.parameter_names = mocoTraj.getParameterNames();

    // Projection state variables.
    // ---------------------------
    // Extra variables needed when using the projection method for enforcing
    // kinematic constraints from Bordalba et al. (2023).
    if (appendProjectionStates) {
        auto mbStateNames = mocoTraj.getMultibodyStateNames();
        for (auto name : mbStateNames) {
            auto valuepos = name.find("/value");
            if (valuepos != std::string::npos) {
                name.replace(valuepos, 6, "/value/projection");
                casIt.projection_state_names.push_back(name);
            }
            auto speedpos = name.find("/speed");
            if (speedpos != std::string::npos) {
                name.replace(speedpos, 6, "/speed/projection");
                casIt.projection_state_names.push_back(name);
            }
        }
    }
    // Slack variables.
    // ----------------
    // Check that the trajectory has the expected slack names from the
    // CasOCProblem.
    bool matchedExpectedSlackNames =
            mocoTraj.getSlackNames().size() == expectedSlackNames.size();
    if (matchedExpectedSlackNames) {
        for (const auto& expectedName : expectedSlackNames) {
            if (std::find(mocoTraj.getSlackNames().begin(),
                        mocoTraj.getSlackNames().end(), expectedName) ==
                    mocoTraj.getSlackNames().end()) {
                matchedExpectedSlackNames = false;
                break;
            }
        }
    }

    // If the guess matches the expected slack names, use the slack values from
    // the guess. If not, create a vector of zeros to use as the initial guess
    // for the slack variables.
    if (matchedExpectedSlackNames) {
        casIt.slack_names = mocoTraj.getSlackNames();
        if (!mocoTraj.getSlackNames().empty()) {
            casVars[Var::slacks] =
                    convertToCasADiDMTranspose(mocoTraj.getSlacksTrajectory());
        }
    } else {
        casIt.slack_names = expectedSlackNames;
        if (!expectedSlackNames.empty()) {
            casVars[Var::slacks] = casadi::DM::zeros(
                    (int)expectedSlackNames.size(), mocoTraj.getNumTimes());
        }
    }

    return casIt;
}

template <typename VectorType = SimTK::Vector>
VectorType convertToSimTKVector(const casadi::DM& casVector) {
    OPENSIM_THROW_IF(casVector.columns() != 1 && casVector.rows() != 1,
            Exception,
            "casVector should be 1-dimensional, but has size {} x {}.",
            casVector.rows(), casVector.columns());
    VectorType simtkVector((int)casVector.numel());
    for (int i = 0; i < casVector.numel(); ++i) {
        simtkVector[i] = double(casVector(i));
    }
    return simtkVector;
}

/// This converts a casadi::DM matrix to a
/// SimTK::Matrix, transposing the data in the process.
inline SimTK::Matrix convertToSimTKMatrix(const casadi::DM& casMatrix) {
    SimTK::Matrix simtkMatrix((int)casMatrix.columns(), (int)casMatrix.rows());
    for (int irow = 0; irow < casMatrix.rows(); ++irow) {
        for (int icol = 0; icol < casMatrix.columns(); ++icol) {
            simtkMatrix(icol, irow) = double(casMatrix(irow, icol));
        }
    }
    return simtkMatrix;
}

template <typename TOut = MocoTrajectory>
TOut convertToMocoTrajectory(const CasOC::Iterate& casIt,
        std::vector<int> inputControlIndexes = {}) {
    SimTK::Matrix simtkStates;
    const auto& casVars = casIt.variables;
    using CasOC::Var;
    if (!casIt.state_names.empty()) {
        simtkStates = convertToSimTKMatrix(casVars.at(Var::states));
    }
    int numTotalControls = static_cast<int>(casIt.control_names.size());
    int numInputControls = static_cast<int>(inputControlIndexes.size());
    int numControls = numTotalControls - numInputControls;
    OPENSIM_ASSERT(numControls >= 0);
    SimTK::Matrix simtkControls;
    SimTK::Matrix simtkInputControls;
    std::vector<std::string> controlNames;
    std::vector<std::string> inputControlNames;
    if (numTotalControls) {
        SimTK::Matrix allControls =
                convertToSimTKMatrix(casVars.at(Var::controls));
        simtkControls.resize(allControls.nrow(), numControls);
        simtkInputControls.resize(allControls.nrow(), numInputControls);
        int ic = 0;
        int iic = 0;
        std::sort(inputControlIndexes.begin(), inputControlIndexes.end());
        for (int i = 0; i < allControls.ncol(); ++i) {
            if (iic < numInputControls && inputControlIndexes[iic] == i) {
                simtkInputControls.updCol(iic) = allControls.col(i);
                inputControlNames.push_back(casIt.control_names[i]);
                ++iic;
            } else {
                simtkControls.updCol(ic) = allControls.col(i);
                controlNames.push_back(casIt.control_names[i]);
                ++ic;
            }
        }
    }
    SimTK::Matrix simtkMultipliers;
    if (!casIt.multiplier_names.empty()) {
        const auto multsValue = casVars.at(Var::multipliers);
        simtkMultipliers = convertToSimTKMatrix(multsValue);
    }
    SimTK::Matrix simtkSlacks;
    if (!casIt.slack_names.empty()) {
        const auto slacksValue = casVars.at(Var::slacks);
        simtkSlacks = convertToSimTKMatrix(slacksValue);
    }
    SimTK::Matrix simtkDerivatives;
    auto derivativeNames = casIt.derivative_names;
    if (casVars.count(Var::derivatives) &&
            casVars.at(Var::derivatives).numel()) {
        const auto derivsValue = casVars.at(Var::derivatives);
        simtkDerivatives = convertToSimTKMatrix(derivsValue);
    } else {
        derivativeNames.clear();
    }
    SimTK::RowVector simtkParameters;
    if (!casIt.parameter_names.empty()) {
        const auto paramsValue = casVars.at(Var::parameters);
        simtkParameters = convertToSimTKVector<SimTK::RowVector>(paramsValue);
    }
    SimTK::Vector simtkTimes = convertToSimTKVector(casIt.times);

    TOut mocoTraj(simtkTimes, casIt.state_names, controlNames,
            inputControlNames, casIt.multiplier_names, derivativeNames,
            casIt.parameter_names, simtkStates, simtkControls,
            simtkInputControls, simtkMultipliers, simtkDerivatives,
            simtkParameters);

    // Append slack variables. MocoTrajectory requires the slack variables to be
    // the same length as its time vector, but it might not be if the
    // CasOC::Iterate was generated from a CasOC::Transcription object.
    // Therefore, slack variables are interpolated as necessary.
    if (!casIt.slack_names.empty()) {
        int simtkSlacksLength = simtkSlacks.nrow();
        SimTK::Vector slackTime = createVectorLinspace(simtkSlacksLength,
                simtkTimes[0], simtkTimes[simtkTimes.size() - 1]);
        for (int i = 0; i < (int)casIt.slack_names.size(); ++i) {
            if (simtkSlacksLength != simtkTimes.size()) {
                mocoTraj.appendSlack(casIt.slack_names[i],
                        interpolate(slackTime, simtkSlacks.col(i), simtkTimes,
                                    true, true));
            } else {
                mocoTraj.appendSlack(casIt.slack_names[i], simtkSlacks.col(i));
            }
        }
    }
    return mocoTraj;
}

/// This class is the bridge between CasOC::Problem and MocoProblemRep. Inputs
/// are CasADi types, which are converted to SimTK types to evaluate problem
/// functions. Then, results are converted back into CasADi types.
class MocoCasOCProblem : public CasOC::Problem {
public:
    MocoCasOCProblem(const MocoCasADiSolver& mocoCasADiSolver,
            const MocoProblemRep& mocoProblemRep,
            std::unique_ptr<ThreadsafeJar<const MocoProblemRep>> jar,
            std::string dynamicsMode,
            std::string kinematicConstraintMethod);

    int getJarSize() const { return (int)m_jar->size(); }

private:
    void calcMultibodySystemExplicit(const ContinuousInput& input,
            bool calcKCErrors,
            MultibodySystemExplicitOutput& output) const override {
        auto mocoProblemRep = m_jar->take();

        const auto& modelBase = mocoProblemRep->getModelBase();
        auto& simtkStateBase = mocoProblemRep->updStateBase();

        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();
        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints();

        applyInput(SimTK::Stage::Acceleration, input.time, input.states,
                input.controls, input.multipliers, input.derivatives,
                input.parameters, mocoProblemRep);

        // Compute the accelerations.
        modelDisabledConstraints.realizeAcceleration(
                simtkStateDisabledConstraints);

        // Compute kinematic constraint errors.
        if (getNumMultipliers() && calcKCErrors) {
            calcKinematicConstraintErrors(
                    modelBase, simtkStateBase,
                    simtkStateDisabledConstraints,
                    output.kinematic_constraint_errors);
        }

        // Copy state derivative values to output.
        const auto& udot = simtkStateDisabledConstraints.getUDot();
        const auto& zdot = simtkStateDisabledConstraints.getZDot();
        std::copy_n(udot.getContiguousScalarData(), udot.size(),
                output.multibody_derivatives.ptr());
        std::copy_n(zdot.getContiguousScalarData(), zdot.size(),
                output.auxiliary_derivatives.ptr());

        // Copy auxiliary residuals to output.
        copyImplicitResidualsToOutput(*mocoProblemRep,
                simtkStateDisabledConstraints, output.auxiliary_residuals);

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcMultibodySystemImplicit(const ContinuousInput& input,
            bool calcKCErrors,
            MultibodySystemImplicitOutput& output) const override {
        auto mocoProblemRep = m_jar->take();

        // Original model and its associated state. These are used to calculate
        // kinematic constraint forces and errors.
        const auto& modelBase = mocoProblemRep->getModelBase();
        auto& simtkStateBase = mocoProblemRep->updStateBase();

        // Model with disabled constraints and its associated state. These are
        // used to compute the accelerations.
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();
        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints();

        applyInput(SimTK::Stage::Acceleration, input.time, input.states,
                input.controls, input.multipliers, input.derivatives,
                input.parameters, mocoProblemRep);

        modelDisabledConstraints.realizeAcceleration(
                simtkStateDisabledConstraints);

        // Compute kinematic constraint errors.
        if (getNumMultipliers() && calcKCErrors) {
            calcKinematicConstraintErrors(
                    modelBase, simtkStateBase,
                    simtkStateDisabledConstraints,
                    output.kinematic_constraint_errors);
        }

        const SimTK::SimbodyMatterSubsystem& matterDisabledConstraints =
                modelDisabledConstraints.getMatterSubsystem();
        SimTK::Vector simtkResidual((int)output.multibody_residuals.rows(),
                output.multibody_residuals.ptr(), true);
        matterDisabledConstraints.findMotionForces(
                simtkStateDisabledConstraints, simtkResidual);

        // Copy auxiliary dynamics to output.
        const auto& zdot = simtkStateDisabledConstraints.getZDot();
        std::copy_n(zdot.getContiguousScalarData(), zdot.size(),
                output.auxiliary_derivatives.ptr());

        // Copy auxiliary residuals to output.
        copyImplicitResidualsToOutput(*mocoProblemRep,
                simtkStateDisabledConstraints, output.auxiliary_residuals);

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcVelocityCorrection(const double& time,
            const casadi::DM& multibody_states, const casadi::DM& slacks,
            const casadi::DM& parameters,
            casadi::DM& velocity_correction) const override {
        if (isPrescribedKinematics()) return;
        auto mocoProblemRep = m_jar->take();

        const auto& modelBase = mocoProblemRep->getModelBase();
        auto& simtkStateBase = mocoProblemRep->updStateBase();

        // Update the model and state.
        applyParametersToModelProperties(parameters, *mocoProblemRep);
        convertStatesToSimTKState(
                SimTK::Stage::Velocity, time, multibody_states,
                modelBase, simtkStateBase, false);
        modelBase.realizeVelocity(simtkStateBase);

        // Apply velocity correction to qdot if at a mesh interval midpoint.
        // This correction modifies the dynamics to enable a projection of
        // the model coordinates back onto the constraint manifold whenever
        // they deviate.
        // Posa, Kuindersma, Tedrake, 2016. "Optimization and stabilization
        // of trajectories for constrained dynamical systems"
        // Note: Only supported for the Hermite-Simpson transcription
        // scheme.
        const SimTK::SimbodyMatterSubsystem& matterBase =
                modelBase.getMatterSubsystem();

        SimTK::Vector gamma(getNumSlacks(), slacks.ptr(), true);
        SimTK::Vector qdotCorr((int)velocity_correction.rows(),
                velocity_correction.ptr(), true);
        matterBase.multiplyByGTranspose(simtkStateBase, gamma, qdotCorr);

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcStateProjection(const double& time,
            const casadi::DM& multibody_states, const casadi::DM& slacks,
            const casadi::DM& parameters,
            casadi::DM& projection) const override {
        if (isPrescribedKinematics()) return;
        auto mocoProblemRep = m_jar->take();

        const auto& modelBase = mocoProblemRep->getModelBase();
        auto& simtkStateBase = mocoProblemRep->updStateBase();

        // Update the model and state.
        applyParametersToModelProperties(parameters, *mocoProblemRep);
        convertStatesToSimTKState(
                SimTK::Stage::Velocity, time, multibody_states,
                modelBase, simtkStateBase, false);
        modelBase.realizeVelocity(simtkStateBase);

        // Compute the state projection vector based on the method by Bordalba
        // et al. (2023). Our implementation looks slightly different from the
        // projection constraints in the manuscript since we compute the
        // projections for the coordinate values and coordinate speeds
        // separately based on how Simbody's assembler handles coordinate
        // projections for kinematic constraints.
        const SimTK::SimbodyMatterSubsystem& matterBase =
                modelBase.getMatterSubsystem();

        // Holonomic constraint errors.
        const SimTK::Vector mu_p(
                getNumHolonomicConstraintEquations(), slacks.ptr(), true);
        SimTK::Vector proj_p(getNumCoordinates(), projection.ptr(), true);
        matterBase.multiplyByPqTranspose(simtkStateBase, mu_p, proj_p);

        // Derivative of holonomic constraint errors and non-holonomic
        // constraint errors.
        const SimTK::Vector mu_v(
                getNumHolonomicConstraintEquations() +
                getNumNonHolonomicConstraintEquations(),
                slacks.ptr() + getNumHolonomicConstraintEquations(), true);
        SimTK::Vector proj_v(getNumSpeeds(), projection.ptr() +
                getNumCoordinates(), true);
        matterBase.multiplyByPVTranspose(simtkStateBase, mu_v, proj_v);

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcCostIntegrand(int index, const ContinuousInput& input,
            double& integrand) const override {
        auto mocoProblemRep = m_jar->take();

        const auto& mocoCost = mocoProblemRep->getCostByIndex(index);
        const auto stageDep = mocoCost.getStageDependency();
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();

        applyInput(stageDep, input.time, input.states, input.controls,
                input.multipliers, input.derivatives, input.parameters,
                mocoProblemRep);

        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints();
        const SimTK::Vector& controls = mocoProblemRep->getControls(
                simtkStateDisabledConstraints);

        integrand = mocoCost.calcIntegrand(
                {input.time, simtkStateDisabledConstraints, controls});

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcCost(int index, const CostInput& input,
            casadi::DM& cost) const override {
        auto mocoProblemRep = m_jar->take();

        const auto& mocoCost = mocoProblemRep->getCostByIndex(index);
        const auto stageDep = mocoCost.getStageDependency();
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();

        applyInput(stageDep, input.initial_time, input.initial_states,
                input.initial_controls, input.initial_multipliers,
                input.initial_derivatives, input.parameters, mocoProblemRep, 0);
        auto& simtkStateDisabledConstraintsInitial =
                mocoProblemRep->updStateDisabledConstraints(0);
        const SimTK::Vector& controlsInitial = mocoProblemRep->getControls(
                simtkStateDisabledConstraintsInitial);

        applyInput(stageDep, input.final_time, input.final_states,
                input.final_controls, input.final_multipliers,
                input.final_derivatives, input.parameters, mocoProblemRep, 1);
        auto& simtkStateDisabledConstraintsFinal =
                mocoProblemRep->updStateDisabledConstraints(1);
        const SimTK::Vector& controlsFinal = mocoProblemRep->getControls(
                simtkStateDisabledConstraintsFinal);

        // Compute the cost for this cost term.
        SimTK::Vector simtkCost((int)cost.rows(), cost.ptr(), true);
        mocoCost.calcGoal(
                {input.initial_time, simtkStateDisabledConstraintsInitial,
                        controlsInitial, input.final_time,
                        simtkStateDisabledConstraintsFinal, controlsFinal,
                        input.integral},
                simtkCost);

        m_jar->leave(std::move(mocoProblemRep));
    }

    void calcEndpointConstraintIntegrand(int index,
            const ContinuousInput& input, double& integrand) const override {
        auto mocoProblemRep = m_jar->take();

        const auto& mocoEC =
                mocoProblemRep->getEndpointConstraintByIndex(index);
        const auto stageDep = mocoEC.getStageDependency();
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();

        applyInput(stageDep, input.time, input.states, input.controls,
                input.multipliers, input.derivatives, input.parameters,
                mocoProblemRep);

        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints();
        const SimTK::Vector& controls = mocoProblemRep->getControls(
                simtkStateDisabledConstraints);

        integrand = mocoEC.calcIntegrand(
                {input.time, simtkStateDisabledConstraints, controls});

        m_jar->leave(std::move(mocoProblemRep));
    }
    void calcEndpointConstraint(int index, const CostInput& input,
            casadi::DM& values) const override {
        auto mocoProblemRep = m_jar->take();

        const auto& mocoEC =
                mocoProblemRep->getEndpointConstraintByIndex(index);
        const auto stageDep = mocoEC.getStageDependency();
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();

        applyInput(stageDep, input.initial_time, input.initial_states,
                input.initial_controls, input.initial_multipliers,
                input.initial_derivatives, input.parameters, mocoProblemRep, 0);
        auto& simtkStateDisabledConstraintsInitial =
                mocoProblemRep->updStateDisabledConstraints(0);
        const SimTK::Vector& controlsInitial = mocoProblemRep->getControls(
                simtkStateDisabledConstraintsInitial);

        applyInput(stageDep, input.final_time, input.final_states,
                input.final_controls, input.final_multipliers,
                input.final_derivatives, input.parameters, mocoProblemRep, 1);
        auto& simtkStateDisabledConstraintsFinal =
                mocoProblemRep->updStateDisabledConstraints(1);
        const SimTK::Vector& controlsFinal = mocoProblemRep->getControls(
                simtkStateDisabledConstraintsFinal);

        // Compute the cost for this cost term.
        SimTK::Vector simtkValues((int)values.rows(), values.ptr(), true);
        mocoEC.calcGoal(
                {input.initial_time, simtkStateDisabledConstraintsInitial,
                        controlsInitial, input.final_time,
                        simtkStateDisabledConstraintsFinal,
                        controlsFinal, input.integral},
                simtkValues);

        m_jar->leave(std::move(mocoProblemRep));
    }

    void calcPathConstraint(int constraintIndex, const ContinuousInput& input,
            casadi::DM& path_constraint) const override {
        auto mocoProblemRep = m_jar->take();
        // Not all path constraints require realizing to Acceleration. We could
        // add a stage dependency for path constraints, but we have yet to
        // conduct profiling to indicate that such an optimization is necessary.
        applyInput(SimTK::Stage::Acceleration,
                input.time, input.states, input.controls, input.multipliers,
                input.derivatives, input.parameters, mocoProblemRep);
        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints();

        // Compute path constraint errors.
        const auto& mocoPathCon =
                mocoProblemRep->getPathConstraintByIndex(constraintIndex);
        SimTK::Vector errors(
                (int)path_constraint.rows(), path_constraint.ptr(), true);
        mocoPathCon.calcPathConstraintErrors(
                simtkStateDisabledConstraints, errors);

        m_jar->leave(std::move(mocoProblemRep));
    }
    std::vector<std::string>
    createKinematicConstraintEquationNamesImpl() const override {
        auto mocoProblemRep = m_jar->take();
        const auto names = mocoProblemRep->getKinematicConstraintEquationNames(
                getEnforceConstraintDerivatives());
        m_jar->leave(std::move(mocoProblemRep));
        return names;
    }
    void intermediateCallbackImpl() const override {
        m_fileDeletionThrower->throwIfDeleted();
    }
    void intermediateCallbackWithIterateImpl(
            const CasOC::Iterate& iterate) const override {
        std::string filename =
                fmt::format("MocoCasADiSolver_{}_trajectory{:06d}.sto",
                        m_formattedTimeString, iterate.iteration);
        convertToMocoTrajectory(iterate).write(filename);
    }

private:
    /// Apply parameters to properties in the models returned by
    /// `mocoProblemRep.getModelBase()` and
    /// `mocoProblemRep.getModelDisabledConstraints()`.
    void applyParametersToModelProperties(const casadi::DM& parameters,
            const MocoProblemRep& mocoProblemRep) const {
        if (parameters.numel()) {
            SimTK::Vector simtkParams(
                    (int)parameters.size1(), parameters.ptr(), true);
            mocoProblemRep.applyParametersToModelProperties(
                    simtkParams, m_paramsRequireInitSystem);
        }
    }
    /// Copy values from `states` into `simtkState.updY()`, accounting for empty
    /// slots in Simbody's Y vector.
    /// It's fine for the size of `states` to be less than the size of Y; only
    /// the first states.size1() values are copied.
    void convertStatesToSimTKState(SimTK::Stage stageDep, const double& time,
            const casadi::DM& states, const Model& model,
            SimTK::State& simtkState, bool copyAuxStates) const {
        if (stageDep >= SimTK::Stage::Time) {
            simtkState.setTime(time);
            // Assign the generalized coordinates. We know we have NU
            // generalized speeds because we do not yet support quaternions.
            for (int isv = 0; isv < getNumCoordinates(); ++isv) {
                simtkState.updQ()[m_yIndexMap.at(isv)] = *(states.ptr() + isv);
            }
            std::copy_n(states.ptr() + getNumCoordinates(), getNumSpeeds(),
                    simtkState.updY().updContiguousScalarData() +
                            simtkState.getNQ());
            if (copyAuxStates) {
                std::copy_n(states.ptr() + getNumCoordinates() + getNumSpeeds(),
                        getNumAuxiliaryStates(),
                        simtkState.updY().updContiguousScalarData() +
                                simtkState.getNQ() + simtkState.getNU());
            }
            // Prescribing motion requires that time is updated.
            model.getSystem().prescribe(simtkState);
        }
    }

    /// Invoke convertStatesToSimTKState() and also
    /// copy values from `controls` into the discrete state variable managed
    /// by the `discreteController`. We assume that if we need the controls
    /// copied over, we likely are going to compute forces with the resulting
    /// state, and so we should also copy over the auxiliary states; we pass
    /// true for the copyAuxStates parameter of convertStatesToSimTKState().
    void convertStatesControlsToSimTKState(SimTK::Stage stageDep,
            const double& time,
            const casadi::DM& states, const casadi::DM& controls,
            const Model& model, SimTK::State& simtkState,
            const ControlDistributor& controlDistributor) const {
        if (stageDep >= SimTK::Stage::Model) {
            convertStatesToSimTKState(
                    stageDep, time, states, model, simtkState, true);

            SimTK::Vector& simtkControls =
                    controlDistributor.updControls(simtkState);
            for (int ic = 0; ic < getNumControls(); ++ic) {
                simtkControls[ic] = *(controls.ptr() + ic);
            }
            // Updating the Inputs to InputControllers via the
            // ControlDistributor does not mark the model controls cache as
            // invalid, so we must do it manually here.
            model.markControlsAsInvalid(simtkState);
        }
    }
    /// Apply variables from the optimizer to the MocoProblemRep's model and
    /// state. The `stageDep` determines which information from the optimizer
    /// must be carried over to the model/state.
    void applyInput(SimTK::Stage stageDep, const double& time,
            const casadi::DM& states, const casadi::DM& controls,
            const casadi::DM& multipliers, const casadi::DM& derivatives,
            const casadi::DM& parameters,
            const std::unique_ptr<const MocoProblemRep>& mocoProblemRep,
            int stateDisConIndex = 0) const {
        // Original model and its associated state. These are used to calculate
        // kinematic constraint forces and errors.
        const auto& modelBase = mocoProblemRep->getModelBase();
        auto& simtkStateBase = mocoProblemRep->updStateBase();

        // Model with disabled constraints and its associated state. These are
        // used to compute the accelerations.
        const auto& modelDisabledConstraints =
                mocoProblemRep->getModelDisabledConstraints();
        auto& simtkStateDisabledConstraints =
                mocoProblemRep->updStateDisabledConstraints(stateDisConIndex);

        // Update the model and state.
        if (stageDep >= SimTK::Stage::Instance) {
            applyParametersToModelProperties(parameters, *mocoProblemRep);
        }

        if (stageDep >= SimTK::Stage::Acceleration && getNumAccelerations()) {
            auto& accel = mocoProblemRep->getAccelerationMotion();
            accel.setEnabled(simtkStateDisabledConstraints, true);
            SimTK::Vector udot(getNumAccelerations(), derivatives.ptr(), true);
            accel.setUDot(simtkStateDisabledConstraints, udot);
        }

        // Set discrete variables that represent state derivatives in implicit
        // auxiliary dynamics.
        // Such discrete variables likely only affect force calculations, so
        // we could perhaps use `stageDep >= SimTK::Stage::Dynamics`, but we
        // use SimTK::Stage::Model to be safe here. Continuous state
        // variables are available at SimTK::Stage::Model, so it's consistent
        // for the discrete variables to be available at SimTK::Stage::Model
        // also. Lastly, goals might be a direct function of these state
        // derivatives.
        if (stageDep >= SimTK::Stage::Model &&
                getNumAuxiliaryResidualEquations()) {
            const auto& implicitRefs =
                    mocoProblemRep->getImplicitComponentReferencePtrs();
            const int numAccels = getNumAccelerations();
            for (int i = 0; i < (int)implicitRefs.size(); ++i) {
                const auto& comp = implicitRefs[i].second.getRef();
                comp.setDiscreteVariableValue(simtkStateDisabledConstraints,
                        implicitRefs[i].first,
                        *(derivatives.ptr() + numAccels + i));
            }
        }

        convertStatesControlsToSimTKState(stageDep, time, states, controls,
                modelDisabledConstraints, simtkStateDisabledConstraints,
                mocoProblemRep->getControlDistributorDisabledConstraints());

        // If enabled constraints exist in the model, compute constraint forces
        // based on Lagrange multipliers. This also updates the associated
        // discrete variables in the state.
        if (stageDep >= SimTK::Stage::Dynamics && getNumMultipliers()) {
            // The base model is used only to compute constraint forces, so
            // we only need to update it if there are kinematic constraints.
            // We pass copyAuxStates as false: we use the base model for its
            // constraint Jacobian, which depends only on kinematics and cannot
            // depend on auxiliary states.
            convertStatesToSimTKState(
                    stageDep, time, states, modelBase, simtkStateBase, false);
            calcKinematicConstraintForces(multipliers, simtkStateBase,
                    modelBase, mocoProblemRep->getConstraintForces(),
                    simtkStateDisabledConstraints);
        }
    }

    static void calcKinematicConstraintForces(const casadi::DM& multipliers,
            const SimTK::State& stateBase, const Model& modelBase,
            const DiscreteForces& constraintForces,
            SimTK::State& stateDisabledConstraints) {
        // Calculate the constraint forces using the original model and the
        // solver-provided Lagrange multipliers.
        modelBase.realizeVelocity(stateBase);
        const auto& matterBase = modelBase.getMatterSubsystem();
        SimTK::Vector simtkMultipliers(
                (int)multipliers.size1(), multipliers.ptr(), true);
        // Multipliers are negated so constraint forces can be used like
        // applied forces.
        matterBase.calcConstraintForcesFromMultipliers(stateBase,
                -simtkMultipliers, m_constraintBodyForces,
                m_constraintMobilityForces);

        // Apply the constraint forces on the model with disabled constraints.
        constraintForces.setAllForces(stateDisabledConstraints,
                m_constraintMobilityForces, m_constraintBodyForces);
    }

    void calcKinematicConstraintErrors(
            const Model& modelBase,
            const SimTK::State& stateBase,
            const SimTK::State& simtkStateDisabledConstraints,
            casadi::DM& kinematic_constraint_errors) const {

        // If all kinematics are prescribed, we assume that the prescribed
        // kinematics obey any kinematic constraints. Therefore, the kinematic
        // constraints would be redundant, and we need not enforce them.
        if (isPrescribedKinematics()) return;

        // Calculate udoterr. We cannot use State::getUDotErr()
        // because that uses Simbody's multipliers and UDot,
        // whereas we have our own multipliers and UDot. Here, we use
        // the udot computed from the model with disabled constraints
        // since we cannot use (nor do we have available) udot computed
        // from the original model.
        if (getEnforceConstraintDerivatives() ||
                getNumAccelerationConstraintEquations()) {
            const auto& matter = modelBase.getMatterSubsystem();
            matter.calcConstraintAccelerationErrors(stateBase,
                    simtkStateDisabledConstraints.getUDot(), m_pvaerr);
        } else {
            m_pvaerr = SimTK::NaN;
        }

        // Position-level errors.
        const auto& qerr = stateBase.getQErr();
        // Velocity-level errors.
        const auto& uerr = stateBase.getUErr();
        // Acceleration-level errors.
        const auto& udoterr = m_pvaerr;

        // This way of copying the data avoids a threadsafety issue in
        // CasADi related to cached Sparsity objects.
        std::copy_n(qerr.getContiguousScalarData(), qerr.size(),
                kinematic_constraint_errors.ptr());
        std::copy_n(uerr.getContiguousScalarData() + m_uerrOffset, m_uerrSize,
                kinematic_constraint_errors.ptr() + qerr.size());
        std::copy_n(udoterr.getContiguousScalarData() + m_udoterrOffset,
                m_udoterrSize,
                kinematic_constraint_errors.ptr() + qerr.size() + m_uerrSize);

    }

    void copyImplicitResidualsToOutput(const MocoProblemRep& mocoProblemRep,
            const SimTK::State& state, casadi::DM& auxiliary_residuals) const {
        if (getNumAuxiliaryResidualEquations()) {
            const auto& residualOutputs =
                    mocoProblemRep.getImplicitResidualReferencePtrs();
            SimTK::Vector auxResiduals((int)residualOutputs.size(), 0.0);
            for (int i = 0; i < (int)residualOutputs.size(); ++i) {
                auxResiduals[i] = residualOutputs[i]->getValue(state);
            }
            std::copy_n(auxResiduals.getContiguousScalarData(),
                    auxResiduals.size(), auxiliary_residuals.ptr());
        }
    }

    std::unique_ptr<ThreadsafeJar<const MocoProblemRep>> m_jar;
    bool m_paramsRequireInitSystem = true;
    std::string m_formattedTimeString;
    std::unordered_map<int, int> m_yIndexMap;
    std::unique_ptr<FileDeletionThrower> m_fileDeletionThrower;
    // Local memory to hold constraint forces.
    static thread_local SimTK::Vector_<SimTK::SpatialVec>
            m_constraintBodyForces;
    static thread_local SimTK::Vector m_constraintMobilityForces;
    // This is the output argument of
    // SimbodyMatterSubsystem::calcConstraintAccelerationErrors(), and includes
    // the acceleration-level holonomic, non-holonomic constraint errors and the
    // acceleration-only constraint errors.
    static thread_local SimTK::Vector m_pvaerr;
    // These offsets are necessary when not enforcing the derivatives of the
    // kinematic constraint equations.
    int m_uerrOffset;
    int m_uerrSize;
    int m_udoterrOffset;
    int m_udoterrSize;
};

} // namespace OpenSim

#endif // OPENSIM_MOCOCASOCPROBLEM_H
