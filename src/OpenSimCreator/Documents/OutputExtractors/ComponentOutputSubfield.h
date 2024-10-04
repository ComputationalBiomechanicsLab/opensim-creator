#pragma once

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>

#include <cstdint>
#include <optional>
#include <span>

namespace OpenSim { class AbstractOutput; }
namespace SimTK { class State; }

namespace osc
{
    // flag type that can be used to say what subfields an OpenSim output has
    enum class ComponentOutputSubfield : uint32_t {
        None       = 0,
        X          = 1<<0,
        Y          = 1<<1,
        Z          = 1<<2,
        Magnitude  = 1<<3,
        RX         = 1<<4,
        RY         = 1<<5,
        RZ         = 1<<6,
        RMagnitude = 1<<7,
        NUM_FLAGS  =    8,

        Default = None,
    };

    constexpr ComponentOutputSubfield operator|(ComponentOutputSubfield lhs, ComponentOutputSubfield rhs)
    {
        return static_cast<ComponentOutputSubfield>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(ComponentOutputSubfield lhs, ComponentOutputSubfield rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    std::optional<CStringView> GetOutputSubfieldLabel(ComponentOutputSubfield);
    std::span<const ComponentOutputSubfield> GetAllSupportedOutputSubfields();

    // tests if the output produces numeric values (e.g. float, Vec3, etc. - as opposed to std::string)
    bool ProducesExtractableNumericValues(const OpenSim::AbstractOutput&);

    // returns applicable ComponentOutputSubfield ORed together
    ComponentOutputSubfield GetSupportedSubfields(const OpenSim::AbstractOutput&);

    using SubfieldExtractorFunc = double(*)(const OpenSim::AbstractOutput&, const SimTK::State&);
    SubfieldExtractorFunc GetExtractorFuncOrNull(const OpenSim::AbstractOutput&, ComponentOutputSubfield);
}
