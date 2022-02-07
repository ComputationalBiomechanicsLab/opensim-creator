#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace OpenSim
{
    class Component;
    class AbstractOutput;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // typedef for a function that can extract a double from some output
    using extrator_fn_t = double(*)(OpenSim::AbstractOutput const&, SimTK::State const&);

    // enables specifying which subfield of an output the user desires
    //
    // not providing this causes the implementation to assume the
    // user desires the top-level output
    struct PlottableOutputSubfield {
        // user-readable name for the subfield
        char const* name;

        // extractor function for this particular subfield
        extrator_fn_t extractor;

        // typehash of the parent abstract output
        //
        // (used for runtime double-checking)
        size_t parentOutputTypeHashcode;
    };

    // returns plottable subfields in the provided output, or empty if the
    // output has no such fields
    std::vector<PlottableOutputSubfield> const& getOutputSubfields(OpenSim::AbstractOutput const&);

    // an output the user is interested in
    struct DesiredOutput final {

        // absolute path to the component that holds the output
        std::string absoluteComponentPath;

        // name of the output on the component
        std::string outputName;

        // user-facing label for this output in the UI
        std::string label;

        // if != nullptr
        //     pointer to a function function that can extract a double from the output
        // else
        //     output is not plottable: call toString on it to "watch" it
        extrator_fn_t extractorFunc;

        // hash of the typeid of the output type
        //
        // this *must* match the typeid of the looked-up output in the model
        // *before* using an `extractor`. Assume the `extractor` does not check
        // the type of `AbstractOutput` at all at runtime.
        size_t outputTypeHashcode;

        // user desires top-level output
        DesiredOutput(
                OpenSim::Component const&,
                OpenSim::AbstractOutput const&);

        // user desires subfield of an output
        DesiredOutput(
                OpenSim::Component const&,
                OpenSim::AbstractOutput const&,
                PlottableOutputSubfield const&);
    };
}
