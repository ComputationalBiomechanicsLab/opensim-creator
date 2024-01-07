#pragma once

#include <oscar/Utils/NullStreambuf.hpp>

#include <cstddef>
#include <iostream>

namespace osc
{
    class NullOStream final : public std::ostream {
    public:
        NullOStream() : std::ostream{&m_Streambuf}
        {
        }

        size_t numCharsWritten() const
        {
            return m_Streambuf.numCharsWritten();
        }

        bool wasWrittenTo() const
        {
            return m_Streambuf.wasWrittenTo();
        }
    private:
        NullStreambuf m_Streambuf;
    };
}
