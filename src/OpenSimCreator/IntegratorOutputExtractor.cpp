#include "IntegratorOutputExtractor.hpp"

#include "OpenSimCreator/Simulation/SimulationReport.hpp"
#include "OpenSimCreator/VirtualOutputExtractor.hpp"

#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Algorithms.hpp>
#include <oscar/Utils/UID.hpp>

#include <nonstd/span.hpp>
#include <simmath/Integrator.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::vector<osc::OutputExtractor> ConstructIntegratorOutputExtractors()
    {
        std::vector<osc::OutputExtractor> rv;
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "AccuracyInUse",
            "The accuracy which is being used for error control. Usually this is the same value that was specified to setAccuracy()",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getAccuracyInUse()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "PredictedNextStepSize",
            "The step size that will be attempted first on the next call to stepTo() or stepBy().",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getPredictedNextStepSize()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumStepsAttempted",
            "The total number of steps that have been attempted (successfully or unsuccessfully)",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumStepsAttempted()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumStepsTaken",
            "The total number of steps that have been successfully taken",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumStepsTaken()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumRealizations",
            "The total number of state realizations that have been performed",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumRealizations()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumQProjections",
            "The total number of times a state positions Q have been projected",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumQProjections()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumUProjections",
            "The total number of times a state velocities U have been projected",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumUProjections()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumErrorTestFailures",
            "The number of attempted steps that have failed due to the error being unacceptably high",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumErrorTestFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumConvergenceTestFailures",
            "The number of attempted steps that failed due to non-convergence of internal step iterations. This is most common with iterative methods but can occur if for some reason a step can't be completed.",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumConvergenceTestFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumRealizationFailures",
            "The number of attempted steps that have failed due to an error when realizing the state",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumRealizationFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumQProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state positions (Q)",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumQProjectionFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumUProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state velocities (U)",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumUProjectionFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state (either a Q- or U-projection)",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumProjectionFailures()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumConvergentIterations",
            "For iterative methods, the number of internal step iterations in steps that led to convergence (not necessarily successful steps).",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumConvergentIterations()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumDivergentIterations",
            "For iterative methods, the number of internal step iterations in steps that did not lead to convergence.",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumDivergentIterations()); }
        });
        rv.emplace_back(osc::IntegratorOutputExtractor{
            "NumIterations",
            "For iterative methods, this is the total number of internal step iterations taken regardless of whether those iterations led to convergence or to successful steps. This is the sum of the number of convergent and divergent iterations which are available separately.",
            [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumIterations()); }
        });
        return rv;
    }

    std::vector<osc::OutputExtractor> const& GetAllIntegratorOutputExtractors()
    {
        static std::vector<osc::OutputExtractor> const s_IntegratorOutputs = ConstructIntegratorOutputExtractors();
        return s_IntegratorOutputs;
    }
}

// public API

osc::IntegratorOutputExtractor::IntegratorOutputExtractor(std::string_view name,
                                                          std::string_view description,
                                                          ExtractorFn extractor) :
    m_AuxiliaryDataID{},
    m_Name{std::move(name)},
    m_Description{std::move(description)},
    m_Extractor{std::move(extractor)}
{
}

std::string const& osc::IntegratorOutputExtractor::getName() const
{
    return m_Name;
}

std::string const& osc::IntegratorOutputExtractor::getDescription() const
{
    return m_Description;
}

osc::OutputType osc::IntegratorOutputExtractor::getOutputType() const
{
    return OutputType::Float;
}

float osc::IntegratorOutputExtractor::getValueFloat(OpenSim::Component const&, SimulationReport const& report) const
{
    return report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
}

void osc::IntegratorOutputExtractor::getValuesFloat(OpenSim::Component const&,
                                           nonstd::span<SimulationReport const> reports,
                                           nonstd::span<float> out) const
{
    OSC_ASSERT_ALWAYS(reports.size() == out.size());
    for (size_t i = 0; i < reports.size(); ++i)
    {
        out[i] = reports[i].getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
    }
}

std::string osc::IntegratorOutputExtractor::getValueString(OpenSim::Component const&, SimulationReport const& report) const
{
    return std::to_string(report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN));
}

osc::UID osc::IntegratorOutputExtractor::getAuxiliaryDataID() const
{
    return m_AuxiliaryDataID;
}

osc::IntegratorOutputExtractor::ExtractorFn osc::IntegratorOutputExtractor::getExtractorFunction() const
{
    return m_Extractor;
}

std::size_t osc::IntegratorOutputExtractor::getHash() const
{
    return osc::HashOf(m_AuxiliaryDataID, m_Name, m_Description, m_Extractor);
}

bool osc::IntegratorOutputExtractor::equals(VirtualOutputExtractor const& other) const
{
    if (this == &other)
    {
        return true;
    }

    IntegratorOutputExtractor const* const otherT = dynamic_cast<IntegratorOutputExtractor const*>(&other);
    if (!otherT)
    {
        return false;
    }

    return
        m_AuxiliaryDataID == otherT->m_AuxiliaryDataID &&
        m_Name == otherT->m_Name &&
        m_Description == otherT->m_Description &&
        m_Extractor == otherT->m_Extractor;
}

int osc::GetNumIntegratorOutputExtractors()
{
    return static_cast<int>(GetAllIntegratorOutputExtractors().size());
}

osc::IntegratorOutputExtractor const& osc::GetIntegratorOutputExtractor(int idx)
{
    return static_cast<osc::IntegratorOutputExtractor const&>(GetAllIntegratorOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

osc::OutputExtractor osc::GetIntegratorOutputExtractorDynamic(int idx)
{
    return GetAllIntegratorOutputExtractors().at(static_cast<size_t>(idx));
}
