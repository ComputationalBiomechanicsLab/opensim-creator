#include "ComponentOutput.hpp"

#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/Algorithms.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/Component.h>
#include <SimTKcommon.h>

#include <typeinfo>

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


using ExtractorFunc = double(*)(OpenSim::AbstractOutput const&, SimTK::State const&);

static std::string const& GetNoDescriptionString()
{
    static std::string const g_NoDescriptionStr = "(no description)";
    return g_NoDescriptionStr;
}

static std::string GenerateLabel(OpenSim::ComponentPath const& cp,
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


static ExtractorFunc GetExtractorFuncOrNull(OpenSim::AbstractOutput const& ao, osc::OutputSubfield subfield)
{
    if (osc::Is<OpenSim::Output<double>>(ao))
    {
        return output_magic::extractTypeErased<OpenSim::Output<double>>;
    }
    else if (osc::Is<OpenSim::Output<SimTK::Vec3>>(ao))
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

class osc::ComponentOutput::Impl final {
public:
    Impl(OpenSim::AbstractOutput const& ao,
         OutputSubfield subfield) :
        m_ID{},
        m_ComponentAbsPath{ao.getOwner().getAbsolutePath()},
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

    UID getID() const
    {
        return m_ID;
    }

    OutputSource getOutputType() const
    {
        return OutputSource::UserEnacted;
    }

    std::string const& getName() const
    {
        return m_Label;
    }

    std::string const& getDescription() const
    {
        return GetNoDescriptionString();
    }

    bool producesNumericValues() const
    {
        return m_ExtractorFunc != nullptr;
    }

    std::optional<float> getNumericValue(OpenSim::Component const& c, SimulationReport const& report) const
    {
        OpenSim::AbstractOutput const* ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);

        if (!ao)
        {
            return std::nullopt;  // cannot find output
        }

        if (typeid(*ao) != *m_OutputType)
        {
            return std::nullopt;  // the type of the output changed
        }

        if (!m_ExtractorFunc)
        {
            return std::nullopt;  // don't know how to extract a value from the output
        }

        return static_cast<float>(m_ExtractorFunc(*ao, report.getState()));
    }

    std::optional<std::string> getStringValue(OpenSim::Component const& c, SimulationReport const& report) const
    {
        OpenSim::AbstractOutput const* ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);

        if (!ao)
        {
            return std::nullopt;  // cannot find output
        }

        return ao->getValueAsString(report.getState());
    }

private:
    UID m_ID;
    OpenSim::ComponentPath m_ComponentAbsPath;
    std::string m_OutputName;
    std::string m_Label;
    std::type_info const* m_OutputType;
    ExtractorFunc m_ExtractorFunc;
};


// public API

char const* osc::GetOutputSubfieldLabel(OutputSubfield subfield)
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

int osc::GetSupportedSubfields(OpenSim::AbstractOutput const& ao)
{
    if (Is<OpenSim::Output<SimTK::Vec3>>(ao))
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

osc::ComponentOutput::ComponentOutput(OpenSim::AbstractOutput const& ao,
                              OutputSubfield subfield) :
    m_Impl{new Impl{ao, std::move(subfield)}}
{
}
osc::ComponentOutput::ComponentOutput(ComponentOutput const&) = default;
osc::ComponentOutput::ComponentOutput(ComponentOutput&&) noexcept = default;
osc::ComponentOutput& osc::ComponentOutput::operator=(ComponentOutput const&) = default;
osc::ComponentOutput& osc::ComponentOutput::operator=(ComponentOutput&&) noexcept = default;
osc::ComponentOutput::~ComponentOutput() noexcept = default;

osc::UID osc::ComponentOutput::getID() const
{
    return m_Impl->getID();
}

osc::OutputSource osc::ComponentOutput::getOutputSource() const
{
    return m_Impl->getOutputType();
}

std::string const& osc::ComponentOutput::getName() const
{
    return m_Impl->getName();
}

std::string const& osc::ComponentOutput::getDescription() const
{
    return m_Impl->getDescription();
}

bool osc::ComponentOutput::producesNumericValues() const
{
    return m_Impl->producesNumericValues();
}

std::optional<float> osc::ComponentOutput::getNumericValue(OpenSim::Component const& c,
                                                           SimulationReport const& report) const
{
    return m_Impl->getNumericValue(c, report);
}

std::optional<std::string> osc::ComponentOutput::getStringValue(OpenSim::Component const& c,
                                                                SimulationReport const& report) const
{
    return m_Impl->getStringValue(c, report);
}
