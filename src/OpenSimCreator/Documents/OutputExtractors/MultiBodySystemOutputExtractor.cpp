#include "MultiBodySystemOutputExtractor.h"

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/HashHelpers.h>
#include <simbody/internal/MultibodySystem.h>

#include <cmath>
#include <optional>
#include <span>
#include <vector>

using namespace osc;

namespace
{
    std::vector<OutputExtractor> ConstructMultiBodySystemOutputExtractors()
    {
        std::vector<OutputExtractor> rv;

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

    std::vector<OutputExtractor> const& GetAllMultiBodySystemOutputExtractors()
    {
        static std::vector<OutputExtractor> const s_Outputs = ConstructMultiBodySystemOutputExtractors();
        return s_Outputs;
    }
}

OutputValueExtractor osc::MultiBodySystemOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[id = m_AuxiliaryDataID](const SimulationReport& report)
    {
        return report.getAuxiliaryValue(id).value_or(quiet_nan_v<float>);
    }};
}

std::size_t osc::MultiBodySystemOutputExtractor::implGetHash() const
{
    return hash_of(m_AuxiliaryDataID, m_Name, m_Description, m_Extractor);
}

bool osc::MultiBodySystemOutputExtractor::implEquals(const IOutputExtractor& other) const
{
    if (&other == this)
    {
        return true;
    }

    auto const* const otherT = dynamic_cast<MultiBodySystemOutputExtractor const*>(&other);
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

int osc::GetNumMultiBodySystemOutputExtractors()
{
    return static_cast<int>(GetAllMultiBodySystemOutputExtractors().size());
}

const MultiBodySystemOutputExtractor& osc::GetMultiBodySystemOutputExtractor(int idx)
{
    return dynamic_cast<const MultiBodySystemOutputExtractor&>(GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

OutputExtractor osc::GetMultiBodySystemOutputExtractorDynamic(int idx)
{
    return GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx));
}
