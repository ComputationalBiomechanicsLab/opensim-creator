#include "ModelWarperConfiguration.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <OpenSim/Common/Component.h>
#include <oscar/Utils/HashHelpers.h>

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc::mow;
namespace rgs = std::ranges;

PairedPoints osc::mow::PairedPointSource::getPairedPoints(
    WarpCache& warpCache,
    const OpenSim::Model& sourceModel,
    const OpenSim::Component& sourceComponent)
{
    // ensure no validation errors
    {
        const auto checks = validate(sourceModel, sourceComponent);
        auto it = rgs::find_if(checks, &ValidationCheckResult::is_error);
        if (it != checks.end()) {
            std::stringstream ss;
            ss << getName() << ": validation errors detected:\n";
            do {
                ss << "    - " << it->description() << '\n';
                it = rgs::find_if(it+1, checks.end(), &ValidationCheckResult::is_error);
            } while (it != checks.end());
            throw std::runtime_error{std::move(ss).str()};
        }
    }
    return implGetPairedPoints(warpCache, sourceModel, sourceComponent);
}

std::vector<ValidationCheckResult> osc::mow::PairedPointSource::implValidate(
    const OpenSim::Model&,
    const OpenSim::Component&) const
{
    return {};  // i.e. no validation checks
}

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

const osc::mow::ComponentWarpingStrategy* osc::mow::ModelWarperConfiguration::tryMatchStrategy(const OpenSim::Component& component) const
{
    struct StrategyMatch {
        const ComponentWarpingStrategy* strategy = nullptr;
        StrategyMatchQuality quality = StrategyMatchQuality::none();
    };

    StrategyMatch bestMatch;
    for (const ComponentWarpingStrategy& strategy : getComponentList<ComponentWarpingStrategy>()) {
        const auto quality = strategy.calculateMatchQuality(component);
        if (quality == StrategyMatchQuality::none()) {
            continue;  // no quality
        }
        else if (quality == bestMatch.quality) {
            std::stringstream ss;
            ss << "ambigous match detected: both " << strategy.getAbsolutePathString() << " and " << bestMatch.strategy->getAbsolutePathString() << " match to " << component.getAbsolutePathString();
            OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
        }
        else if (quality > bestMatch.quality) {
            bestMatch = {&strategy, quality};  // overwrite with better quality
        }
    }
    return bestMatch.strategy;
}
