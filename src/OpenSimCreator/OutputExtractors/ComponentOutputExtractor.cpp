#include "ComponentOutputExtractor.h"

#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/SmallMatrix.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/Perf.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <concepts>
#include <memory>
#include <span>
#include <sstream>
#include <typeinfo>
#include <utility>

using namespace osc;

// constants
namespace
{
    constexpr auto c_OutputSubfieldsLut = std::to_array(
    {
        OutputSubfield::X,
        OutputSubfield::Y,
        OutputSubfield::Z,
        OutputSubfield::Magnitude,
    });
}

// named namespace is due to an MSVC internal linkage compiler bug
namespace
{
    // top-level output extractor declaration
    template<std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // subfield output extractor declaration
    template<OutputSubfield, std::derived_from<OpenSim::AbstractOutput> ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(OpenSim::Output<double> const& o, SimTK::State const& s)
    {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<OutputSubfield::X>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<OutputSubfield::Y>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<OutputSubfield::Z>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<OutputSubfield::Magnitude>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
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
        OutputSubfield sf,
        std::derived_from<OpenSim::AbstractOutput> OutputType
    >
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s)
    {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }
}

// other helpers
namespace
{
    using ExtractorFunc = double(*)(OpenSim::AbstractOutput const&, SimTK::State const&);

    std::string GenerateLabel(
        OpenSim::ComponentPath const& cp,
        std::string const& outputName,
        OutputSubfield subfield)
    {
        std::stringstream ss;
        ss << cp.toString() << '[' << outputName;
        if (auto label = GetOutputSubfieldLabel(subfield))
        {
            ss << '.' << *label;
        }
        ss << ']';
        return std::move(ss).str();
    }

    ExtractorFunc GetExtractorFuncOrNull(
        OpenSim::AbstractOutput const& ao,
        OutputSubfield subfield)
    {
        if (dynamic_cast<OpenSim::Output<double> const*>(&ao))
        {
            return extractTypeErased<OpenSim::Output<double>>;
        }
        else if (dynamic_cast<OpenSim::Output<SimTK::Vec3> const*>(&ao))
        {
            switch (subfield) {
            case OutputSubfield::X:
                return extractTypeErased<OutputSubfield::X, OpenSim::Output<SimTK::Vec3>>;
            case OutputSubfield::Y:
                return extractTypeErased<OutputSubfield::Y, OpenSim::Output<SimTK::Vec3>>;
            case OutputSubfield::Z:
                return extractTypeErased<OutputSubfield::Z, OpenSim::Output<SimTK::Vec3>>;
            case OutputSubfield::Magnitude:
                return extractTypeErased<OutputSubfield::Magnitude, OpenSim::Output<SimTK::Vec3>>;
            default:
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }
}

class osc::ComponentOutputExtractor::Impl final {
public:
    Impl(OpenSim::AbstractOutput const& ao,
         OutputSubfield subfield) :

        m_ComponentAbsPath{GetAbsolutePath(GetOwnerOrThrow(ao))},
        m_OutputName{ao.getName()},
        m_Label{GenerateLabel(m_ComponentAbsPath, m_OutputName, subfield)},
        m_OutputType{&typeid(ao)},
        m_ExtractorFunc{GetExtractorFuncOrNull(ao, subfield)}
    {
    }

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    OpenSim::ComponentPath const& getComponentAbsPath() const
    {
        return m_ComponentAbsPath;
    }

    CStringView getName() const
    {
        return m_Label;
    }

    CStringView getDescription() const
    {
        return CStringView{};
    }

    bool producesNumericValues() const
    {
        return m_ExtractorFunc != nullptr;
    }

    OutputType getOutputType() const
    {
        return m_ExtractorFunc ? OutputType::Float : OutputType::String;
    }

    float getValueFloat(OpenSim::Component const& c, SimulationReport const& r) const
    {
        std::array<float, 1> v{};
        std::span<SimulationReport const> const reports(&r, 1);
        getValuesFloat(c, reports, v);
        return v.front();
    }

    void getValuesFloat(OpenSim::Component const& c,
                        std::span<SimulationReport const> reports,
                        std::span<float> out) const
    {
        OSC_PERF("ComponentOutputExtractor::getValuesFloat");
        OSC_ASSERT_ALWAYS(reports.size() == out.size());

        OpenSim::AbstractOutput const* const ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);

        if (!ao || typeid(*ao) != *m_OutputType || !m_ExtractorFunc)
        {
            // cannot find output
            // or the type of the output changed
            // or don't know how to extract a value from the output
            std::fill(out.begin(), out.end(), quiet_nan_v<float>);
            return;
        }

        OSC_PERF("ComponentOutputExtractor::getValuesFloat");

        for (size_t i = 0; i < reports.size(); ++i)
        {
            out[i] = static_cast<float>(m_ExtractorFunc(*ao, reports[i].getState()));
        }
    }

    std::string getValueString(OpenSim::Component const& c, SimulationReport const& r) const
    {
        OpenSim::AbstractOutput const* const ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);
        if (ao)
        {
            if (m_ExtractorFunc)
            {
                return std::to_string(m_ExtractorFunc(*ao, r.getState()));
            }
            else
            {
                return ao->getValueAsString(r.getState());
            }
        }
        else
        {
            return std::string{};
        }
    }

