#pragma once

#include <OpenSimCreator/Utils/ParamValue.h>

#include <oscar/Utils/ClonePtr.h>

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
        ParamBlock(const ParamBlock&);
        ParamBlock(ParamBlock&&) noexcept;
        ParamBlock& operator=(const ParamBlock&);
        ParamBlock& operator=(ParamBlock&&) noexcept;
        ~ParamBlock() noexcept;

        int size() const;
        const std::string& getName(int idx) const;
        const std::string& getDescription(int idx) const;
        const ParamValue& getValue(int idx) const;
        std::optional<ParamValue> findValue(std::string_view) const;

        void pushParam(std::string_view name, std::string_view description, ParamValue);
        void setValue(int idx, ParamValue);  // throws if param doesn't exist
        void setValue(const std::string& name, ParamValue);  // throws if param doesn't exist

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
