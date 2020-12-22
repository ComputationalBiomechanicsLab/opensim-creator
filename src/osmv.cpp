#include "application.hpp"
#include "loading_screen.hpp"

#include <iostream>

// osmv: main method

static const char usage[] = "usage: osmv model.osim [other_models.osim]";

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << usage << std::endl;
        return EXIT_FAILURE;
    }

    auto application = osmv::Application{};
    application.show(std::make_unique<osmv::Loading_screen>(argv[1]));

    return EXIT_SUCCESS;
}
