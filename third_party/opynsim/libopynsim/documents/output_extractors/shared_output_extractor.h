#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>
#include <libopynsim/documents/state_view_with_metadata.h>

#include <liboscar/utilities/c_string_view.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace opyn
{
    // concrete reference-counted value-type wrapper for an `OutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class SharedOutputExtractor final {
    public:
        template<typename ConcreteOutputExtractor>
        explicit SharedOutputExtractor(ConcreteOutputExtractor&& output) :
            output_{std::make_shared<ConcreteOutputExtractor>(std::forward<ConcreteOutputExtractor>(output))}
        {}

        osc::CStringView name() const { return output_->name(); }
        osc::CStringView description() const { return output_->description(); }
        OutputExtractorDataType output_type() const { return output_->output_type(); }

        OutputValueExtractor output_value_extractor(const OpenSim::Component& component) const
        {
            return output_->output_value_extractor(component);
        }

        template<typename T>
        requires std::constructible_from<T, osc::Variant&&>
        T value(const OpenSim::Component& component, const StateViewWithMetadata& state) const
        {
            return output_->value<T>(component, state);
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, osc::Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        void values(
            const OpenSim::Component& component,
            const R& states,
            const std::function<void(T)>& consumer) const
        {
            return output_->values<T>(component, states, consumer);
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, osc::Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        std::vector<T> slurp_values(const OpenSim::Component& component, const R& states) const
        {
            return output_->slurp_values<T>(component, states);
        }

        operator const OutputExtractor& () const { return *output_; }
        const OutputExtractor& inner() const { return *output_; }

        friend bool operator==(const SharedOutputExtractor& lhs, const SharedOutputExtractor& rhs)
        {
            return *lhs.output_ == *rhs.output_;
        }
    private:
        friend std::string to_string(const SharedOutputExtractor&);
        friend struct std::hash<SharedOutputExtractor>;

        std::shared_ptr<const OutputExtractor> output_;
    };

    template<std::derived_from<OutputExtractor> ConcreteOutputExtractor, typename... Args>
    requires std::constructible_from<ConcreteOutputExtractor, Args&&...>
    SharedOutputExtractor make_output_extractor(Args&&... args)
    {
        return SharedOutputExtractor{ConcreteOutputExtractor{std::forward<Args>(args)...}};
    }

    std::ostream& operator<<(std::ostream&, const SharedOutputExtractor&);
    std::string to_string(const SharedOutputExtractor&);
}

template<>
struct std::hash<opyn::SharedOutputExtractor> final {
    size_t operator()(const opyn::SharedOutputExtractor& o) const
    {
        return o.output_->hash();
    }
};
