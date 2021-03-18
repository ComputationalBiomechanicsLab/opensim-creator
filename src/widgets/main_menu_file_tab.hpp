#pragma once

#include "src/config.hpp"

#include <filesystem>
#include <vector>

namespace OpenSim {
    class Model;
}

namespace osmv {
    struct Main_menu_file_tab_state final {
        std::vector<std::filesystem::path> example_osims = osmv::config::example_osim_files();
        std::vector<config::Recent_file> recent_files = osmv::config::recent_files();
    };

    void main_menu_new();
    void main_menu_open();
    void main_menu_save(OpenSim::Model&);
    void main_menu_save_as(OpenSim::Model&);
    void draw_main_menu_file_tab(Main_menu_file_tab_state&, OpenSim::Model* opened_model = nullptr);
}
