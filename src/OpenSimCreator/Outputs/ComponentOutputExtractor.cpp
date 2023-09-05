#include "ComponentOutputExtractor.hpp"

#include "OpenSimCreator/Simulation/SimulationReport.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/Perf.hpp>

#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <typeinfo>
#include <sstream>
#include <utility>
#include <vector>

// named namespace is due to an MSVC internal linkage compiler bug
namespace output_magic
{
    // top-level output extractor declaration
    template<typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // subfield output extractor declaration
    template<osc::OutputSubfield, typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(OpenSim::Output<double> const& o, SimTK::State const& s)
    {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<osc::OutputSubfield::X>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<osc::OutputSubfield::Y>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<osc::OutputSubfield::Z>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<osc::OutputSubfield::Magnitude>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s)
    {
        return o.getValue(s).norm();
    }

    // type-erased version of one of the above
    template<typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s)
    {
        return extract<>(dynamic_cast<OutputType const&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<osc::OutputSubfield sf, typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s)
    {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }
}

// other helpers
namespace
{
    using ExtractorFunc = double(*)(OpenSim::AbstractOutput const&, SimTK::State const&);

    std::string const& GetNoDescriptionString()
    {
        static std::string const s_NoDescriptionStr;
        return s_NoDescriptionStr;
    }

    std::string GenerateLabel(
        OpenSim::ComponentPath const& cp,
        std::string const& outputName,
        osc::OutputSubfield subfield)
    {
        std::stringstream ss;
        ss << cp.toString() << '[' << outputName;
        if (subfield != osc::OutputSubfield::None)
        {
            ss << '.' << osc::GetOutputSubfieldLabel(subfield);
        }
        ss << ']';
        return std::move(ss).str();
    }

    ExtractorFunc GetExtractorFuncOrNull(
        OpenSim::AbstractOutput const& ao,
        osc::OutputSubfield subfield)
    {
        if (typeid(ao) == typeid(OpenSim::Output<double>))
        {
            return output_magic::extractTypeErased<OpenSim::Output<double>>;
        }
        else if (typeid(ao) == typeid(OpenSim::Output<SimTK::Vec3>))
        {
            switch (subfield) {
            case osc::OutputSubfield::X:
                return output_magic::extractTypeErased<osc::OutputSubfield::X, OpenSim::Output<SimTK::Vec3>>;
            case osc::OutputSubfield::Y:
                return output_magic::extractTypeErased<osc::OutputSubfield::Y, OpenSim::Output<SimTK::Vec3>>;
            case osc::OutputSubfield::Z:
                return output_magic::extractTypeErased<osc::OutputSubfield::Z, OpenSim::Output<SimTK::Vec3>>;
            case osc::OutputSubfield::Magnitude:
                return output_magic::extractTypeErased<osc::OutputSubfield::Magnitude, OpenSim::Output<SimTK::Vec3>>;
            default:
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

    std::vector<osc::OutputSubfield> CreateOutputSubfieldsLut()
    {
        return std::vector<osc::OutputSubfield>
        {
            osc::OutputSubfield::X,
            osc::OutputSubfield::Y,
            osc::OutputSubfield::Z,
            osc::OutputSubfield::Magnitude,
        };
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

    osc::CStringView getName() const
    {
        return m_Label;
    }

    osc::CStringView getDescription() const
    {
        return GetNoDescriptionString();
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
        nonstd::span<SimulationReport const> const reports(&r, 1);
        getValuesFloat(c, reports, v);
        return v.front();
    }

    void getValuesFloat(OpenSim::Component const& c,
                        nonstd::span<SimulationReport const> reports,
                        nonstd::span<float> out) const
    {
        OSC_PERF("osc::ComponentOutputExtractor::getValuesFloat");
        OSC_ASSERT_ALWAYS(reports.size() == out.size());

        OpenSim::AbstractOutput const* const ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);

        if (!ao || typeid(*ao) != *m_OutputType || !m_ExtractorFunc)
        {
            // cannot find output
            // or the type of the output changed
            // or don't know how to extract a value from the output
            std::fill(out.begin(), out.end(), NAN);
            return;
        }

        OSC_PERF("osc::ComponentOutputExtractor::getValuesFloat");

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
        return osc::HashOf(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputType, m_ExtractorFunc);
    }

    bool equals(VirtualOutputExtractor const& other)
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

osc::CStringView osc::GetOutputSubfieldLabel(OutputSubfield subfield)
{
    switch (subfield) {
    case osc::OutputSubfield::X:
        return "X";
    case osc::OutputSubfield::Y:
        return "Y";
    case osc::OutputSubfield::Z:
        return "Z";
    case osc::OutputSubfield::Magnitude:
    case osc::OutputSubfield::Default:
        return "Magnitude";
    default:
        return "Unknown";
    }
}

nonstd::span<osc::OutputSubfield const> osc::GetAllSupportedOutputSubfields()
{
    static auto const s_AllSubfields = CreateOutputSubfieldsLut();
    return s_AllSubfields;
}

int osc::GetSupportedSubfields(OpenSim::AbstractOutput const& ao)
{
    if (typeid(ao) == typeid(OpenSim::Output<SimTK::Vec3>))
    {
        int rv = static_cast<int>(osc::OutputSubfield::X);
        rv |= static_cast<int>(osc::OutputSubfield::Y);
        rv |= static_cast<int>(osc::OutputSubfield::Z);
        rv |= static_cast<int>(osc::OutputSubfield::Magnitude);
        return rv;
    }
    else
    {
        return static_cast<int>(osc::OutputSubfield::None);
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

osc::CStringView osc::ComponentOutputExtractor::getName() const
{
    return m_Impl->getName();
}

osc::CStringView osc::ComponentOutputExtractor::getDescription() const
{
    return m_Impl->getDescription();
}

osc::OutputType osc::ComponentOutputExtractor::getOutputType() const
{
    return m_Impl->getOutputType();
}

float osc::ComponentOutputExtractor::getValueFloat(OpenSim::Component const& c,
                                                   SimulationReport const& r) const
{
    return m_Impl->getValueFloat(c, r);
}

void osc::ComponentOutputExtractor::getValuesFloat(OpenSim::Component const& c,
                                                   nonstd::span<osc::SimulationReport const> reports,
                                                   nonstd::span<float> out) const
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

bool osc::ComponentOutputExtractor::equals(VirtualOutputExtractor const& other) const
{
    return m_Impl->equals(other);
}
