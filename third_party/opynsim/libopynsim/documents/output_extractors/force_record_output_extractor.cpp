#include "force_record_output_extractor.h"

#include <libopynsim/documents/state_view_with_metadata.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/hash_helpers.h>

using namespace opyn;

class opyn::ForceRecordOutputExtractor::Impl final {
public:
    Impl(const OpenSim::Force& force, int record_index) :
        force_abs_path_{force.getAbsolutePath()},
        record_index_{record_index}
    {
        OSC_ASSERT(record_index >= 0);
        const OpenSim::Array<std::string> labels = force.getRecordLabels();
        OSC_ASSERT(0 <= record_index && record_index < labels.size() && "the provided OpenSim::Force record index is out of bounds");
        label_ = labels[record_index];
    }

    friend bool operator==(const Impl&, const Impl&) = default;

    osc::CStringView name() const { return label_; }
    osc::CStringView description() const { return {}; }
    OutputExtractorDataType output_type() const { return OutputExtractorDataType::Float; }
    OutputValueExtractor output_value_extractor(const OpenSim::Component& root) const
    {
        if (const auto* force = FindComponent<OpenSim::Force>(root, force_abs_path_)) {
            return OutputValueExtractor{[force, index = record_index_](const StateViewWithMetadata& state)
            {
                const OpenSim::Array<double> values = force->getRecordValues(state.state());
                if (0 <= index and index < values.size()) {
                    return osc::Variant{static_cast<float>(values[index])};
                }
                else {
                    return osc::Variant{osc::quiet_nan_v<float>};  // Index out of bounds
                }
            }};
        }
        else {
            return OutputValueExtractor::constant(osc::quiet_nan_v<float>);  // Invalid component
        }
    }
    size_t hash() const { return osc::hash_of(force_abs_path_, record_index_, label_); }
    bool equals(const OutputExtractor& other) const
    {
        if (const auto* const downcasted = dynamic_cast<const ForceRecordOutputExtractor*>(&other)) {
            return downcasted->impl_.get() == this or *downcasted->impl_ == *this;
        }
        else {
            return false;
        }
    }
    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }
private:
    OpenSim::ComponentPath force_abs_path_;
    int record_index_ = 0;
    std::string label_;
};

opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(
    const OpenSim::Force& force,
    int record_index) :
    impl_{std::make_unique<Impl>(force, record_index)}
{}
opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(const ForceRecordOutputExtractor&) = default;
opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(ForceRecordOutputExtractor&&) noexcept = default;
ForceRecordOutputExtractor& opyn::ForceRecordOutputExtractor::operator=(const ForceRecordOutputExtractor&) = default;
ForceRecordOutputExtractor& opyn::ForceRecordOutputExtractor::operator=(ForceRecordOutputExtractor&&) noexcept = default;
opyn::ForceRecordOutputExtractor::~ForceRecordOutputExtractor() noexcept = default;
osc::CStringView opyn::ForceRecordOutputExtractor::impl_name() const { return impl_->name(); }
osc::CStringView opyn::ForceRecordOutputExtractor::impl_description() const { return impl_->description(); }
OutputExtractorDataType opyn::ForceRecordOutputExtractor::impl_output_type() const { return impl_->output_type(); }
OutputValueExtractor opyn::ForceRecordOutputExtractor::impl_output_value_extractor(const OpenSim::Component& component) const { return impl_->output_value_extractor(component); }
size_t opyn::ForceRecordOutputExtractor::impl_hash() const { return impl_->hash(); }
bool opyn::ForceRecordOutputExtractor::impl_equals(const OutputExtractor& other) const { return impl_->equals(other); }
