#include "PlottableOutputSubfield.hpp"

#include <OpenSim/Common/Component.h>
#include <SimTKcommon.h>

using namespace osc;

// named namespace is due to an MSVC internal linkage compiler bug
namespace subfield_magic {

    enum Subfield_ {
        Subfield_x,
        Subfield_y,
        Subfield_z,
        Subfield_mag,
    };

    // top-level output extractor declaration
    template<typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // subfield output extractor declaration
    template<Subfield_ sf, typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(OpenSim::Output<double> const& o, SimTK::State const& s) {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<Subfield_x>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<Subfield_y>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<Subfield_z>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<Subfield_mag>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).norm();
    }

    // type-erase a concrete extractor function
    template<typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<>(dynamic_cast<OutputType const&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<Subfield_ sf, typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }

    // helper function that wires the above together for the lookup
    template<Subfield_ sf, typename TValue>
    PlottableOutputSubfield subfield(char const* name) {
        return {name, extractTypeErased<sf, OpenSim::Output<TValue>>, typeid(OpenSim::Output<TValue>).hash_code()};
    }

    // create a constant-time lookup for an OpenSim::Output<T>'s available subfields
    static std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> createSubfieldLookup() {
        std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> rv;

        // SimTK::Vec3
        rv[typeid(OpenSim::Output<SimTK::Vec3>).hash_code()] = {
            subfield<Subfield_x, SimTK::Vec3>("x"),
            subfield<Subfield_y, SimTK::Vec3>("y"),
            subfield<Subfield_z, SimTK::Vec3>("z"),
            subfield<Subfield_mag, SimTK::Vec3>("magnitude"),
        };

        return rv;
    }

    // returns top-level extractor function for an AO, or nullptr if it isn't plottable
    static extrator_fn_t extractorFunctionForOutput(OpenSim::AbstractOutput const& ao) {
        auto const* dp = dynamic_cast<OpenSim::Output<double> const*>(&ao);
        if (dp) {
            return extractTypeErased<OpenSim::Output<double>>;
        } else {
            return nullptr;
        }
    }
}



// public API

std::vector<PlottableOutputSubfield> const& osc::getOutputSubfields(
        OpenSim::AbstractOutput const& ao) {

    static std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> const g_SubfieldLut =
            subfield_magic::createSubfieldLookup();
    static std::vector<osc::PlottableOutputSubfield> const g_EmptyResponse = {};

    size_t argTypeid = typeid(ao).hash_code();
    auto it = g_SubfieldLut.find(argTypeid);

    if (it != g_SubfieldLut.end()) {
        return it->second;
    } else {
        return g_EmptyResponse;
    }
}

osc::DesiredOutput::DesiredOutput(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao) :

    absoluteComponentPath{c.getAbsolutePathString()},
    outputName{ao.getName()},
    extractorFunc{subfield_magic::extractorFunctionForOutput(ao)},
    outputTypeHashcode{typeid(ao).hash_code()} {
}

osc::DesiredOutput::DesiredOutput(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao,
        PlottableOutputSubfield const& pls) :
    absoluteComponentPath{c.getAbsolutePathString()},
    outputName{ao.getName()},
    extractorFunc{pls.extractor},
    outputTypeHashcode{typeid(ao).hash_code()} {

    if (pls.parentOutputTypeHashcode != outputTypeHashcode) {
        throw std::runtime_error{"output subfield mismatch: the provided Plottable_output_field does not match the provided AbstractOutput: this is a developer error"};
    }
}
