#pragma once

#include <libopensimcreator/documents/param_value.h>

#include <liboscar/utilities/clone_ptr.h>

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
        const std::string& get_name(int idx) const;
        const std::string& get_description(int idx) const;
        const ParamValue& get_value(int idx) const;
        std::optional<ParamValue> find_value(std::string_view) const;

        void push_param(std::string_view name, std::string_view description, ParamValue);
        void set_value(int idx, ParamValue);                  // throws if param doesn't exist
        void set_value(const std::string& name, ParamValue);  // throws if param doesn't exist

    private:
        class Impl;
        ClonePtr<Impl> impl_;
    };
}
