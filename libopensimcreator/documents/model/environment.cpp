#include "environment.h"

#include <libopensimcreator/documents/simulation/forward_dynamic_simulator_params.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>

#include <vector>

using namespace osc;

osc::Environment::Environment() :
    m_ParamBlock{ToParamBlock(ForwardDynamicSimulatorParams{})}
{}

const opyn::SharedOutputExtractor& osc::Environment::getUserOutputExtractor(int index) const
{
    return m_OutputExtractors.at(index);
}
void osc::Environment::addUserOutputExtractor(const opyn::SharedOutputExtractor& extractor)
{
    m_OutputExtractors.push_back(extractor);
    App::upd().upd_settings().set_value("panels/Output Watches/enabled", true);  // TODO: this should be an event... ;)
}
void osc::Environment::removeUserOutputExtractor(int index)
{
    OSC_ASSERT(0 <= index and index < static_cast<int>(m_OutputExtractors.size()));
    m_OutputExtractors.erase(m_OutputExtractors.begin() + index);
}
bool osc::Environment::hasUserOutputExtractor(const opyn::SharedOutputExtractor& extractor) const
{
    return cpp23::contains(m_OutputExtractors, extractor);
}
bool osc::Environment::removeUserOutputExtractor(const opyn::SharedOutputExtractor& extractor)
{
    return std::erase(m_OutputExtractors, extractor) > 0;
}
bool osc::Environment::overwriteOrAddNewUserOutputExtractor(const opyn::SharedOutputExtractor& old, const opyn::SharedOutputExtractor& newer)
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

std::vector<opyn::SharedOutputExtractor> osc::Environment::getAllUserOutputExtractors() const
{
    const int nOutputs = getNumUserOutputExtractors();
    std::vector<opyn::SharedOutputExtractor> rv;
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i) {
        rv.push_back(getUserOutputExtractor(i));
    }
    return rv;
}
