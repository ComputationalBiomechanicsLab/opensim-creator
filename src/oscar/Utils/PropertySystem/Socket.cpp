#include "Socket.hpp"

#include "oscar/Utils/PropertySystem/Component.hpp"
#include "oscar/Utils/PropertySystem/ComponentPath.hpp"

#include <sstream>
#include <string>
#include <utility>

std::runtime_error osc::CreateConnectionError(AbstractSocket const& socket)
{
    std::stringstream ss;
    ss << socket.getOwner().getName() << ": cannot connect to " << std::string_view{socket.getConnecteePath()};
    return std::runtime_error{std::move(ss).str()};
}
