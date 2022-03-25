#include "MultiBodySystemOutput.hpp"

#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Utils/Assertions.hpp"

#include <simbody/internal/MultibodySystem.h>

static std::vector<osc::Output> ConstructMultiBodySystemOutputs()
{
    std::vector<osc::Output> rv;

    // SimTK::System (base class)
    rv.emplace_back(osc::MultiBodySystemOutput{
        "NumPrescribeQcalls",
        "Get the number of prescribe Q calls made against the system",
        [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumPrescribeQCalls()); }
    });
    rv.emplace_back(osc::MultiBodySystemOutput{
        "NumHandleEventCalls",
        "The total number of calls to handleEvents() regardless of the outcome",
        [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumHandleEventCalls()); }
    });
    rv.emplace_back(osc::MultiBodySystemOutput{
        "NumReportEventCalls",
        "The total number of calls to reportEvents() regardless of the outcome",
        [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumReportEventCalls()); }
    });
    rv.emplace_back(osc::MultiBodySystemOutput{
        "NumRealizeCalls",
        "The total number of calls to realizeTopology(), realizeModel(), or realize(), regardless of whether these routines actually did anything when called",
        [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumRealizeCalls()); }
    });
    return rv;
}

static std::vector<osc::Output> const& GetAllMultiBodySystemOutputs()
{
    static std::vector<osc::Output> const g_Outputs = ConstructMultiBodySystemOutputs();
    return g_Outputs;
}

osc::MultiBodySystemOutput::MultiBodySystemOutput(std::string_view name,
                                                  std::string_view description,
                                                  ExtractorFn extractor) :
    m_Name{std::move(name)},
    m_Description{std::move(description)},
    m_Extractor{std::move(extractor)}
{
}

std::string const& osc::MultiBodySystemOutput::getName() const
{
    return m_Name;
}

std::string const& osc::MultiBodySystemOutput::getDescription() const
{
    return m_Description;
}

osc::OutputType osc::MultiBodySystemOutput::getOutputType() const
{
    return OutputType::Float;
}

float osc::MultiBodySystemOutput::getValueFloat(OpenSim::Component const&, SimulationReport const& report) const
{
    return report.getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
}

void osc::MultiBodySystemOutput::getValuesFloat(OpenSim::Component const&,
                                                nonstd::span<SimulationReport const> reports,
                                                nonstd::span<float> out) const
{
    OSC_ASSERT_ALWAYS(reports.size() == out.size());
    for (size_t i = 0; i < reports.size(); ++i)
    {
        out[i] = reports[i].getAuxiliaryValue(m_AuxiliaryDataID).value_or(NAN);
    }
}

std::string osc::MultiBodySystemOutput::getValueString(OpenSim::Component const& c, SimulationReport const& report) const
{
    return std::to_string(getValueFloat(c, report));
}

osc::UID osc::MultiBodySystemOutput::getAuxiliaryDataID() const
{
    return m_AuxiliaryDataID;
}

osc::MultiBodySystemOutput::ExtractorFn osc::MultiBodySystemOutput::getExtractorFunction() const
{
    return m_Extractor;
}

int osc::GetNumMultiBodySystemOutputs()
{
    return static_cast<int>(GetAllMultiBodySystemOutputs().size());
}

osc::MultiBodySystemOutput const& osc::GetMultiBodySystemOutput(int idx)
{
    return static_cast<MultiBodySystemOutput const&>(GetAllMultiBodySystemOutputs().at(static_cast<size_t>(idx)).getInner());
}

osc::Output osc::GetMultiBodySystemOutputDynamic(int idx)
{
    return GetAllMultiBodySystemOutputs().at(static_cast<size_t>(idx));
}
