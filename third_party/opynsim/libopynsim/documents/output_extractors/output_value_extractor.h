#pragma once

#include <libopynsim/documents/state_view_with_metadata.h>

#include <liboscar/variant/variant.h>

#include <concepts>
#include <functional>
#include <utility>

namespace opyn
{
    // encapsulates a function that can extract a single output value from a `StateViewWithMetadata`
    //
    // be careful about lifetimes: these value extractors are usually "tied" to a component that
    // they're extracting from, so it's handy to ensure that the callback function has proper
    // lifetime management (e.g. reference counted pointers or similar)
    class OutputValueExtractor final {
    public:
        static OutputValueExtractor constant(osc::Variant value)
        {
            return OutputValueExtractor{std::move(value)};
        }

        template<typename T>
        requires std::constructible_from<osc::Variant, T&&>
        static OutputValueExtractor constant(T&& value)
        {
            return OutputValueExtractor{osc::Variant{std::forward<T>(value)}};
        }

        explicit OutputValueExtractor(std::function<osc::Variant(const StateViewWithMetadata&)> callback) :
            callback_{std::move(callback)}
        {}

        osc::Variant operator()(const StateViewWithMetadata& state) const { return callback_(state); }
    private:
        explicit OutputValueExtractor(osc::Variant value) :
            callback_{[v = std::move(value)](const StateViewWithMetadata&) { return v; }}
        {}

        std::function<osc::Variant(const StateViewWithMetadata&)> callback_;
    };
}
