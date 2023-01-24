#pragma once

#include <vector>

namespace osc { class CustomDecorationOptions; }
namespace osc { class SceneDecoration; }
namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    void GenerateModelDecorations(VirtualConstModelStatePair const&, std::vector<SceneDecoration>&, CustomDecorationOptions const&);
    void GenerateModelDecorations(VirtualConstModelStatePair const&, std::vector<SceneDecoration>&);  // default decoration options

    // returns the recommended scale factor for the given model+state pair
    float GetRecommendedScaleFactor(VirtualConstModelStatePair const&);
}