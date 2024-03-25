#include "MultiBodySystemOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IFloatOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>
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
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumPrescribeQCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumHandleEventCalls",
            "The total number of calls to handleEvents() regardless of the outcome",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumHandleEventCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumReportEventCalls",
            "The total number of calls to reportEvents() regardless of the outcome",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumReportEventCalls()); }
        });
        rv.emplace_back(MultiBodySystemOutputExtractor{
            "NumRealizeCalls",
            "The total number of calls to realizeTopology(), realizeModel(), or realize(), regardless of whether these routines actually did anything when called",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumRealizeCalls()); }
        });
        return rv;
    }

    std::vector<OutputExtractor> const& GetAllMultiBodySystemOutputExtractors()
    {
        static std::vector<OutputExtractor> const s_Outputs = ConstructMultiBodySystemOutputExtractors();
        return s_Outputs;
    }
}

void osc::MultiBodySystemOutputExtractor::implAccept(IOutputValueExtractorVisitor& visitor) const
{
    visitor(*this);
}

std::size_t osc::MultiBodySystemOutputExtractor::implGetHash() const
{
    return HashOf(m_AuxiliaryDataID, m_Name, m_Description, m_Extractor);
}

bool osc::MultiBodySystemOutputExtractor::implEquals(IOutputExtractor const& other) const
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

void osc::MultiBodySystemOutputExtractor::implExtractFloats(
    OpenSim::Component const&,
    std::span<SimulationReport const> reports,
    std::function<void(float)> const& consumer) const
{
    for (size_t i = 0; i < reports.size(); ++i)
    {
        consumer(reports[i].getAuxiliaryValue(m_AuxiliaryDataID).value_or(quiet_nan_v<float>));
    }
}

int osc::GetNumMultiBodySystemOutputExtractors()
{
    return static_cast<int>(GetAllMultiBodySystemOutputExtractors().size());
}

MultiBodySystemOutputExtractor const& osc::GetMultiBodySystemOutputExtractor(int idx)
{
    return dynamic_cast<MultiBodySystemOutputExtractor const&>(GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

OutputExtractor osc::GetMultiBodySystemOutputExtractorDynamic(int idx)
{
    return GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx));
}
