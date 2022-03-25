#include "IntegratorOutput.hpp"

#include "src/OpenSimBindings/VirtualOutput.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/UID.hpp"

#include <simmath/Integrator.h>

#include <string_view>
#include <optional>
#include <utility>

static std::vector<osc::Output> ConstructIntegratorOutputs()
{
    std::vector<osc::Output> rv;
    rv.emplace_back(osc::IntegratorOutput{
        "AccuracyInUse",
        "The accuracy which is being used for error control. Usually this is the same value that was specified to setAccuracy()",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getAccuracyInUse()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "PredictedNextStepSize",
        "The step size that will be attempted first on the next call to stepTo() or stepBy().",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getPredictedNextStepSize()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumStepsAttempted",
        "The total number of steps that have been attempted (successfully or unsuccessfully)",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumStepsAttempted()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumStepsTaken",
        "The total number of steps that have been successfully taken",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumStepsTaken()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumRealizations",
        "The total number of state realizations that have been performed",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumRealizations()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumQProjections",
        "The total number of times a state positions Q have been projected",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumQProjections()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumUProjections",
        "The total number of times a state velocities U have been projected",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumUProjections()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumErrorTestFailures",
        "The number of attempted steps that have failed due to the error being unacceptably high",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumErrorTestFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumConvergenceTestFailures",
        "The number of attempted steps that failed due to non-convergence of internal step iterations. This is most common with iterative methods but can occur if for some reason a step can't be completed.",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumConvergenceTestFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumRealizationFailures",
        "The number of attempted steps that have failed due to an error when realizing the state",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumRealizationFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumQProjectionFailures",
        "The number of attempted steps that have failed due to an error when projecting the state positions (Q)",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumQProjectionFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumUProjectionFailures",
        "The number of attempted steps that have failed due to an error when projecting the state velocities (U)",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumUProjectionFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumProjectionFailures",
        "The number of attempted steps that have failed due to an error when projecting the state (either a Q- or U-projection)",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumProjectionFailures()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumConvergentIterations",
        "For iterative methods, the number of internal step iterations in steps that led to convergence (not necessarily successful steps).",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumConvergentIterations()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumDivergentIterations",
        "For iterative methods, the number of internal step iterations in steps that did not lead to convergence.",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumDivergentIterations()); }
    });
    rv.emplace_back(osc::IntegratorOutput{
        "NumIterations",
        "For iterative methods, this is the total number of internal step iterations taken regardless of whether those iterations led to convergence or to successful steps. This is the sum of the number of convergent and divergent iterations which are available separately.",
        [](SimTK::Integrator const& inter) { return static_cast<float>(inter.getNumIterations()); }
    });
    return rv;
}

static std::vector<osc::Output> const& GetAllIntegratorOutputs()
{
    static std::vector<osc::Output> const g_IntegratorOutputs = ConstructIntegratorOutputs();
    return g_IntegratorOutputs;
}

// public API

osc::IntegratorOutput::IntegratorOutput(std::string_view name,
                                        std::string_view description,
                                        ExtractorFn extractor) :
    m_AuxiliaryDataID{},
    m_Name{std::move(name)},
    m_Description{std::move(description)},
    m_Extractor{std::move(extractor)}
{
}

std::string const& osc::IntegratorOutput::getName() const
{
    return m_Name;
}

std::string const& osc::IntegratorOutput::getDescription() const
{
    return m_Description;
}

osc::OutputType osc::IntegratorOutput::getOutputType() const
{
    return OutputType::Float;
}

float osc::IntegratorOutput::getValueFloat(OpenSim::Component const&, SimulationReport const& report) const
{
    return report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
}

void osc::IntegratorOutput::getValuesFloat(OpenSim::Component const&,
                                           nonstd::span<SimulationReport const> reports,
                                           nonstd::span<float> out) const
{
    OSC_ASSERT_ALWAYS(reports.size() == out.size());
    for (size_t i = 0; i < reports.size(); ++i)
    {
        out[i] = reports[i].getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
    }
}

std::string osc::IntegratorOutput::getValueString(OpenSim::Component const&, SimulationReport const& report) const
{
    return std::to_string(report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN));
}

osc::UID osc::IntegratorOutput::getAuxiliaryDataID() const
{
    return m_AuxiliaryDataID;
}

osc::IntegratorOutput::ExtractorFn osc::IntegratorOutput::getExtractorFunction() const
{
    return m_Extractor;
}

int osc::GetNumIntegratorOutputs()
{
    return static_cast<int>(GetAllIntegratorOutputs().size());
}

osc::IntegratorOutput const& osc::GetIntegratorOutput(int idx)
{
    return static_cast<osc::IntegratorOutput const&>(GetAllIntegratorOutputs().at(static_cast<size_t>(idx)).getInner());
}

osc::Output osc::GetIntegratorOutputDynamic(int idx)
{
    return GetAllIntegratorOutputs().at(static_cast<size_t>(idx));
}
