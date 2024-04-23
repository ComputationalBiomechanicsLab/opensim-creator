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
            using pointer = const std::string_view*;
            using reference = const std::string_view&;
            using iterator_category = std::forward_iterator_tag;

            reference operator*() const { return current_; }

            pointer operator->() const { return &current_; }

            Iterator& operator++()
            {
                current_ = remaining_.substr(0, remaining_.find(separator));
                remaining_ = current_.size() == remaining_.size() ? std::string_view{} : remaining_.substr(current_.size()+1);
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy = *this;
                ++(*this);
                return copy;
            }

            friend bool operator==(const Iterator&, const Iterator&) = default;

        private:
            friend class NodePath;

            Iterator() = default;

            Iterator(std::string_view path) :
                current_{path.substr(0, path.find(separator))},
                remaining_{current_.size() == path.size() ? std::string_view{} : path.substr(current_.size()+1)}
            {}

            std::string_view current_;
            std::string_view remaining_;
        };

        using value_type = std::string_view;
        using iterator = Iterator;
        using const_iterator = Iterator;

        NodePath() = default;
        explicit NodePath(std::string_view);

        [[nodiscard]] bool empty() const
        {
            return parsed_path_.empty();
        }

        bool is_absolute() const
        {
            return !parsed_path_.empty() and parsed_path_.front() == separator;
        }

        operator std::string_view () const
        {
            return parsed_path_;
        }

        Iterator begin() const
        {
            return Iterator{is_absolute() ? std::string_view{parsed_path_}.substr(1) : parsed_path_};
        }

        Iterator end() const
        {
            return Iterator{};
        }

        friend bool operator==(const NodePath&, const NodePath&) = default;
    private:
        std::string parsed_path_;
    };
}

template<>
struct std::hash<osc::NodePath> final {
    size_t operator()(const osc::NodePath& node_path) const
    {
        return std::hash<std::string_view>{}(node_path);
    }
};
