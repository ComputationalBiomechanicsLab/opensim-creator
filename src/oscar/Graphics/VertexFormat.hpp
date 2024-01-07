#pragma once

#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeDescriptor.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>

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
            VertexAttributeLayout(VertexAttributeDescriptor const& description_, size_t offset_) :
                VertexAttributeDescriptor{description_},
                m_Offset{offset_}
            {
            }

            friend bool operator==(VertexAttributeLayout const&, VertexAttributeLayout const&) = default;

            size_t offset() const { return m_Offset; }
        private:
            size_t m_Offset;
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
            {
            }

            friend bool operator==(VertexAttributeLayoutIterator const& lhs, VertexAttributeLayoutIterator const& rhs)
            {
                return lhs.m_Iter == rhs.m_Iter;
            }

            VertexAttributeLayoutIterator& operator++()
            {
                m_Offset += m_Iter->stride();
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
                return VertexAttributeLayout{*m_Iter, m_Offset};
            }
        private:
            size_t m_Offset = 0;
            std::vector<VertexAttributeDescriptor>::const_iterator m_Iter;
        };

        // C++20 ranges support for `VertexAttributeLayoutIterator`
        class VertexAttributeLayoutRange final {
        public:
            explicit VertexAttributeLayoutRange(
                std::vector<VertexAttributeDescriptor> const& descriptions_) :
                m_Descriptions{&descriptions_}
            {
            }

            VertexAttributeLayoutIterator begin() const
            {
                return VertexAttributeLayoutIterator{m_Descriptions->begin()};
            }

            VertexAttributeLayoutIterator end() const
            {
                return VertexAttributeLayoutIterator{m_Descriptions->end()};
            }
        private:
            std::vector<VertexAttributeDescriptor> const* m_Descriptions;
        };

        // constructs an "empty" format
        VertexFormat() = default;

        // attribute descriptions must:
        //
        // - have unique `VertexAttribute`s
        // - be provided in the same order as `VertexAttribute` (e.g. `Position`, then `Normal`, then `Color`, etc.)
        VertexFormat(std::initializer_list<VertexAttributeDescriptor>);

        friend bool operator==(VertexFormat const&, VertexFormat const&) = default;

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
            for (auto const& desc : m_AttributeDescriptions)
            {
                if (desc.attribute() == attr)
                {
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
            for (VertexAttributeLayout l : attributeLayouts())
            {
                if (l.attribute() == attr)
                {
                    return l;
                }
            }
            return std::nullopt;
        }

        size_t stride() const
        {
            return m_Stride;
        }

        void insert(VertexAttributeDescriptor const&);
        void erase(VertexAttribute);
    private:
        size_t computeStride() const;

        std::vector<VertexAttributeDescriptor> m_AttributeDescriptions;
        size_t m_Stride = 0;
    };
}
