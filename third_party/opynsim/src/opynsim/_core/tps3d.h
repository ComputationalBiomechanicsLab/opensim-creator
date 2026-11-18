#pragma once

namespace nanobind
{
    class module_;  // NOLINT(readability-identifier-naming)
}

namespace opyn
{
    void init_tps3d_submodule(nanobind::module_& tps3d_module);
}
