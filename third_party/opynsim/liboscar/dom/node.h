#pragma once

#include <liboscar/dom/node_path.h>
#include <liboscar/dom/object.h>
#include <liboscar/utilities/c_string_view.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace osc
{
    class PropertyDescriptions;
    class Variant;

    class Node : public Object {
    public:
        ~Node() noexcept override = default;

    protected:
        Node();
        Node(const Node&);
        Node(Node&&) noexcept;
        Node& operator=(const Node&);
        Node& operator=(Node&&) noexcept;

    public:
        std::unique_ptr<Node> clone() const
        {
            return std::unique_ptr<Node>{
                dynamic_cast<Node*>(static_cast<const Object&>(*this).clone().release())};
        }

        CStringView name() const;
        void set_name(std::string_view);

        template<std::derived_from<Node> TNode = Node>
        const TNode* parent() const
        {
            return parent(Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_parent()
        {
            return upd_parent(Identity<TNode>{});
        }

        size_t num_children() const;

        template<std::derived_from<Node> TNode = Node>
        const TNode* child(size_t pos) const
        {
            return child(pos, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        const TNode* child(std::string_view child_name) const
        {
            return child(child_name, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_child(size_t pos)
        {
            return upd_child(pos, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* upd_child(std::string_view child_name)
        {
            return upd_child(child_name, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode& add_child(std::unique_ptr<TNode> node_ptr)
        {
            return add_child(std::move(node_ptr), Identity<TNode>{});
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
            return find(node_path, Identity<TNode>{});
        }

        template<std::derived_from<Node> TNode = Node>
        TNode* find_mut(const NodePath& node_path)
        {
            return dynamic_cast<TNode*>(find_mut(node_path, Identity<TNode>{}));
        }

    private:
        // You might be (rightly) wondering why this implementation goes through the
        // bother of using `Identity<T>` classes to distinguish overloads etc. rather
        // than just specializing a template function.
        //
        // It's because standard C++ doesn't allow template specialization on class
        // member functions. See: https://stackoverflow.com/a/3057522

        template<typename T>
        struct Identity {
            using type = T;
        };

        template<std::derived_from<Node> TDerived>
        const TDerived* parent(Identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(parent(Identity<Node>{}));
        }
        const Node* parent(Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* upd_parent(Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_parent(Identity<Node>{}));
        }
        Node* upd_parent(Identity<Node>);

        template<std::derived_from<Node> TDerived>
        const TDerived* child(size_t pos, Identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(child(pos, Identity<Node>{}));
        }
        const Node* child(size_t, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        const TDerived* child(std::string_view child_name, Identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(child(child_name, Identity<Node>{}));
        }
        const Node* child(std::string_view, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* upd_child(size_t pos, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_child(pos, Identity<Node>{}));
        }
        Node* upd_child(size_t, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived* upd_child(std::string_view child_name, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(upd_child(child_name, Identity<Node>{}));
        }
        Node* upd_child(std::string_view child_name, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        TDerived& add_child(std::unique_ptr<TDerived> node_ptr, Identity<TDerived>)
        {
            TDerived& rv = *node_ptr;
            add_child(std::move(node_ptr), Identity<Node>{});
            return rv;
        }
        Node& add_child(std::unique_ptr<Node>, Identity<Node>);

        template<std::derived_from<Node> TDerived>
        const TDerived* find(const NodePath& node_path, Identity<TDerived>) const
        {
            return dynamic_cast<const TDerived*>(find(node_path, Identity<Node>{}));
        }
        const Node* find(const NodePath&, Identity<Node>) const;

        template<std::derived_from<Node> TDerived>
        TDerived* find_mut(const NodePath& node_path, Identity<TDerived>)
        {
            return dynamic_cast<TDerived*>(find_mut(node_path, Identity<Node>{}));
        }
        Node* find_mut(const NodePath&, Identity<Node>);

        // lifetime
        // lifetimed ptr to parent
        // children
    };
}
