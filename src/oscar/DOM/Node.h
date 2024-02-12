#pragma once

#include <oscar/DOM/NodePath.h>
#include <oscar/DOM/Object.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace osc { class Variant; }
namespace osc { class PropertyDescriptions; }

namespace osc
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

        template<std::derived_from<Node> TNode = Node>
        TNode const* getParent() const
        {
            return getParent(Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* updParent()
        {
            return updParent(Identity<TNode>{});
        }


        size_t getNumChildren() const;

        template<std::derived_from<Node> TNode = Node>
        TNode const* getChild(size_t i) const
        {
            return getChild(i, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode const* getChild(std::string_view childName) const
        {
            return getChild(childName, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* updChild(size_t i)
        {
            return updChild(i, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* updChild(std::string_view childName)
        {
            return updChild(childName, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode& addChild(std::unique_ptr<TNode> p)
        {
            return addChild(std::move(p), Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode, typename... Args>
        TNode& emplaceChild(Args&&... args)
            requires std::constructible_from<TNode, Args&&...>
        {
            return addChild(std::make_unique<TNode>(std::forward<Args>(args)...));
        }

        bool removeChild(size_t);
        bool removeChild(Node&);
        bool removeChild(std::string_view childName);

        NodePath getAbsolutePath() const;

        template<std::derived_from<Node> TNode = Node>
        TNode const* find(NodePath const& p) const
        {
            return find(p, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* findMut(NodePath const& p)
        {
            return dynamic_cast<TNode*>(findMut(p, Identity<TNode>{}));
        }

    private:
        // You might be (rightly) wondering why this implementation goes through the
        // bother of using `Identity<T>` classes to distinguish overloads etc. rather
        // than just specializing a template function.
        //
        // It's because standard C++ doesn't allow template specialization on class
        // member functions. See: https://stackoverflow.com/a/3057522

        template<typename T>
        struct Identity { using type = T; };

        template<std::derived_from<Node> TDerived>
        TDerived const* getParent(Identity<TDerived>) const
        {
            return dynamic_cast<TDerived const*>(getParent(Identity<Node>{}));
        }
        Node const* getParent(Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* updParent(Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(updParent(Identity<Node>{}));
        }
        Node* updParent(Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived const* getChild(size_t i, Identity<TDerived>) const
        {
            return dynamic_cast<TDerived const*>(getChild(i, Identity<Node>{}));
        }
        Node const* getChild(size_t, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived const* getChild(std::string_view childName, Identity<TDerived>) const
        {
            return dynamic_cast<TDerived const*>(getChild(childName, Identity<Node>{}));
        }
        Node const* getChild(std::string_view, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* updChild(size_t i, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(updChild(i, Identity<Node>{}));
        }
        Node* updChild(size_t, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived* updChild(std::string_view childName, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(updChild(childName, Identity<Node>{}));
        }
        Node* updChild(std::string_view childName, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived& addChild(std::unique_ptr<TDerived> p, Identity<TDerived>)
        {
            TDerived& rv = *p;
            addChild(std::move(p), Identity<Node>{});
            return rv;
        }
        Node& addChild(std::unique_ptr<Node>, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived const* find(NodePath const& p, Identity<TDerived>) const
        {
            return dynamic_cast<TDerived const*>(find(p, Identity<Node>{}));
        }
        Node const* find(NodePath const&, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* findMut(NodePath const& p, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(findMut(p, Identity<Node>{}));
        }
        Node* findMut(NodePath const&, Identity<Node>);

        // lifetime
        // lifetimed ptr to parent
        // children
    };
}
