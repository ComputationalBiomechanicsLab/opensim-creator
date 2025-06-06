#include "IntegratorOutputExtractor.h"

#include <libopensimcreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <liboscar/Maths/Constants.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/HashHelpers.h>
#include <liboscar/Utils/UID.h>
#include <simmath/Integrator.h>

#include <cmath>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

using namespace osc;

namespace
{
    std::vector<OutputExtractor> ConstructIntegratorOutputExtractors()
    {
        std::vector<OutputExtractor> rv;
        rv.emplace_back(IntegratorOutputExtractor{
            "AccuracyInUse",
            "The accuracy which is being used for error control. Usually this is the same value that was specified to setAccuracy()",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getAccuracyInUse()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "PredictedNextStepSize",
            "The step size that will be attempted first on the next call to stepTo() or stepBy().",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getPredictedNextStepSize()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumStepsAttempted",
            "The total number of steps that have been attempted (successfully or unsuccessfully)",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumStepsAttempted()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumStepsTaken",
            "The total number of steps that have been successfully taken",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumStepsTaken()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumRealizations",
            "The total number of state realizations that have been performed",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumRealizations()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumQProjections",
            "The total number of times a state positions Q have been projected",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumQProjections()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumUProjections",
            "The total number of times a state velocities U have been projected",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumUProjections()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumErrorTestFailures",
            "The number of attempted steps that have failed due to the error being unacceptably high",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumErrorTestFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumConvergenceTestFailures",
            "The number of attempted steps that failed due to non-convergence of internal step iterations. This is most common with iterative methods but can occur if for some reason a step can't be completed.",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumConvergenceTestFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumRealizationFailures",
            "The number of attempted steps that have failed due to an error when realizing the state",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumRealizationFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumQProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state positions (Q)",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumQProjectionFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumUProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state velocities (U)",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumUProjectionFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumProjectionFailures",
            "The number of attempted steps that have failed due to an error when projecting the state (either a Q- or U-projection)",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumProjectionFailures()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumConvergentIterations",
            "For iterative methods, the number of internal step iterations in steps that led to convergence (not necessarily successful steps).",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumConvergentIterations()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumDivergentIterations",
            "For iterative methods, the number of internal step iterations in steps that did not lead to convergence.",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumDivergentIterations()); }
        });
        rv.emplace_back(IntegratorOutputExtractor{
            "NumIterations",
            "For iterative methods, this is the total number of internal step iterations taken regardless of whether those iterations led to convergence or to successful steps. This is the sum of the number of convergent and divergent iterations which are available separately.",
            [](const SimTK::Integrator& inter) { return static_cast<float>(inter.getNumIterations()); }
        });
        return rv;
    }

    const std::vector<OutputExtractor>& GetAllIntegratorOutputExtractors()
    {
        static const std::vector<OutputExtractor> s_IntegratorOutputs = ConstructIntegratorOutputExtractors();
        return s_IntegratorOutputs;
    }
}

OutputValueExtractor osc::IntegratorOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[id = m_AuxiliaryDataID](const SimulationReport& report)
    {
        return Variant{report.getAuxiliaryValue(id).value_or(quiet_nan_v<float>)};
    }};
}

std::size_t osc::IntegratorOutputExtractor::implGetHash() const
{
    return hash_of(m_AuxiliaryDataID, m_Name, m_Description, m_Extractor);
}

bool osc::IntegratorOutputExtractor::implEquals(const IOutputExtractor& other) const
{
    if (this == &other)
    {
        return true;
    }

    const auto* const otherT = dynamic_cast<const IntegratorOutputExtractor*>(&other);
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

const IntegratorOutputExtractor& osc::GetIntegratorOutputExtractor(int idx)
{
    return dynamic_cast<const IntegratorOutputExtractor&>(GetAllIntegratorOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

OutputExtractor osc::GetIntegratorOutputExtractorDynamic(int idx)
{
    return GetAllIntegratorOutputExtractors().at(static_cast<size_t>(idx));
}
