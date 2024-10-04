#include "ComponentOutputSubfield.h"

#include <OpenSim/Common/Component.h>  // necessary because ComponentOutput.h doesn't correctly include it
#include <OpenSim/Common/ComponentOutput.h>
#include <SimTKcommon/SmallMatrix.h>

#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <concepts>
#include <optional>
#include <span>

using namespace osc;

// constants
namespace
{
    constexpr std::array<ComponentOutputSubfield, num_flags<ComponentOutputSubfield>()> c_OutputSubfieldsLut = std::to_array(
    {
        ComponentOutputSubfield::X,
        ComponentOutputSubfield::Y,
        ComponentOutputSubfield::Z,
        ComponentOutputSubfield::Magnitude,
        ComponentOutputSubfield::RX,
        ComponentOutputSubfield::RY,
        ComponentOutputSubfield::RZ,
        ComponentOutputSubfield::RMagnitude,
    });
}

// templated/concrete subfield extractor functions
namespace
{
    // top-level output extractor declaration
    template<std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(const ConcreteOutput&, const SimTK::State&);

    // subfield output extractor declaration
    template<ComponentOutputSubfield, std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(const ConcreteOutput&, const SimTK::State&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(const OpenSim::Output<double>& o, const SimTK::State& s)
    {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::X>(const OpenSim::Output<SimTK::Vec3>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Y>(const OpenSim::Output<SimTK::Vec3>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Z>(const OpenSim::Output<SimTK::Vec3>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Magnitude>(const OpenSim::Output<SimTK::Vec3>& o, const SimTK::State& s)
    {
        return o.getValue(s).norm();
    }

    // extract X from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::X>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(1).get(0);
    }

    // extract Y from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::Y>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(1).get(1);
    }

    // extract Z from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::Z>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(1).get(2);
    }

    // extract Magnitude from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::Magnitude>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(1).norm();
    }

    // extract RX from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::RX>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(0).get(0);
    }

    // extract RY from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::RY>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(0).get(1);
    }

    // extract RZ from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::RZ>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(0).get(2);
    }

    // extract RMagnitude from `SimTK::SpatialVec`
    template<>
    double extract<ComponentOutputSubfield::RMagnitude>(const OpenSim::Output<SimTK::SpatialVec>& o, const SimTK::State& s)
    {
        return o.getValue(s).get(0).norm();
    }

    // type-erased version of one of the above
    template<std::derived_from<OpenSim::AbstractOutput> OutputType>
    double extractTypeErased(const OpenSim::AbstractOutput& o, const SimTK::State& s)
    {
        return extract<>(dynamic_cast<const OutputType&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<
        ComponentOutputSubfield sf,
        std::derived_from<OpenSim::AbstractOutput> OutputType
    >
    double extractTypeErased(const OpenSim::AbstractOutput& o, const SimTK::State& s)
    {
        return extract<sf>(dynamic_cast<const OutputType&>(o), s);
    }
}

std::optional<CStringView> osc::GetOutputSubfieldLabel(ComponentOutputSubfield subfield)
{
    static_assert(num_flags<ComponentOutputSubfield>() == 8);

    switch (subfield) {
    case ComponentOutputSubfield::X:          return "X";
    case ComponentOutputSubfield::Y:          return "Y";
    case ComponentOutputSubfield::Z:          return "Z";
    case ComponentOutputSubfield::Magnitude:  return "Magnitude";
    case ComponentOutputSubfield::RX:         return "RX";
    case ComponentOutputSubfield::RY:         return "RY";
    case ComponentOutputSubfield::RZ:         return "RZ";
    case ComponentOutputSubfield::RMagnitude: return "RMagnitude";
    case ComponentOutputSubfield::Default:    return "Magnitude";
    default:                                  return std::nullopt;
    }
}

std::span<const ComponentOutputSubfield> osc::GetAllSupportedOutputSubfields()
{
    return c_OutputSubfieldsLut;
}

bool osc::ProducesExtractableNumericValues(const OpenSim::AbstractOutput& ao)
{
    if (dynamic_cast<const OpenSim::Output<double>*>(&ao)) {
        return true;
    }

    if (dynamic_cast<const OpenSim::Output<SimTK::Vec3>*>(&ao)) {
        return true;
    }

    return false;
}

ComponentOutputSubfield osc::GetSupportedSubfields(const OpenSim::AbstractOutput& ao)
{
    if (dynamic_cast<const OpenSim::Output<SimTK::Vec3>*>(&ao)) {
        return
            ComponentOutputSubfield::X |
            ComponentOutputSubfield::Y |
            ComponentOutputSubfield::Z |
            ComponentOutputSubfield::Magnitude;
    }
    else if (dynamic_cast<const OpenSim::Output<SimTK::SpatialVec>*>(&ao)) {
        return
            ComponentOutputSubfield::X |
            ComponentOutputSubfield::Y |
            ComponentOutputSubfield::Z |
            ComponentOutputSubfield::Magnitude |
            ComponentOutputSubfield::RX |
            ComponentOutputSubfield::RY |
            ComponentOutputSubfield::RZ |
            ComponentOutputSubfield::RMagnitude;
    }
    else {
        return ComponentOutputSubfield::None;
    }
}

SubfieldExtractorFunc osc::GetExtractorFuncOrNull(
    const OpenSim::AbstractOutput& ao,
    ComponentOutputSubfield subfield)
{
    if (dynamic_cast<const OpenSim::Output<double>*>(&ao)) {
        return extractTypeErased<OpenSim::Output<double>>;
    }
    else if (dynamic_cast<const OpenSim::Output<SimTK::Vec3>*>(&ao)) {
        switch (subfield) {
        case ComponentOutputSubfield::X:
            return extractTypeErased<ComponentOutputSubfield::X, OpenSim::Output<SimTK::Vec3>>;
        case ComponentOutputSubfield::Y:
            return extractTypeErased<ComponentOutputSubfield::Y, OpenSim::Output<SimTK::Vec3>>;
        case ComponentOutputSubfield::Z:
            return extractTypeErased<ComponentOutputSubfield::Z, OpenSim::Output<SimTK::Vec3>>;
        case ComponentOutputSubfield::Magnitude:
            return extractTypeErased<ComponentOutputSubfield::Magnitude, OpenSim::Output<SimTK::Vec3>>;
        default:
            return nullptr;
        }
    }
    else if (dynamic_cast<const OpenSim::Output<SimTK::SpatialVec>*>(&ao)) {
        switch (subfield) {
        case ComponentOutputSubfield::X:
            return extractTypeErased<ComponentOutputSubfield::X, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::Y:
            return extractTypeErased<ComponentOutputSubfield::Y, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::Z:
            return extractTypeErased<ComponentOutputSubfield::Z, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::Magnitude:
            return extractTypeErased<ComponentOutputSubfield::Magnitude, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::RX:
            return extractTypeErased<ComponentOutputSubfield::RX, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::RY:
            return extractTypeErased<ComponentOutputSubfield::RY, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::RZ:
            return extractTypeErased<ComponentOutputSubfield::RZ, OpenSim::Output<SimTK::SpatialVec>>;
        case ComponentOutputSubfield::RMagnitude:
            return extractTypeErased<ComponentOutputSubfield::RMagnitude, OpenSim::Output<SimTK::SpatialVec>>;
        default:
            return nullptr;
        }
    }
    else {
        return nullptr;
    }
}
