#include "application.hpp"
#include "loading_screen.hpp"
#include "globals.hpp"

#include <iostream>

// osmv: main method

static const char usage[] = "usage: osmv model.osim [other_models.osim]";

int main(int argc, char** argv) {
    osmv::DANGER_set_app_startup_time(osmv::clock::now());

    if (argc <= 1) {
        std::cerr << usage << std::endl;
        return EXIT_FAILURE;
    }

    auto application = osmv::Application{};
    application.current_screen = std::make_unique<osmv::Loading_screen>(argv[1]);
    application.show();

    return EXIT_SUCCESS;
}
