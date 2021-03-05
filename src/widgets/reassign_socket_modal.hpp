#pragma once

#include <string>

namespace OpenSim {
    class Model;
    class AbstractSocket;
}

namespace osmv {
    struct Reassign_socket_modal final {
        std::string last_connection_error;
        char search[128]{};

        void show(char const* modal_name);
        void close();
        void draw(char const* modal_name, OpenSim::Model& model, OpenSim::AbstractSocket& socket);
    };
}
