/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoCasOCSolver.cpp                                          *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2018 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 * * * Unless required by applicable law or agreed to in writing, software *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "CasOCProblem.h"
#include "CasOCTranscription.h"
#include "CasOCTrapezoidal.h"
#include "CasOCHermiteSimpson.h"
#include "CasOCLegendreGauss.h"
#include "CasOCLegendreGaussRadau.h"

#include <OpenSim/Moco/MocoUtilities.h>

using OpenSim::Exception;

namespace CasOC {

std::unique_ptr<Transcription> Solver::createTranscription() const {
    std::unique_ptr<Transcription> transcription;
    if (m_transcriptionScheme == "trapezoidal") {
        transcription = std::make_unique<Trapezoidal>(*this, m_problem);
    } else if (m_transcriptionScheme == "hermite-simpson") {
        transcription = std::make_unique<HermiteSimpson>(*this, m_problem);
    } else if (m_transcriptionScheme == "legendre-gauss-1") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 1);
    } else if (m_transcriptionScheme == "legendre-gauss-2") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 2);
    } else if (m_transcriptionScheme == "legendre-gauss-3") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 3);
    } else if (m_transcriptionScheme == "legendre-gauss-4") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 4);
    } else if (m_transcriptionScheme == "legendre-gauss-5") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 5);
    } else if (m_transcriptionScheme == "legendre-gauss-6") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 6);
    } else if (m_transcriptionScheme == "legendre-gauss-7") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 7);
    } else if (m_transcriptionScheme == "legendre-gauss-8") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 8);
    } else if (m_transcriptionScheme == "legendre-gauss-9") {
        transcription = std::make_unique<LegendreGauss>(*this, m_problem, 9);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-1") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 1);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-2") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 2);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-3") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 3);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-4") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 4);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-5") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 5);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-6") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 6);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-7") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 7);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-8") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 8);
    } else if (m_transcriptionScheme == "legendre-gauss-radau-9") {
        transcription = std::make_unique<LegendreGaussRadau>(
                *this, m_problem, 9);
    } else {
        OPENSIM_THROW(Exception, "Unknown transcription scheme '{}'.",
                m_transcriptionScheme);
    }
    return transcription;
}

Iterate Solver::createInitialGuessFromBounds() const {
    auto transcription = createTranscription();
    return transcription->createInitialGuessFromBounds();
}

Iterate Solver::createRandomIterateWithinBounds() const {
    auto transcription = createTranscription();
    return transcription->createRandomIterateWithinBounds();
}

void Solver::setSparsityDetection(const std::string& setting) {
    OPENSIM_THROW_IF(setting != "none" && setting != "random" &&
                             setting != "initial-guess",
            Exception);
    m_sparsity_detection = setting;
}

void Solver::setSparsityDetectionRandomCount(int count) {
    OPENSIM_THROW_IF(count <= 0, Exception);
    m_sparsity_detection_random_count = count;
}

void Solver::setParallelism(std::string parallelism, int numThreads) {
    m_parallelism = parallelism;
    OPENSIM_THROW_IF(numThreads < 1, OpenSim::Exception,
            "Expected numThreads >= 1 but got {}.", numThreads);
    m_numThreads = numThreads;
}

Solution Solver::solve(const Iterate& guess) const {
    auto transcription = createTranscription();
    auto pointsForSparsityDetection =
            std::make_shared<std::vector<VariablesDM>>();
    if (m_sparsity_detection == "initial-guess") {
        // Interpolate the guess.
        Iterate guessCopy(guess);
        const auto guessTimes =
                transcription->createTimes(guessCopy.variables.at(initial_time),
                        guessCopy.variables.at(final_time));
        bool appendProjectionStates =
                m_problem.getNumKinematicConstraintEquations() &&
                m_problem.isKinematicConstraintMethodBordalba2023();
        guessCopy = guessCopy.resample(guessTimes, appendProjectionStates);
        pointsForSparsityDetection->push_back(guessCopy.variables);
    } else if (m_sparsity_detection == "random") {
        // Make sure the exact same sparsity pattern is used every time.
        auto randGen = std::make_unique<SimTK::Random::Uniform>(-1, 1);
        randGen->setSeed(0);
        for (int i = 0; i < m_sparsity_detection_random_count; ++i) {
            pointsForSparsityDetection->push_back(
                    transcription->createRandomIterateWithinBounds(
                                         randGen.get())
                            .variables);
        }
    }
    m_problem.initialize(m_finite_difference_scheme,
            std::const_pointer_cast<const std::vector<VariablesDM>>(
                    pointsForSparsityDetection));
    return transcription->solve(guess);
}

} // namespace CasOC
