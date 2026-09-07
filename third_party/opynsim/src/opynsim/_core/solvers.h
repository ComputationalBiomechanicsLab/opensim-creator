#pragma once

namespace nanobind { class module_; }

namespace opyn
{
    void init_solvers_submodule(nanobind::module_& solvers_module);
}
