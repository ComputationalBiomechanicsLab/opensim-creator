#include "MultiBodySystemOutputExtractor.hpp"

#include <OpenSimCreator/Documents/Simulation/SimulationReport.hpp>

#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <simbody/internal/MultibodySystem.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace
{
    std::vector<osc::OutputExtractor> ConstructMultiBodySystemOutputExtractors()
    {
        std::vector<osc::OutputExtractor> rv;

        // SimTK::System (base class)
        rv.emplace_back(osc::MultiBodySystemOutputExtractor{
            "NumPrescribeQcalls",
            "Get the number of prescribe Q calls made against the system",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumPrescribeQCalls()); }
        });
        rv.emplace_back(osc::MultiBodySystemOutputExtractor{
            "NumHandleEventCalls",
            "The total number of calls to handleEvents() regardless of the outcome",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumHandleEventCalls()); }
        });
        rv.emplace_back(osc::MultiBodySystemOutputExtractor{
            "NumReportEventCalls",
            "The total number of calls to reportEvents() regardless of the outcome",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumReportEventCalls()); }
        });
        rv.emplace_back(osc::MultiBodySystemOutputExtractor{
            "NumRealizeCalls",
            "The total number of calls to realizeTopology(), realizeModel(), or realize(), regardless of whether these routines actually did anything when called",
            [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumRealizeCalls()); }
        });
        return rv;
    }

    std::vector<osc::OutputExtractor> const& GetAllMultiBodySystemOutputExtractors()
    {
        static std::vector<osc::OutputExtractor> const s_Outputs = ConstructMultiBodySystemOutputExtractors();
        return s_Outputs;
    }
}

osc::MultiBodySystemOutputExtractor::MultiBodySystemOutputExtractor(std::string_view name,
                                                                    std::string_view description,
                                                                    ExtractorFn extractor) :
    m_Name{name},
    m_Description{description},
    m_Extractor{extractor}
{
}

osc::CStringView osc::MultiBodySystemOutputExtractor::getName() const
{
    return m_Name;
}

osc::CStringView osc::MultiBodySystemOutputExtractor::getDescription() const
{
    return m_Description;
}

osc::OutputType osc::MultiBodySystemOutputExtractor::getOutputType() const
{
    return OutputType::Float;
}

float osc::MultiBodySystemOutputExtractor::getValueFloat(OpenSim::Component const&,
                                                         SimulationReport const& report) const
{
    return report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
}

void osc::MultiBodySystemOutputExtractor::getValuesFloat(OpenSim::Component const&,
                                                         std::span<SimulationReport const> reports,
                                                         std::span<float> out) const
{
    OSC_ASSERT_ALWAYS(reports.size() == out.size());
    for (size_t i = 0; i < reports.size(); ++i)
    {
        out[i] = reports[i].getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
    }
}

std::string osc::MultiBodySystemOutputExtractor::getValueString(OpenSim::Component const& c,
                                                                SimulationReport const& report) const
{
    return std::to_string(getValueFloat(c, report));
}

osc::UID osc::MultiBodySystemOutputExtractor::getAuxiliaryDataID() const
{
    return m_AuxiliaryDataID;
}

osc::MultiBodySystemOutputExtractor::ExtractorFn osc::MultiBodySystemOutputExtractor::getExtractorFunction() const
{
    return m_Extractor;
}

std::size_t osc::MultiBodySystemOutputExtractor::getHash() const
{
    return osc::HashOf(m_AuxiliaryDataID, m_Name, m_Description, m_Extractor);
}

bool osc::MultiBodySystemOutputExtractor::equals(IOutputExtractor const& other) const
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

osc::MultiBodySystemOutputExtractor const& osc::GetMultiBodySystemOutputExtractor(int idx)
{
    return dynamic_cast<MultiBodySystemOutputExtractor const&>(GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

osc::OutputExtractor osc::GetMultiBodySystemOutputExtractorDynamic(int idx)
{
    return GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx));
}
