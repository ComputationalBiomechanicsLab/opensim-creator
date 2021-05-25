#pragma once

#include "src/application.hpp"
#include "src/config.hpp"

#include <filesystem>
#include <vector>

namespace osc {
    struct Main_editor_state;
}

namespace OpenSim {
    class Model;
}

namespace osc::ui::main_menu {
    void action_new_model(std::shared_ptr<Main_editor_state> = nullptr);
    void action_open_model(std::shared_ptr<Main_editor_state> = nullptr);
    void action_save(OpenSim::Model&);
    void action_save_as(OpenSim::Model&);

    namespace file_tab {
        struct State final {
            std::vector<std::filesystem::path> example_osims = osc::config::example_osim_files();
            std::vector<config::Recent_file> recent_files = osc::config::recent_files();
        };

        void draw(State&, std::shared_ptr<Main_editor_state> = nullptr);
    }

    namespace about_tab {
        void draw();
    }
}
