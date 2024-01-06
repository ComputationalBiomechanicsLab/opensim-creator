#pragma once

#include <array>
#include <cstddef>
#include <streambuf>

namespace osc
{
    class NullStreambuf final : public std::streambuf {
    public:
        size_t numCharsWritten() const
        {
            return m_NumCharsWritten;
        }

        bool wasWrittenTo() const
        {
            return m_NumCharsWritten > 0;
        }
    protected:
        int overflow(int c) final
        {
            setp(m_DummyBuffer.data(), m_DummyBuffer.data() + m_DummyBuffer.size());
            ++m_NumCharsWritten;
            return (c == traits_type::eof() ? char_type{} : c);
        }

        std::streamsize xsputn(const char_type*, std::streamsize count) final
        {
            m_NumCharsWritten += count;
            return count;
        }
    private:
        std::array<char_type, 1024> m_DummyBuffer;
        size_t m_NumCharsWritten = 0;
    };
}
