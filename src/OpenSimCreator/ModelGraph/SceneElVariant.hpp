#pragma once

#include <utility>
#include <variant>

namespace osc
{
    class GroundEl;
    class MeshEl;
    class BodyEl;
    class JointEl;
    class StationEl;
    class EdgeEl;

    // a variant for storing a `const` reference to a `const` scene element
    using ConstSceneElVariant = std::variant<
        std::reference_wrapper<GroundEl const>,
        std::reference_wrapper<MeshEl const>,
        std::reference_wrapper<BodyEl const>,
        std::reference_wrapper<JointEl const>,
        std::reference_wrapper<StationEl const>,
        std::reference_wrapper<EdgeEl const>
    >;

    // a variant for storing a non-`const` reference to a non-`const` scene element
    using SceneElVariant = std::variant<
        std::reference_wrapper<GroundEl>,
        std::reference_wrapper<MeshEl>,
        std::reference_wrapper<BodyEl>,
        std::reference_wrapper<JointEl>,
        std::reference_wrapper<StationEl>,
        std::reference_wrapper<EdgeEl>
    >;
}
