#pragma once

namespace nanobind
{
    class module_;  // NOLINT(readability-identifier-naming)
}

namespace opyn::python
{
    void def_model_warper(nanobind::module_&);
}
