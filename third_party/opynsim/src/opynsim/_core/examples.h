#pragma once

namespace nanobind { class module_; }

namespace opyn
{
    void init_examples_submodule(nanobind::module_& examples_module);
}
