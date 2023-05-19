#pragma once

#include "oscar/Utils/PropertySystem/ComponentMemberOffset.hpp"
#include "oscar/Utils/PropertySystem/ComponentPath.hpp"
#include "oscar/Utils/PropertySystem/Socket.hpp"

#include <limits>
#include <string_view>

namespace osc { class AbstractSocket; }
namespace osc { class Component; }

namespace osc
{
    void RegisterSocketInParent(
        Component& parent,
        AbstractSocket const&,
        ComponentMemberOffset
    );
    Component const* TryFindComponent(
        Component const&,
        ComponentPath const&
    );
    Component* TryFindComponentMut(
        Component&,
        ComponentPath const&
    );

    // concrete class that defines a socket member in a component
    //
    // (used by `OSC_SOCKET`)
    template<
        typename TParent,
        typename TConnectee,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class SocketDefinition final : public Socket<TConnectee> {
    public:
        SocketDefinition() :
            SocketDefinition{std::string_view{}}
        {
        }

        SocketDefinition(std::string_view initialConnecteePath_) :
            m_ConnecteePath{initialConnecteePath_}
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            RegisterSocketInParent(
                implUpdOwner(),
                *this,
                static_cast<ComponentMemberOffset>(FuncGetOffsetInParent())
            );
        }

    private:
        ComponentPath const& implGetConnecteePath() const final
        {
            return m_ConnecteePath;
        }

        Component const* implTryGetConnectee() const final
        {
            return TryFindComponent(implGetOwner(), m_ConnecteePath);
        }

        Component* implTryUpdConnectee() final
        {
            return TryFindComponentMut(implUpdOwner(), m_ConnecteePath);
        }

        Component const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<Component const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        Component& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<Component*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView implGetName() const final
        {
            return VName;
        }

        CStringView implGetDescription() const final
        {
            return VDescription;
        }

        ComponentPath m_ConnecteePath;
    };
}