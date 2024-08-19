#pragma once

#include <oscar/Utils/NullStreambuf.h>

#include <cstddef>
#include <ostream>

namespace osc
{
    // a `std::ostream` that performs no actual input-output operations
    class NullOStream final : public std::ostream {
    public:
        NullOStream() : std::ostream{&streambuf_} {}

        size_t num_chars_written() const
        {
            return streambuf_.num_chars_written();
        }

        bool was_written_to() const
        {
            return streambuf_.was_written_to();
        }
    private:
        NullStreambuf streambuf_;
    };
}
