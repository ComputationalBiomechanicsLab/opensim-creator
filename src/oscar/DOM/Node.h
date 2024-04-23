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
        Node(const Node&);
        Node(Node&&) noexcept;
        Node& operator=(const Node&);
        Node& operator=(Node&&) noexcept;

    public:
        std::unique_ptr<Node> clone() const
        {
            return std::unique_ptr<Node>{static_cast<Node*>(static_cast<const Object&>(*this).clone().release())};
        }

        CStringView name() const;
        void set_name(std::string_view);

        template<std::derived_from<Node> TNode = Node>
        const TNode* parent() const
        {
            return parent(identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_parent()
        {
            return upd_parent(identity<TNode>{});
        }


        size_t num_children() const;

        template<std::derived_from<Node> TNode = Node>
        const TNode* child(size_t pos) const
        {
            return child(pos, identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        const TNode* child(std::string_view child_name) const
        {
            return child(child_name, identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_child(size_t pos)
        {
            return upd_child(pos, identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_child(std::string_view child_name)
        {
            return upd_child(child_name, identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode& add_child(std::unique_ptr<TNode> node_ptr)
        {
            return add_child(std::move(node_ptr), identity<TNode>{});
        }

        template<std::derived_from<Node> TNode, typename... Args>
        requires std::constructible_from<TNode, Args&&...>
        TNode& emplace_child(Args&&... args)
        {
            return add_child(std::make_unique<TNode>(std::forward<Args>(args)...));
        }

        bool remove_child(size_t);
        bool remove_child(Node&);
        bool remove_child(std::string_view child_name);

        NodePath absolute_path() const;

        template<std::derived_from<Node> TNode = Node>
        const TNode* find(const NodePath& node_path) const
        {
            return find(node_path, identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* find_mut(const NodePath& node_path)
        {
            return dynamic_cast<TNode*>(find_mut(node_path, identity<TNode>{}));
        }

    private:
        // You might be (rightly) wondering why this implementation goes through the
        // bother of using `Identity<T>` classes to distinguish overloads etc. rather
        // than just specializing a template function.
        //
        // It's because standard C++ doesn't allow template specialization on class
        // member functions. See: https://stackoverflow.com/a/3057522

        template<typename T>
        struct identity { using type = T; };

        template<std::derived_from<Node> TDerived>
        const TDerived* parent(identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(parent(identity<Node>{}));
        }
        const Node* parent(identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* upd_parent(identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_parent(identity<Node>{}));
        }
        Node* upd_parent(identity<Node>);

        template<std::derived_from<Node> TDerived>
        const TDerived* child(size_t pos, identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(child(pos, identity<Node>{}));
        }
        const Node* child(size_t, identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        const TDerived* child(std::string_view child_name, identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(child(child_name, identity<Node>{}));
        }
        const Node* child(std::string_view, identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* upd_child(size_t pos, identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_child(pos, identity<Node>{}));
        }
        Node* upd_child(size_t, identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived* upd_child(std::string_view child_name, identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_child(child_name, identity<Node>{}));
        }
        Node* upd_child(std::string_view child_name, identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived& add_child(std::unique_ptr<TDerived> node_ptr, identity<TDerived>)
        {
            TDerived& rv = *node_ptr;
            add_child(std::move(node_ptr), identity<Node>{});
            return rv;
        }
        Node& add_child(std::unique_ptr<Node>, identity<Node>);

        template<std::derived_from<Node> TDerived>
        const TDerived* find(const NodePath& node_path, identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(find(node_path, identity<Node>{}));
        }
        const Node* find(const NodePath&, identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* find_mut(const NodePath& node_path, identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(find_mut(node_path, identity<Node>{}));
        }
        Node* find_mut(const NodePath&, identity<Node>);

        // lifetime
        // lifetimed ptr to parent
        // children
    };
}
