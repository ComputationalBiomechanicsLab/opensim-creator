#pragma once

#include <variant>

namespace osc
{
    class MiGround;
    class MiMesh;
    class MiBody;
    class MiJoint;
    class MiStation;

    // a variant for storing a `const` reference to a `const` object
    using MiVariantConstReference = std::variant<
        std::reference_wrapper<const MiGround>,
        std::reference_wrapper<const MiMesh>,
        std::reference_wrapper<const MiBody>,
        std::reference_wrapper<const MiJoint>,
        std::reference_wrapper<const MiStation>
    >;

    // a variant for storing a non-`const` reference to a non-`const` object
    using MiVariantReference = std::variant<
        std::reference_wrapper<MiGround>,
        std::reference_wrapper<MiMesh>,
        std::reference_wrapper<MiBody>,
        std::reference_wrapper<MiJoint>,
        std::reference_wrapper<MiStation>
    >;
}
