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
#include <vector>

namespace OpenSim { class Component; }
namespace opyn { class StateViewWithMetadata; }
namespace osc { class IOutputValueExtractorVisitor; }

namespace opyn
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

        osc::CStringView name() const { return impl_name(); }
        osc::CStringView description() const { return impl_description(); }

        OutputExtractorDataType output_type() const { return impl_output_type(); }
        OutputValueExtractor output_value_extractor(const OpenSim::Component& component) const
        {
            return impl_output_value_extractor(component);
        }

        template<typename T>
        requires std::constructible_from<T, osc::Variant&&>
        T value(const OpenSim::Component& component, const StateViewWithMetadata& state) const
        {
            return to<T>(output_value_extractor(component)(state));
        }

        template<typename T, std::ranges::forward_range R, std::invocable<T> Consumer>
        requires (
            std::constructible_from<T, osc::Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        void values(
            const OpenSim::Component& component,
            const R& states,
            Consumer&& consumer) const
        {
            const OutputValueExtractor extractor = output_value_extractor(component);
            for (const StateViewWithMetadata& state : states) {
                consumer(to<T>(extractor(state)));
            }
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, osc::Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        std::vector<T> slurp_values(const OpenSim::Component& component, const R& states) const
        {
            std::vector<T> rv;
            if constexpr (std::ranges::sized_range<R>) {
                rv.reserve(std::ranges::size(states));
            }
            values<T>(component, states, [&rv](T value) { rv.push_back(std::move(value)); });
            return rv;
        }

        size_t hash() const { return impl_hash(); }
        bool equals(const OutputExtractor& other) const { return impl_equals(other); }

        friend bool operator==(const OutputExtractor& lhs, const OutputExtractor& rhs)
        {
            return lhs.equals(rhs);
        }
    private:
        virtual osc::CStringView impl_name() const = 0;
        virtual osc::CStringView impl_description() const = 0;
        virtual OutputExtractorDataType impl_output_type() const = 0;
        virtual OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const = 0;
        virtual size_t impl_hash() const = 0;
        virtual bool impl_equals(const OutputExtractor&) const = 0;
    };
}