    std::size_t getHash() const
    {
        return HashOf(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputType, m_ExtractorFunc);
    }

    bool equals(IOutputExtractor const& other)
    {
        auto const* const otherT = dynamic_cast<ComponentOutputExtractor const*>(&other);
        if (!otherT)
        {
            return false;
        }

        ComponentOutputExtractor::Impl const* const otherImpl = otherT->m_Impl.get();
        if (otherImpl == this)
        {
            return true;
        }

        return
            m_ComponentAbsPath == otherImpl->m_ComponentAbsPath &&
            m_OutputName == otherImpl->m_OutputName &&
            m_Label == otherImpl->m_Label &&
            m_OutputType == otherImpl->m_OutputType &&
            m_ExtractorFunc == otherImpl->m_ExtractorFunc;
    }

private:
    OpenSim::ComponentPath m_ComponentAbsPath;
    std::string m_OutputName;
    std::string m_Label;
    std::type_info const* m_OutputType;
    ExtractorFunc m_ExtractorFunc;
};


// public API

std::optional<CStringView> osc::GetOutputSubfieldLabel(OutputSubfield subfield)
{
    switch (subfield) {
    case OutputSubfield::X:
        return "X";
    case OutputSubfield::Y:
        return "Y";
    case OutputSubfield::Z:
        return "Z";
    case OutputSubfield::Magnitude:
    case OutputSubfield::Default:
        return "Magnitude";
    default:
        return std::nullopt;
    }
}

std::span<OutputSubfield const> osc::GetAllSupportedOutputSubfields()
{
    return c_OutputSubfieldsLut;
}

OutputSubfield osc::GetSupportedSubfields(OpenSim::AbstractOutput const& ao)
{
    if (dynamic_cast<OpenSim::Output<SimTK::Vec3> const*>(&ao))
    {
        return
            OutputSubfield::X |
            OutputSubfield::Y |
            OutputSubfield::Z |
            OutputSubfield::Magnitude;
    }
    else
    {
        return OutputSubfield::None;
    }
}

osc::ComponentOutputExtractor::ComponentOutputExtractor(OpenSim::AbstractOutput const& ao,
                                                        OutputSubfield subfield) :
    m_Impl{std::make_unique<Impl>(ao, subfield)}
{
}
osc::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor const&) = default;
osc::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(ComponentOutputExtractor const&) = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor::~ComponentOutputExtractor() noexcept = default;

OpenSim::ComponentPath const& osc::ComponentOutputExtractor::getComponentAbsPath() const
{
    return m_Impl->getComponentAbsPath();
}

CStringView osc::ComponentOutputExtractor::getName() const
{
    return m_Impl->getName();
}

CStringView osc::ComponentOutputExtractor::getDescription() const
{
    return m_Impl->getDescription();
}

OutputType osc::ComponentOutputExtractor::getOutputType() const
{
    return m_Impl->getOutputType();
}

float osc::ComponentOutputExtractor::getValueFloat(OpenSim::Component const& c,
                                                   SimulationReport const& r) const
{
    return m_Impl->getValueFloat(c, r);
}

void osc::ComponentOutputExtractor::getValuesFloat(OpenSim::Component const& c,
                                                   std::span<SimulationReport const> reports,
                                                   std::span<float> out) const
{
    m_Impl->getValuesFloat(c, reports, out);
}

std::string osc::ComponentOutputExtractor::getValueString(OpenSim::Component const& c,
                                                          SimulationReport const& r) const
{
    return m_Impl->getValueString(c, r);
}

std::size_t osc::ComponentOutputExtractor::getHash() const
{
    return m_Impl->getHash();
}

bool osc::ComponentOutputExtractor::equals(IOutputExtractor const& other) const
{
    return m_Impl->equals(other);
}
