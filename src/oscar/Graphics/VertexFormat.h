#pragma once

#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeDescriptor.h>

#include <cstddef>
#include <initializer_list>
#include <optional>
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

            explicit VertexAttributeLayoutIterator(
                std::vector<VertexAttributeDescriptor>::const_iterator iter_) :

                m_Iter{iter_}
            {}

            friend bool operator==(const VertexAttributeLayoutIterator& lhs, const VertexAttributeLayoutIterator& rhs)
            {
                return lhs.m_Iter == rhs.m_Iter;
            }

            VertexAttributeLayoutIterator& operator++()
            {
                offset_ += m_Iter->stride();
                ++m_Iter;
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
                return VertexAttributeLayout{*m_Iter, offset_};
            }
        private:
            size_t offset_ = 0;
            std::vector<VertexAttributeDescriptor>::const_iterator m_Iter;
        };

        // C++20 ranges support for `VertexAttributeLayoutIterator`
        class VertexAttributeLayoutRange final {
        public:
            explicit VertexAttributeLayoutRange(
                const std::vector<VertexAttributeDescriptor>& descriptions) :
                descriptions_{&descriptions}
            {}

            VertexAttributeLayoutIterator begin() const
            {
                return VertexAttributeLayoutIterator{descriptions_->begin()};
            }

            VertexAttributeLayoutIterator end() const
            {
                return VertexAttributeLayoutIterator{descriptions_->end()};
            }
        private:
            const std::vector<VertexAttributeDescriptor>* descriptions_;
        };

        // constructs an "empty" format
        VertexFormat() = default;

        // attribute descriptions must:
        //
        // - have unique `VertexAttribute`s
        // - be provided in the same order as `VertexAttribute` (e.g. `Position`, then `Normal`, then `Color`, etc.)
        VertexFormat(std::initializer_list<VertexAttributeDescriptor>);

        friend bool operator==(const VertexFormat&, const VertexFormat&) = default;

        void clear()
        {
            *this = VertexFormat{};
        }

        [[nodiscard]] bool empty() const
        {
            return m_AttributeDescriptions.empty();
        }

        bool contains(VertexAttribute attr) const
        {
            for (const auto& desc : m_AttributeDescriptions) {
                if (desc.attribute() == attr) {
                    return true;
                }
            }
            return false;
        }

        size_t numAttributes() const
        {
            return m_AttributeDescriptions.size();
        }

        VertexAttributeLayoutRange attributeLayouts() const
        {
            return VertexAttributeLayoutRange{m_AttributeDescriptions};
        }

        std::optional<VertexAttributeLayout> attributeLayout(VertexAttribute attr) const
        {
            for (VertexAttributeLayout l : attributeLayouts()) {
                if (l.attribute() == attr) {
                    return l;
                }
            }
            return std::nullopt;
        }

        size_t stride() const
        {
            return stride_;
        }

        void insert(const VertexAttributeDescriptor&);
        void erase(VertexAttribute);
    private:
        size_t calc_stride() const;

        std::vector<VertexAttributeDescriptor> m_AttributeDescriptions;
        size_t stride_ = 0;
    };
}
