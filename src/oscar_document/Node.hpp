#pragma once

#include <oscar_document/NodePath.hpp>
#include <oscar_document/Object.hpp>

#include <oscar/Utils/ClonePtr.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace osc::doc { class Variant; }
namespace osc::doc { class PropertyDescriptions; }

namespace osc::doc
{
    class Node : public Object {
    protected:
        Node();
        Node(Node const&);
        Node(Node&&) noexcept;
        Node& operator=(Node const&);
        Node& operator=(Node&&) noexcept;

    public:
        std::unique_ptr<Node> clone() const
        {
            return std::unique_ptr<Node>{static_cast<Node*>(static_cast<Object const&>(*this).clone().release())};
        }

        CStringView getName() const;
        void setName(std::string_view);

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived const* getParent() const
        {
            return dynamic_cast<TDerived const*>(getParent<Node>());
        }
        template<>
        Node const* getParent<Node>() const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived* updParent()
        {
            return dynamic_cast<TDerived*>(updParent<Node>());
        }
        template<>
        Node* updParent<Node>();


        size_t getNumChildren() const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived const* getChild(size_t i) const
        {
            return dynamic_cast<TDerived const*>(getChild<Node>(i));
        }
        template<>
        Node const* getChild<Node>(size_t) const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived const* getChild(std::string_view childName) const
        {
            return dynamic_cast<TDerived const*>(getChild<Node>(childName));
        }
        template<>
        Node const* getChild<Node>(std::string_view) const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived* updChild(size_t i)
        {
            return dynamic_cast<TDerived*>(updChild<Node>(i));
        }
        template<>
        Node* updChild<Node>(size_t);

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived* updChild(std::string_view childName)
        {
            return dynamic_cast<TDerived*>(updChild<Node>(childName));
        }
        template<>
        Node* updChild<Node>(std::string_view childName);

        template<
            typename TDerived,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived& addChild(std::unique_ptr<TDerived> p)
        {
            TDerived& rv = *p;
            addChild<Node>(std::move(p));
            return rv;
        }
        template<>
        Node& addChild<Node>(std::unique_ptr<Node>);

        template<
            typename TDerived,
            typename... Args,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived& emplaceChild(Args&&... args)
        {
            return addChild(std::make_unique<TDerived>(std::forward<Args>(args)...));
        }

        bool removeChild(size_t);
        bool removeChild(Node&);
        bool removeChild(std::string_view childName);

        NodePath getAbsolutePath() const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived const* find(NodePath const& p) const
        {
            return dynamic_cast<TDerived const*>(find<Node>(p));
        }
        template<>
        Node const* find<Node>(NodePath const&) const;

        template<
            typename TDerived = Node,
            typename std::enable_if_t<std::is_base_of_v<Node, TDerived>, bool> = true
        >
        TDerived* findMut(NodePath const& p)
        {
            return dynamic_cast<TDerived*>(findMut<Node>(p));
        }
        template<>
        Node* findMut<Node>(NodePath const&);

    private:
        // lifetime
        // lifetimed ptr to parent
        // children
    };
}
