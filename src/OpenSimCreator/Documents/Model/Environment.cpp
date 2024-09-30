#include "Environment.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>

#include <vector>

using namespace osc;

osc::Environment::Environment() :
    m_ParamBlock{ToParamBlock(ForwardDynamicSimulatorParams{})}
{}

const OutputExtractor& osc::Environment::getUserOutputExtractor(int index) const
{
    return m_OutputExtractors.at(index);
}
void osc::Environment::addUserOutputExtractor(const OutputExtractor& extractor)
{
    m_OutputExtractors.push_back(extractor);
    App::upd().upd_settings().set_value("panels/Output Watches/enabled", true);  // TODO: this should be an event... ;)
}
void osc::Environment::removeUserOutputExtractor(int index)
{
    OSC_ASSERT(0 <= index and index < static_cast<int>(m_OutputExtractors.size()));
    m_OutputExtractors.erase(m_OutputExtractors.begin() + index);
}
bool osc::Environment::hasUserOutputExtractor(const OutputExtractor& extractor) const
{
    return cpp23::contains(m_OutputExtractors, extractor);
}
bool osc::Environment::removeUserOutputExtractor(const OutputExtractor& extractor)
{
    return std::erase(m_OutputExtractors, extractor) > 0;
}
bool osc::Environment::overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer)
{
    if (auto it = find_or_nullptr(m_OutputExtractors, old)) {
        *it = newer;
        return true;
    }
    else {
        m_OutputExtractors.push_back(newer);
        return true;
    }
}

std::vector<OutputExtractor> osc::Environment::getAllUserOutputExtractors() const
{
    int nOutputs = getNumUserOutputExtractors();

    std::vector<OutputExtractor> rv;
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i) {
        rv.push_back(getUserOutputExtractor(i));
    }
    return rv;
}
