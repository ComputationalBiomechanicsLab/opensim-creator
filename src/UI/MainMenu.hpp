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

namespace osc {
    void actionNewModel(std::shared_ptr<MainEditorState>);
    void actionOpenModel(std::shared_ptr<MainEditorState>);

    struct MainMenuFileTab final {
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;

        MainMenuFileTab();

        void draw(std::shared_ptr<MainEditorState>);
    };

    struct MainMenuAboutTab final {
        void draw();
    };

    struct MainMenuWindowTab final {
        void draw(MainEditorState&);
    };
}
