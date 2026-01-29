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
    Impl(const OpenSim::Force& force, int recordIndex) :
        m_ForceAbsPath{force.getAbsolutePath()},
        m_RecordIndex{recordIndex}
    {
        OSC_ASSERT(recordIndex >= 0);
        const OpenSim::Array<std::string> labels = force.getRecordLabels();
        OSC_ASSERT(0 <= recordIndex && recordIndex < labels.size() && "the provided OpenSim::Force record index is out of bounds");
        m_Label = labels[recordIndex];
    }

    friend bool operator==(const Impl&, const Impl&) = default;

    osc::CStringView getName() const { return m_Label; }
    osc::CStringView getDescription() const { return {}; }
    OutputExtractorDataType getOutputType() const { return OutputExtractorDataType::Float; }
    OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& root) const
    {
        if (const auto* force = FindComponent<OpenSim::Force>(root, m_ForceAbsPath)) {
            return OutputValueExtractor{[force, index = m_RecordIndex](const StateViewWithMetadata& state)
            {
                const OpenSim::Array<double> values = force->getRecordValues(state.getState());
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
    size_t getHash() const { return osc::hash_of(m_ForceAbsPath, m_RecordIndex, m_Label); }
    bool equals(const OutputExtractor& other) const
    {
        if (const auto* downcasted = dynamic_cast<const ForceRecordOutputExtractor*>(&other)) {
            return downcasted->m_Impl.get() == this or *downcasted->m_Impl == *this;
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
    OpenSim::ComponentPath m_ForceAbsPath;
    int m_RecordIndex = 0;
    std::string m_Label;
};

opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(
    const OpenSim::Force& force,
    int recordIndex) :
    m_Impl{std::make_unique<Impl>(force, recordIndex)}
{}
opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(const ForceRecordOutputExtractor&) = default;
opyn::ForceRecordOutputExtractor::ForceRecordOutputExtractor(ForceRecordOutputExtractor&&) noexcept = default;
ForceRecordOutputExtractor& opyn::ForceRecordOutputExtractor::operator=(const ForceRecordOutputExtractor&) = default;
ForceRecordOutputExtractor& opyn::ForceRecordOutputExtractor::operator=(ForceRecordOutputExtractor&&) noexcept = default;
opyn::ForceRecordOutputExtractor::~ForceRecordOutputExtractor() noexcept = default;
osc::CStringView opyn::ForceRecordOutputExtractor::implGetName() const { return m_Impl->getName(); }
osc::CStringView opyn::ForceRecordOutputExtractor::implGetDescription() const { return m_Impl->getDescription(); }
OutputExtractorDataType opyn::ForceRecordOutputExtractor::implGetOutputType() const { return m_Impl->getOutputType(); }
OutputValueExtractor opyn::ForceRecordOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& component) const { return m_Impl->getOutputValueExtractor(component); }
size_t opyn::ForceRecordOutputExtractor::implGetHash() const { return m_Impl->getHash(); }
bool opyn::ForceRecordOutputExtractor::implEquals(const OutputExtractor& other) const { return m_Impl->equals(other); }
