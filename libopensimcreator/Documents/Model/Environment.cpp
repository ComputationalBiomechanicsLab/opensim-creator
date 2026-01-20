#include "Environment.h"

#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>

#include <libopynsim/Documents/output_extractors/shared_output_extractor.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utils/algorithms.h>
#include <liboscar/utils/assertions.h>

#include <vector>

using namespace osc;

osc::Environment::Environment() :
    m_ParamBlock{ToParamBlock(ForwardDynamicSimulatorParams{})}
{}

const SharedOutputExtractor& osc::Environment::getUserOutputExtractor(int index) const
{
    return m_OutputExtractors.at(index);
}
void osc::Environment::addUserOutputExtractor(const SharedOutputExtractor& extractor)
{
    m_OutputExtractors.push_back(extractor);
    App::upd().upd_settings().set_value("panels/Output Watches/enabled", true);  // TODO: this should be an event... ;)
}
void osc::Environment::removeUserOutputExtractor(int index)
{
    OSC_ASSERT(0 <= index and index < static_cast<int>(m_OutputExtractors.size()));
    m_OutputExtractors.erase(m_OutputExtractors.begin() + index);
}
bool osc::Environment::hasUserOutputExtractor(const SharedOutputExtractor& extractor) const
{
    return cpp23::contains(m_OutputExtractors, extractor);
}
bool osc::Environment::removeUserOutputExtractor(const SharedOutputExtractor& extractor)
{
    return std::erase(m_OutputExtractors, extractor) > 0;
}
bool osc::Environment::overwriteOrAddNewUserOutputExtractor(const SharedOutputExtractor& old, const SharedOutputExtractor& newer)
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

std::vector<SharedOutputExtractor> osc::Environment::getAllUserOutputExtractors() const
{
    const int nOutputs = getNumUserOutputExtractors();
    std::vector<SharedOutputExtractor> rv;
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i) {
        rv.push_back(getUserOutputExtractor(i));
    }
    return rv;
}
