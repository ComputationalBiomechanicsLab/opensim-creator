#pragma once

#include <array>
#include <cstddef>
#include <streambuf>

namespace osc
{
    // a `std::streambuf` that performs no actual input-output operations
    class NullStreambuf final : public std::streambuf {
    public:
        size_t num_chars_written() const
        {
            return num_chars_written_;
        }

        bool was_written_to() const
        {
            return num_chars_written_ > 0;
        }
    protected:
        int overflow(int c) final
        {
            setp(dummy_buffer_.data(), dummy_buffer_.data() + dummy_buffer_.size());
            ++num_chars_written_;
            return (c == traits_type::eof() ? char_type{} : c);
        }

        std::streamsize xsputn(const char_type*, std::streamsize count) final
        {
            num_chars_written_ += count;
            return count;
        }
    private:
        std::array<char_type, 1024> dummy_buffer_;
        size_t num_chars_written_ = 0;
    };
}
