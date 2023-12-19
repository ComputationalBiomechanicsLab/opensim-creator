#pragma once

#include <oscar/Utils/Assertions.hpp>

namespace osc
{
    class VertexAttributeComponentCount final {
    public:
        static constexpr size_t min() { return 1; }
        static constexpr size_t max() { return 4; }

        constexpr VertexAttributeComponentCount(size_t value_) :
            m_Value{static_cast<int>(value_)}
        {
            OSC_ASSERT_ALWAYS(min() <= m_Value && m_Value < max() && "Vertex component count must be 1, 2, 3, or 4");
        }

        friend bool operator==(VertexAttributeComponentCount, VertexAttributeComponentCount) = default;

        constexpr size_t get() const
        {
            return static_cast<size_t>(m_Value);
        }

        constexpr int toInt() const
        {
            return m_Value;
        }
    private:
        int m_Value;
    };
}
