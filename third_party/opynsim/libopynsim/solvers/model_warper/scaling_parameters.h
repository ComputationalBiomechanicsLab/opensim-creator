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
            const auto it = m_Values.find(key);
            if (it == m_Values.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        auto begin() const { return m_Values.begin(); }
        auto end() const { return m_Values.end(); }

        auto try_emplace(const std::string& name, const ScalingParameterValue& value)
        {
            return m_Values.try_emplace(name, value);
        }

        auto insert_or_assign(const std::string& name, const ScalingParameterValue& value)
        {
            return m_Values.insert_or_assign(name, value);
        }
    private:
        std::map<std::string, ScalingParameterValue> m_Values;
    };
}
