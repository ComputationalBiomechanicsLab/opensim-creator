#pragma once

#include "src/Utils/CStringView.hpp"

namespace osc { class Component; }
namespace osc { class ComponentPath; }
namespace osc { template<typename TComponent> class Socket; }

namespace osc
{
    // type-erased base class for a socket
    class AbstractSocket {
    private:
        // only a Socket<TValue> may construct this base class
        template<typename TComponent> 
        friend class Socket;

        AbstractSocket() = default;
        AbstractSocket(AbstractSocket const&) = default;
        AbstractSocket(AbstractSocket&&) noexcept = default;
        AbstractSocket& operator=(AbstractSocket const&) = default;
        AbstractSocket& operator=(AbstractSocket&&) noexcept = default;
    public:
        virtual ~AbstractSocket() noexcept = default;

        ComponentPath const& getConnecteePath() const
        {
            return implGetConnecteePath();
        }

        Component const* tryGetConnectee() const
        {
            return implTryGetConnectee();
        }

        Component* tryUpdConnectee()
        {
            return implTryUpdConnectee();
        }

        Component const& getOwner() const
        {
            return implGetOwner();
        }

        Component& updOwner()
        {
            return implUpdOwner();
        }

        CStringView getName() const
        {
            return implGetName();
        }

        CStringView getDescription() const
        {
            return implGetDescription();
        }

    private:
        virtual ComponentPath const& implGetConnecteePath() const = 0;
        virtual Component const* implTryGetConnectee() const = 0;
        virtual Component* implTryUpdConnectee() = 0;
        virtual Component const& implGetOwner() const = 0;
        virtual Component& implUpdOwner() = 0;
        virtual CStringView implGetName() const = 0;
        virtual CStringView implGetDescription() const = 0;
    };
}