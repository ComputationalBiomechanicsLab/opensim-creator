#include "opynsim_app.h"

#include <liboscar/platform/app_metadata.h>

namespace
{
    osc::AppMetadata opynsim_default_app_metadata()
    {
        osc::AppMetadata metadata;
        metadata.set_organization_name("opynsim");
        metadata.set_application_name("OPynSim");
        metadata.set_config_filename("opynsim.toml");
        metadata.set_headless_mode(true);
        metadata.set_maximize_main_window(false);
        return metadata;
    }
}

opyn::OPynSimApp::OPynSimApp() :
    osc::App{opynsim_default_app_metadata()}
{}
