#include "ModelWarperConfiguration.h"

#include <OpenSim/Common/Component.h>
#include <oscar/Utils/HashHelpers.h>

#include <filesystem>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <utility>

osc::mow::ModelWarperConfiguration::ModelWarperConfiguration()
{
    constructProperties();
}

osc::mow::ModelWarperConfiguration::ModelWarperConfiguration(const std::filesystem::path& filePath) :
    OpenSim::Component{filePath.string()}
{
    constructProperties();
    updateFromXMLDocument();
}

void osc::mow::ModelWarperConfiguration::constructProperties()
{}

void osc::mow::ModelWarperConfiguration::extendFinalizeFromProperties()
{
    if (getProperty_components().empty()) {
        return;  // BODGE: the OpenSim API throws if you call `getComponentList` on an
    }

    // note: it's ok to have the same `StrategyTarget` if the `ComponentStrategy` applies
    //       to a different type of component
    //
    // (e.g. if a station warper targets "*", that will capture different components from
    //       a offset frame warper that targets "*")
    using SetElement = std::pair<const std::type_info*, std::string_view>;
    std::unordered_set<SetElement, Hasher<SetElement>> allStrategyTargets;
    for (const auto& warpingStrategy : getComponentList<ComponentWarpingStrategy>()) {
        const std::type_info& targetType = warpingStrategy.getTargetComponentTypeInfo();
        for (int i = 0; i < warpingStrategy.getProperty_StrategyTargets().size(); ++i) {
            if (not allStrategyTargets.emplace(&targetType, warpingStrategy.get_StrategyTargets(i)).second) {
                std::stringstream ss;
                ss << warpingStrategy.get_StrategyTargets(i) << ": duplicate strategy target detected in '" << warpingStrategy.getName() << "': this will confuse the engine and should be resolved";
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
            }
        }
    }
}
