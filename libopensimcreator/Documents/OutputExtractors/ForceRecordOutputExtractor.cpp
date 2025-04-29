#include "ForceRecordOutputExtractor.h"

#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/HashHelpers.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>


#include <liboscar/Platform/Log.h>

using namespace osc;

class osc::ForceRecordOutputExtractor::Impl final {
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

    CStringView getName() const { return m_Label; }
    CStringView getDescription() const { return {}; }
    OutputExtractorDataType getOutputType() const { return OutputExtractorDataType::Float; }
    OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& root) const
    {
        if (const auto* force = FindComponent<OpenSim::Force>(root, m_ForceAbsPath)) {
            return OutputValueExtractor{[force, index = m_RecordIndex](const SimulationReport& report)
            {
                const OpenSim::Array<double> values = force->getRecordValues(report.getState());
                if (0 <= index and index < values.size()) {
                    return Variant{static_cast<float>(values[index])};
                }
                else {
                    return Variant{quiet_nan_v<float>};  // Index out of bounds
                }
            }};
        }
        else {
            return OutputValueExtractor::constant(quiet_nan_v<float>);  // Invalid component
        }
    }
    size_t getHash() const { return hash_of(m_ForceAbsPath, m_RecordIndex, m_Label); }
    bool equals(const IOutputExtractor& other) const
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

osc::ForceRecordOutputExtractor::ForceRecordOutputExtractor(
    const OpenSim::Force& force,
    int recordIndex) :
    m_Impl{std::make_unique<Impl>(force, recordIndex)}
{}
osc::ForceRecordOutputExtractor::ForceRecordOutputExtractor(const ForceRecordOutputExtractor&) = default;
osc::ForceRecordOutputExtractor::ForceRecordOutputExtractor(ForceRecordOutputExtractor&&) noexcept = default;
ForceRecordOutputExtractor& osc::ForceRecordOutputExtractor::operator=(const ForceRecordOutputExtractor&) = default;
ForceRecordOutputExtractor& osc::ForceRecordOutputExtractor::operator=(ForceRecordOutputExtractor&&) noexcept = default;
osc::ForceRecordOutputExtractor::~ForceRecordOutputExtractor() noexcept = default;
CStringView osc::ForceRecordOutputExtractor::implGetName() const { return m_Impl->getName(); }
CStringView osc::ForceRecordOutputExtractor::implGetDescription() const { return m_Impl->getDescription(); }
OutputExtractorDataType osc::ForceRecordOutputExtractor::implGetOutputType() const { return m_Impl->getOutputType(); }
OutputValueExtractor osc::ForceRecordOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& component) const { return m_Impl->getOutputValueExtractor(component); }
size_t osc::ForceRecordOutputExtractor::implGetHash() const { return m_Impl->getHash(); }
bool osc::ForceRecordOutputExtractor::implEquals(const IOutputExtractor& other) const { return m_Impl->equals(other); }
