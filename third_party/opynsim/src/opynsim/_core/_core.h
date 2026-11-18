#pragma once

namespace opyn
{
    class OPynSimApp;

    OPynSimApp& get_lazy_loaded_opynsim_app();
    void destroy_lazy_loaded_opynsim_app();
}
