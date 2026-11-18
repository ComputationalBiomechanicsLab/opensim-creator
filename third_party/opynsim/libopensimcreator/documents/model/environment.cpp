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
    param_block_{to_param_block(ForwardDynamicSimulatorParams{})}
{}

const opyn::SharedOutputExtractor& osc::Environment::get_user_output_extractor(int index) const
{
    return output_extractors_.at(index);
}
void osc::Environment::add_user_output_extractor(const opyn::SharedOutputExtractor& extractor)
{
    output_extractors_.push_back(extractor);
    // TODO: this should be an event... ;)
    App::upd_settings().set_value("panels/Output Watches/enabled", true);
}
void osc::Environment::remove_user_output_extractor(int index)
{
    OSC_ASSERT(0 <= index and index < static_cast<int>(output_extractors_.size()));
    output_extractors_.erase(output_extractors_.begin() + index);
}
bool osc::Environment::has_user_output_extractor(const opyn::SharedOutputExtractor& extractor) const
{
    return cpp23::contains(output_extractors_, extractor);
}
bool osc::Environment::remove_user_output_extractor(const opyn::SharedOutputExtractor& extractor)
{
    return std::erase(output_extractors_, extractor) > 0;
}
bool osc::Environment::overwrite_or_add_new_user_output_extractor(
    const opyn::SharedOutputExtractor& old, const opyn::SharedOutputExtractor& newer)
{
    if (auto it = find_or_nullptr(output_extractors_, old)) {
        *it = newer;
        return true;
    } else {
        output_extractors_.push_back(newer);
        return true;
    }
}

std::vector<opyn::SharedOutputExtractor> osc::Environment::get_all_user_output_extractors() const
{
    const int n_outputs = get_num_user_output_extractors();
    std::vector<opyn::SharedOutputExtractor> rv;
    rv.reserve(n_outputs);
    for (int i = 0; i < n_outputs; ++i) {
        rv.push_back(get_user_output_extractor(i));
    }
    return rv;
}
