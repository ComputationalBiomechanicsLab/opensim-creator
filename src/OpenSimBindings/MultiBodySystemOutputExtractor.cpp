#include "MultiBodySystemOutputExtractor.hpp"

#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"

#include <nonstd/span.hpp>
#include <simbody/internal/MultibodySystem.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

static std::vector<osc::OutputExtractor> ConstructMultiBodySystemOutputExtractors()
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

static std::vector<osc::OutputExtractor> const& GetAllMultiBodySystemOutputExtractors()
{
    static std::vector<osc::OutputExtractor> const g_Outputs = ConstructMultiBodySystemOutputExtractors();
    return g_Outputs;
}

osc::MultiBodySystemOutputExtractor::MultiBodySystemOutputExtractor(std::string_view name,
                                                                    std::string_view description,
                                                                    ExtractorFn extractor) :
    m_Name{std::move(name)},
    m_Description{std::move(description)},
    m_Extractor{std::move(extractor)}
{
}

std::string const& osc::MultiBodySystemOutputExtractor::getName() const
{
    return m_Name;
}

std::string const& osc::MultiBodySystemOutputExtractor::getDescription() const
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
                                                         nonstd::span<SimulationReport const> reports,
                                                         nonstd::span<float> out) const
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

bool osc::MultiBodySystemOutputExtractor::equals(VirtualOutputExtractor const& other) const
{
    if (&other == this)
    {
        return true;
    }

    auto* otherT = dynamic_cast<MultiBodySystemOutputExtractor const*>(&other);

    if (!otherT)
    {
        return false;
    }

    bool result =
        m_AuxiliaryDataID == otherT->m_AuxiliaryDataID &&
        m_Name == otherT->m_Name &&
        m_Description == otherT->m_Description &&
        m_Extractor == otherT->m_Extractor;

    return result;
}

int osc::GetNumMultiBodySystemOutputExtractors()
{
    return static_cast<int>(GetAllMultiBodySystemOutputExtractors().size());
}

osc::MultiBodySystemOutputExtractor const& osc::GetMultiBodySystemOutputExtractor(int idx)
{
    return static_cast<MultiBodySystemOutputExtractor const&>(GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx)).getInner());
}

osc::OutputExtractor osc::GetMultiBodySystemOutputExtractorDynamic(int idx)
{
    return GetAllMultiBodySystemOutputExtractors().at(static_cast<size_t>(idx));
}
