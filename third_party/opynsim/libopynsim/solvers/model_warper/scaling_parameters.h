#pragma once

#include <libopynsim/solvers/model_warper/scaling_parameter_value.h>

#include <map>
#include <optional>
#include <string>
#include <type_traits>

namespace opyn
{
    // A collection of runtime scaling parameters, usually created by aggregating
    // from individual `ScalingStep`s and `ScalingParameterOverride`s.
    class ScalingParameters final {
    public:
        template<std::same_as<double> T>
        std::optional<T> lookup(const std::string& key) const
        {
            const auto it = values_.find(key);
            if (it == values_.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        size_t size() const { return values_.size(); }
        auto begin() const { return values_.begin(); }
        auto end() const { return values_.end(); }

        auto try_emplace(const std::string& name, const ScalingParameterValue& value)
        {
            return values_.try_emplace(name, value);
        }

        auto insert_or_assign(const std::string& name, const ScalingParameterValue& value)
        {
            return values_.insert_or_assign(name, value);
        }
    private:
        std::map<std::string, ScalingParameterValue> values_;
    };
}
