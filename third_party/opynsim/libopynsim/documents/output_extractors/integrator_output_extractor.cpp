#include "integrator_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>
#include <libopynsim/documents/state_view_with_metadata.h>

#include <liboscar/maths/constants.h>
#include <liboscar/utilities/hash_helpers.h>
#include <liboscar/utilities/uid.h>
#include <simmath/Integrator.h>

#include <cstddef>
#include <optional>
#include <vector>

using namespace opyn;

namespace
{
    std::vector<SharedOutputExtractor> construct_integrator_output_extractors()
    {
        std::vector<SharedOutputExtractor> rv;
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

    const std::vector<SharedOutputExtractor>& get_all_integrator_output_extractors()
    {
        static const std::vector<SharedOutputExtractor> s_integrator_outputs = construct_integrator_output_extractors();
        return s_integrator_outputs;
    }
}

OutputValueExtractor opyn::IntegratorOutputExtractor::impl_output_value_extractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[id = auxiliary_data_id_](const StateViewWithMetadata& state)
    {
        return osc::Variant{state.getAuxiliaryValue(id).value_or(osc::quiet_nan_v<float>)};
    }};
}

size_t opyn::IntegratorOutputExtractor::impl_hash() const
{
    return osc::hash_of(auxiliary_data_id_, name_, description_, extractor_);
}

bool opyn::IntegratorOutputExtractor::impl_equals(const OutputExtractor& other) const
{
    if (this == &other) {
        return true;
    }

    const auto* const other_t = dynamic_cast<const IntegratorOutputExtractor*>(&other);
    if (not other_t) {
        return false;
    }

    return
        auxiliary_data_id_ == other_t->auxiliary_data_id_ &&
        name_ == other_t->name_ &&
        description_ == other_t->description_ &&
        extractor_ == other_t->extractor_;
}

int opyn::num_integrator_output_extractors()
{
    return static_cast<int>(get_all_integrator_output_extractors().size());
}

const IntegratorOutputExtractor& opyn::integrator_output_extractor(int idx)
{
    return dynamic_cast<const IntegratorOutputExtractor&>(get_all_integrator_output_extractors().at(static_cast<size_t>(idx)).inner());
}

SharedOutputExtractor opyn::integrator_output_extractor_dynamic(int idx)
{
    return get_all_integrator_output_extractors().at(static_cast<size_t>(idx));
}
