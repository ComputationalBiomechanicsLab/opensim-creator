#include "MultiBodySystemOutput.hpp"

#include "src/OpenSimBindings/SimulationReport.hpp"

#include <simbody/internal/MultibodySystem.h>

static std::vector<osc::Output> ConstructMultiBodySystemOutputs()
{
    std::vector<osc::Output> rv;
    rv.emplace_back(osc::MultiBodySystemOutput{
        "NumPrescribeQcalls",
        "Get the number of prescribe Q calls made against the system",
        [](SimTK::MultibodySystem const& mbs) { return static_cast<float>(mbs.getNumPrescribeQCalls()); }
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

osc::UID osc::MultiBodySystemOutput::getID() const
{
    return m_ID;
}

osc::OutputType osc::MultiBodySystemOutput::getOutputType() const
{
    return OutputType::System;
}

std::string const& osc::MultiBodySystemOutput::getName() const
{
    return m_Name;
}

std::string const& osc::MultiBodySystemOutput::getDescription() const
{
    return m_Description;
}

bool osc::MultiBodySystemOutput::producesNumericValues() const
{
    return true;
}

std::optional<float> osc::MultiBodySystemOutput::getNumericValue(OpenSim::Component const&, SimulationReport const& report) const
{
    return report.getAuxiliaryValue(m_ID);
}

std::optional<std::string> osc::MultiBodySystemOutput::getStringValue(OpenSim::Component const& c, SimulationReport const& report) const
{
    auto maybeValue = getNumericValue(c, report);
    if (maybeValue)
    {
        return std::to_string(*maybeValue);
    }
    else
    {
        return std::nullopt;
    }
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
