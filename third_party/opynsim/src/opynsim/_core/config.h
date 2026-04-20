#pragma once

namespace nanobind { class module_; }

namespace opyn
{
    void init_config_submodule(nanobind::module_& config_module);
}
