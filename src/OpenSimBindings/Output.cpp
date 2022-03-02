#include "Output.hpp"

#include "src/Utils/UID.hpp"

#include <functional>
#include <iostream>
#include <sstream>
#include <string>


osc::UID osc::Output::getID() const
{
    return m_Output->getID();
}

osc::OutputType osc::Output::getOutputType() const
{
    return m_Output->getOutputType();
}

std::string const& osc::Output::getName() const
{
    return m_Output->getName();
}

std::string const& osc::Output::getDescription() const
{
    return m_Output->getDescription();
}

bool osc::Output::producesNumericValues() const
{
    return m_Output->producesNumericValues();
}

std::optional<float> osc::Output::getNumericValue(OpenSim::Model const& model, SimulationReport const& report) const
{
    return m_Output->getNumericValue(model, report);
}

std::optional<std::string> osc::Output::getStringValue(OpenSim::Model const& model, SimulationReport const& report) const
{
    return m_Output->getStringValue(model, report);
}

osc::VirtualOutput const& osc::Output::getInner() const
{
    return *m_Output;
}

bool osc::operator==(Output const& a, Output const& b)
{
    return a.m_Output == b.m_Output;
}

bool osc::operator!=(Output const& a, Output const& b)
{
    return a.m_Output != b.m_Output;
}

bool osc::operator<(Output const& a, Output const& b)
{
    return a.m_Output < b.m_Output;
}

bool osc::operator<=(Output const& a, Output const& b)
{
    return a.m_Output <= b.m_Output;
}

bool osc::operator>(Output const& a, Output const& b)
{
    return a.m_Output > b.m_Output;
}

bool osc::operator>=(Output const& a, Output const& b)
{
    return a.m_Output >= b.m_Output;
}

std::ostream& osc::operator<<(std::ostream& o, Output const& out)
{
    return o << "Output(id = " << out.getID() << ", " << out.getName() << ')';
}

std::string osc::to_string(Output const& out)
{
    std::stringstream ss;
    ss << out;
    return std::move(ss).str();
}

std::size_t std::hash<osc::Output>::operator()(osc::Output const& out) const
{
    return std::hash<std::shared_ptr<osc::VirtualOutput>>{}(out.m_Output);
}
