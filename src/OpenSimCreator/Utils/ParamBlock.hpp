#pragma once

#include "OpenSimCreator/Utils/ParamValue.hpp"

#include <oscar/Utils/ClonePtr.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace osc
{
    // a generic block of parameters - usually used to generically read/write
    // values into other systems (e.g. simulators)
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
        std::optional<ParamValue> findValue(char const* c) const
        {
            return findValue(std::string_view{c});
        }
        std::optional<ParamValue> findValue(std::string_view) const;
        std::optional<ParamValue> findValue(std::string const& name) const;

        void pushParam(std::string_view name, std::string_view description, ParamValue);
        void setValue(int idx, ParamValue);  // throws if param doesn't exist
        void setValue(std::string const& name, ParamValue);  // throws if param doesn't exist

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
