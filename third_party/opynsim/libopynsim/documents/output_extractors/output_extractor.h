#pragma once

#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/variant/variant.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

namespace OpenSim { class Component; }
namespace osc { class IOutputValueExtractorVisitor; }
namespace osc { class StateViewWithMetadata; }

namespace osc
{
    // an interface for something that can produce an output value extractor
    // for a particular model against multiple states
    //
    // implementors of this interface are assumed to be immutable (important,
    // because output extractors might be shared between simulations, threads,
    // etc.)
    class OutputExtractor {
    protected:
        OutputExtractor() = default;
        OutputExtractor(const OutputExtractor&) = default;
        OutputExtractor(OutputExtractor&&) noexcept = default;
        OutputExtractor& operator=(const OutputExtractor&) = default;
        OutputExtractor& operator=(OutputExtractor&&) noexcept = default;
    public:
        virtual ~OutputExtractor() noexcept = default;

        CStringView getName() const { return implGetName(); }
        CStringView getDescription() const { return implGetDescription(); }

        OutputExtractorDataType getOutputType() const { return implGetOutputType(); }
        OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
        {
            return implGetOutputValueExtractor(component);
        }

        template<typename T>
        requires std::constructible_from<T, Variant&&>
        T getValue(const OpenSim::Component& component, const StateViewWithMetadata& state) const
        {
            return to<T>(getOutputValueExtractor(component)(state));
        }

        template<typename T, std::ranges::forward_range R, std::invocable<T> Consumer>
        requires (
            std::constructible_from<T, Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        void getValues(
            const OpenSim::Component& component,
            const R& states,
            Consumer&& consumer) const
        {
            const OutputValueExtractor extractor = getOutputValueExtractor(component);
            for (const StateViewWithMetadata& state : states) {
                consumer(to<T>(extractor(state)));
            }
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        std::vector<T> slurpValues(const OpenSim::Component& component, const R& states) const
        {
            std::vector<T> rv;
            if constexpr (std::ranges::sized_range<R>) {
                rv.reserve(std::ranges::size(states));
            }
            getValues<T>(component, states, [&rv](T value) { rv.push_back(std::move(value)); });
            return rv;
        }

        size_t getHash() const { return implGetHash(); }
        bool equals(const OutputExtractor& other) const { return implEquals(other); }

        friend bool operator==(const OutputExtractor& lhs, const OutputExtractor& rhs)
        {
            return lhs.equals(rhs);
        }
    private:
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
        virtual OutputExtractorDataType implGetOutputType() const = 0;
        virtual OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const = 0;
        virtual size_t implGetHash() const = 0;
        virtual bool implEquals(const OutputExtractor&) const = 0;
    };
}
