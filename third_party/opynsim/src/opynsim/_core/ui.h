#pragma once

namespace nanobind
{
    class module_;  // NOLINT(readability-identifier-naming)
}

namespace opyn
{
    void init_ui_submodule(nanobind::module_& ui_module);
}
