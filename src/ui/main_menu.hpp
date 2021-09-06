#pragma once

#include "src/RecentFile.hpp"

#include <filesystem>
#include <vector>
#include <memory>

namespace osc {
    struct MainEditorState;
}

namespace OpenSim {
    class Model;
}

namespace osc::ui::main_menu {
    void action_new_model(std::shared_ptr<MainEditorState>);
    void action_open_model(std::shared_ptr<MainEditorState>);
    void action_save(OpenSim::Model&);
    void action_save_as(OpenSim::Model&);

    namespace file_tab {
        struct State final {
            std::vector<std::filesystem::path> example_osims;
            std::vector<RecentFile> recent_files;

            State();
        };

        void draw(State&, std::shared_ptr<MainEditorState>);
    }

    namespace about_tab {
        void draw();
    }

    namespace window_tab {
        void draw(MainEditorState&);
    }
}
