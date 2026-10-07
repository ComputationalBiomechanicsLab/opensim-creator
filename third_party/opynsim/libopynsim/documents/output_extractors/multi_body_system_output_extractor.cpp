#include "multi_body_system_output_extractor.h"

#include <libopynsim/documents/state_view_with_metadata.h>

#include <liboscar/maths/constants.h>
#include <liboscar/utilities/hash_helpers.h>
#include <simbody/internal/MultibodySystem.h>

#include <optional>
#include <vector>

using namespace opyn;

namespace
{
    std::vector<SharedOutputExtractor> construct_multi_body_system_output_extractors()
    {
        std::vector<SharedOutputExtractor> rv;

        // SimTK::System (base class)
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumPrescribeQcalls",
            "Get the number of prescribe Q calls made against the system",
            [](const SimTK::MultibodySystem& mbs) { return static_cast<float>(mbs.getNumPrescribeQCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumHandleEventCalls",
            "The total number of calls to handleEvents() regardless of the outcome",
            [](const SimTK::MultibodySystem& mbs) { return static_cast<float>(mbs.getNumHandleEventCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumReportEventCalls",
            "The total number of calls to reportEvents() regardless of the outcome",
            [](const SimTK::MultibodySystem& mbs) { return static_cast<float>(mbs.getNumReportEventCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumRealizeCalls",
            "The total number of calls to realizeTopology(), realizeModel(), or realize(), regardless of whether these routines actually did anything when called",
            [](const SimTK::MultibodySystem& mbs) { return static_cast<float>(mbs.getNumRealizeCalls()); }
        });
        return rv;
    }

    const std::vector<SharedOutputExtractor>& get_all_multi_body_system_output_extractors()
    {
        static const std::vector<SharedOutputExtractor> s_outputs = construct_multi_body_system_output_extractors();
        return s_outputs;
    }
}

OutputValueExtractor opyn::MultiBodySystemOutputExtractor::impl_output_value_extractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[id = auxiliary_data_id_](const StateViewWithMetadata& state)
    {
        return osc::Variant{state.get_auxiliary_value(id).value_or(osc::quiet_nan_v<float>)};
    }};
}

size_t opyn::MultiBodySystemOutputExtractor::impl_hash() const
{
    return hash_of(auxiliary_data_id_, name_, description_, extractor_);
}

bool opyn::MultiBodySystemOutputExtractor::impl_equals(const OutputExtractor& other) const
{
    if (&other == this) {
        return true;
    }

    const auto* const other_t = dynamic_cast<const MultiBodySystemOutputExtractor*>(&other);
    if (not other_t) {
        return false;
    }

    return
        auxiliary_data_id_ == other_t->auxiliary_data_id_ &&
        name_ == other_t->name_ &&
        description_ == other_t->description_ &&
        extractor_ == other_t->extractor_;
}

int opyn::num_multi_body_system_output_extractors()
{
    return static_cast<int>(get_all_multi_body_system_output_extractors().size());
}

const MultiBodySystemOutputExtractor& opyn::multi_body_system_output_extractor(int idx)
{
    return dynamic_cast<const MultiBodySystemOutputExtractor&>(get_all_multi_body_system_output_extractors().at(static_cast<size_t>(idx)).inner());
}

SharedOutputExtractor opyn::multi_body_system_output_extractor_dynamic(int idx)
{
    return get_all_multi_body_system_output_extractors().at(static_cast<size_t>(idx));
}
