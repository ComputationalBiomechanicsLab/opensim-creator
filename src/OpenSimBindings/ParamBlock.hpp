#pragma once

#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <optional>
#include <variant>
#include <string>
#include <string_view>

namespace osc
{
    using ParamValue = std::variant<double, int, IntegratorMethod>;

    class ParamBlock final {
    public:
        ParamBlock();
        ParamBlock(ParamBlock const&);
        ParamBlock(ParamBlock&&) noexcept;
        ParamBlock& operator=(ParamBlock const&);
        ParamBlock& operator=(ParamBlock&&) noexcept;
        ~ParamBlock() noexcept;

        int size() const;
        std::string const& getName(int idx) const;
        std::string const& getDescription(int idx) const;
        ParamValue const& getValue(int idx) const;
        std::optional<ParamValue> findValue(std::string const& name) const;

        void pushParam(std::string_view name, std::string_view description, ParamValue);
        void setValue(int idx, ParamValue);  // throws if param doesn't exist
        void setValue(std::string const& name, ParamValue);  // throws if param doesn't exist

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}
