#pragma once

#include "src/Utils/PropertySystem/AbstractSocket.hpp"

#include <stdexcept>

namespace osc
{
    std::runtime_error CreateConnectionError(AbstractSocket const&);

    // typed base class for a socket
    template<typename TConnectee>
    class Socket : public AbstractSocket {
    public:
        TConnectee const* tryGetConnectee() const
        {
            return dynamic_cast<TConnectee const*>(static_cast<AbstractSocket const&>(*this).tryGetConnectee());
        }

        TConnectee* tryUpdConnectee()
        {
            return dynamic_cast<TConnectee*>(static_cast<AbstractSocket&>(*this).tryUpdConnectee());
        }

        TConnectee const& getConnectee() const
        {
            TConnectee const* connectee = tryGetConnectee();
            if (connectee)
            {
                return *connectee;
            }
            else
            {
                throw CreateConnectionError(*this);
            }
        }

        TConnectee& updConnectee()
        {
            TConnectee const* connectee = tryUpdConnectee();
            if (connectee)
            {
                return *connectee;
            }
            else
            {
                throw CreateConnectionError(*this);
            }
        }
    };
}