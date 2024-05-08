#pragma once

#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>

namespace osc
{
    class VertexFormat final {
    public:
        // describes a vertex attribute's layout within a `VertexFormat`
        //
        // i.e. it's a `VertexAttributeDescriptor` that has a known offset within `VertexFormat`
        class VertexAttributeLayout final : public VertexAttributeDescriptor {
        public:
            VertexAttributeLayout(const VertexAttributeDescriptor& descriptor, size_t offset) :
                VertexAttributeDescriptor{descriptor},
                offset_{offset}
            {}

            friend bool operator==(const VertexAttributeLayout&, const VertexAttributeLayout&) = default;

            size_t offset() const { return offset_; }
        private:
            size_t offset_;
        };

        // iterates over each attribute's layout within `VertexFormat`
        class VertexAttributeLayoutIterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = VertexAttributeLayout;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            VertexAttributeLayoutIterator() = default;  // to satisfy `std::ranges::range`

            explicit VertexAttributeLayoutIterator(
                std::vector<VertexAttributeDescriptor>::const_iterator iter) :

                iter_{iter}
            {}

            friend bool operator==(const VertexAttributeLayoutIterator& lhs, const VertexAttributeLayoutIterator& rhs)
            {
                return lhs.iter_ == rhs.iter_;
            }

            VertexAttributeLayoutIterator& operator++()
            {
                offset_ += iter_->stride();
                ++iter_;
                return *this;
            }

            VertexAttributeLayoutIterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            reference operator*() const
            {
                return VertexAttributeLayout{*iter_, offset_};
            }
        private:
            size_t offset_ = 0;
            std::vector<VertexAttributeDescriptor>::const_iterator iter_{};
        };

        // C++20 ranges support for `VertexAttributeLayoutIterator`
        class VertexAttributeLayoutRange final {
        public:
            using value_type = VertexAttributeLayout;
            using iterator = VertexAttributeLayoutIterator;
            using const_iterator = VertexAttributeLayoutIterator;

            explicit VertexAttributeLayoutRange(
                const std::vector<VertexAttributeDescriptor>& descriptions) :
                descriptions_{&descriptions}
            {}

            const_iterator begin() const
            {
                return VertexAttributeLayoutIterator{descriptions_->begin()};
            }

            const_iterator end() const
            {
                return VertexAttributeLayoutIterator{descriptions_->end()};
            }
        private:
            const std::vector<VertexAttributeDescriptor>* descriptions_ = nullptr;
        };

        // constructs an "empty" format
        VertexFormat() = default;

        // attribute descriptions must:
        //
        // - have at least a `VertexAttribute::Position`
        // - all be unique
        explicit VertexFormat(std::span<const VertexAttributeDescriptor>);

        VertexFormat(std::initializer_list<VertexAttributeDescriptor> il) :
            VertexFormat{std::span{il.begin(), il.end()}}
        {}

        friend bool operator==(const VertexFormat&, const VertexFormat&) = default;

        void clear()
        {
            *this = VertexFormat{};
        }

        [[nodiscard]] bool empty() const
        {
            return attribute_descriptions_.empty();
        }

        bool contains(VertexAttribute attribute) const
        {
            return cpp23::contains(attribute_descriptions_, attribute, &VertexAttributeDescriptor::attribute);
        }

        size_t num_attributes() const
        {
            return attribute_descriptions_.size();
        }

        VertexAttributeLayoutRange attribute_layouts() const
        {
            return VertexAttributeLayoutRange{attribute_descriptions_};
        }

        std::optional<VertexAttributeLayout> attribute_layout(VertexAttribute attribute) const
        {
            return find_or_nullopt(attribute_layouts(), attribute, &VertexAttributeLayout::attribute);
        }

        size_t stride() const
        {
            return stride_;
        }

        void insert(const VertexAttributeDescriptor&);
        void erase(VertexAttribute);
    private:
        size_t calc_stride() const;

        std::vector<VertexAttributeDescriptor> attribute_descriptions_;
        size_t stride_ = 0;
    };
}
