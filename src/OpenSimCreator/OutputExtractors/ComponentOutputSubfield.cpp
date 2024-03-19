#include "ComponentOutputSubfield.h"

#include <OpenSim/Common/Component.h>  // necessary because ComponentOutput.h doesn't correctly include it
#include <OpenSim/Common/ComponentOutput.h>
#include <SimTKcommon/SmallMatrix.h>

#include <array>
#include <concepts>
#include <optional>
#include <span>

using namespace osc;

// constants
namespace
{
    constexpr auto c_OutputSubfieldsLut = std::to_array(
    {
        ComponentOutputSubfield::X,
        ComponentOutputSubfield::Y,
        ComponentOutputSubfield::Z,
        ComponentOutputSubfield::Magnitude,
    });
}

// templated/concrete subfield extractor functions
namespace
{
    // top-level output extractor declaration
    template<std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // subfield output extractor declaration
    template<ComponentOutputSubfield, std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(OpenSim::Output<double> const& o, SimTK::State const& s)
    {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::X>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Y>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Z>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<ComponentOutputSubfield::Magnitude>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).norm();
    }

    // type-erased version of one of the above
    template<std::derived_from<OpenSim::AbstractOutput> OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s)
    {
        return extract<>(dynamic_cast<OutputType const&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<
        ComponentOutputSubfield sf,
        std::derived_from<OpenSim::AbstractOutput> OutputType
    >
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s)
    {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }
}

std::optional<CStringView> osc::GetOutputSubfieldLabel(ComponentOutputSubfield subfield)
{
    switch (subfield) {
    case ComponentOutputSubfield::X:
        return "X";
    case ComponentOutputSubfield::Y:
        return "Y";
    case ComponentOutputSubfield::Z:
        return "Z";
    case ComponentOutputSubfield::Magnitude:
    case ComponentOutputSubfield::Default:
        return "Magnitude";
    default:
        return std::nullopt;
    }
}

std::span<ComponentOutputSubfield const> osc::GetAllSupportedOutputSubfields()
{
    return c_OutputSubfieldsLut;
}

bool osc::ProducesExtractableNumericValues(OpenSim::AbstractOutput const& ao)
{
    if (dynamic_cast<OpenSim::Output<double> const*>(&ao)) {
        return true;
    }

    if (dynamic_cast<OpenSim::Output<SimTK::Vec3> const*>(&ao)) {
        return true;
    }

    return false;
}

ComponentOutputSubfield osc::GetSupportedSubfields(OpenSim::AbstractOutput const& ao)
{
    if (dynamic_cast<OpenSim::Output<SimTK::Vec3> const*>(&ao)) {
        return
            ComponentOutputSubfield::X |
            ComponentOutputSubfield::Y |
            ComponentOutputSubfield::Z |
            ComponentOutputSubfield::Magnitude;
    }
    else {
        return ComponentOutputSubfield::None;
    }
}

SubfieldExtractorFunc osc::GetExtractorFuncOrNull(
    OpenSim::AbstractOutput const& ao,
    ComponentOutputSubfield subfield)
{
    if (dynamic_cast<OpenSim::Output<double> const*>(&ao))
    {
        return extractTypeErased<OpenSim::Output<double>>;
    }
    else if (dynamic_cast<OpenSim::Output<SimTK::Vec3> const*>(&ao))
    {
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
    else
    {
        return nullptr;
    }
}
