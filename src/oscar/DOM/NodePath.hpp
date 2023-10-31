#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace osc
{
    // a pre-parsed tree path
    //
    // inspired by:
    //
    // - godot's `NodePath`
    // - OpenSim's `ComponentPath`
    class NodePath final {
    public:
        static constexpr std::string_view::value_type separator = '/';

        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = std::string_view;
            using pointer = std::string_view const*;
            using reference = std::string_view const&;
            using iterator_category = std::forward_iterator_tag;

            reference operator*() const
            {
                return m_Current;
            }

            pointer operator->() const
            {
                return &m_Current;
            }

            Iterator& operator++()
            {
                m_Current = m_Remaining.substr(0, m_Remaining.find(separator));
                m_Remaining = m_Current.size() == m_Remaining.size() ? std::string_view{} : m_Remaining.substr(m_Current.size()+1);
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy = *this;
                ++(*this);
                return copy;
            }

            friend bool operator==(Iterator const& lhs, Iterator const& rhs)
            {
                return lhs.m_Current == rhs.m_Current && lhs.m_Remaining == rhs.m_Remaining;
            }

            friend bool operator!=(Iterator const& lhs, Iterator const& rhs)
            {
                return !(lhs == rhs);
            }

        private:
            friend class NodePath;

            Iterator() = default;

            Iterator(std::string_view path) :
                m_Current{path.substr(0, path.find(separator))},
                m_Remaining{m_Current.size() == path.size() ? std::string_view{} : path.substr(m_Current.size()+1)}
            {
            }

            std::string_view m_Current;
            std::string_view m_Remaining;
        };

        using value_type = std::string_view;
        using iterator = Iterator;
        using const_iterator = Iterator;

        NodePath() = default;
        explicit NodePath(std::string_view);

        [[nodiscard]] bool empty() const
        {
            return m_ParsedPath.empty();
        }

        bool isAbsolute() const
        {
            return !m_ParsedPath.empty() && m_ParsedPath.front() == separator;
        }

        operator std::string_view () const
        {
            return m_ParsedPath;
        }

        Iterator begin() const
        {
            return Iterator{isAbsolute() ? std::string_view{m_ParsedPath}.substr(1) : m_ParsedPath};
        }

        Iterator end() const
        {
            return Iterator{};
        }

        friend bool operator==(NodePath const& lhs, NodePath const& rhs)
        {
            return lhs.m_ParsedPath == rhs.m_ParsedPath;
        }

        friend bool operator!=(NodePath const& lhs, NodePath const& rhs)
        {
            return lhs.m_ParsedPath != rhs.m_ParsedPath;
        }
    private:
        std::string m_ParsedPath;
    };

    namespace literals
    {
        inline NodePath operator""_np(char const* s, size_t len)
        {
            return NodePath{std::string_view(s, len)};
        }
    }
}

template<>
struct std::hash<osc::NodePath> final {
    size_t operator()(osc::NodePath const& np) const
    {
        return std::hash<std::string_view>{}(np);
    }
};
