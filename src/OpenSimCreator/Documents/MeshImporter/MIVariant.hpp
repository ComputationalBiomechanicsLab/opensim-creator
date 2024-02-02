#pragma once

#include <variant>

namespace osc::mi
{
    class Ground;
    class Mesh;
    class Body;
    class Joint;
    class StationEl;

    // a variant for storing a `const` reference to a `const` object
    using ConstSceneElVariant = std::variant<
        std::reference_wrapper<Ground const>,
        std::reference_wrapper<Mesh const>,
        std::reference_wrapper<Body const>,
        std::reference_wrapper<Joint const>,
        std::reference_wrapper<StationEl const>
    >;

    // a variant for storing a non-`const` reference to a non-`const` object
    using SceneElVariant = std::variant<
        std::reference_wrapper<Ground>,
        std::reference_wrapper<Mesh>,
        std::reference_wrapper<Body>,
        std::reference_wrapper<Joint>,
        std::reference_wrapper<StationEl>
    >;
}
