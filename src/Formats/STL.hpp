#pragma once

#include <functional>
#include <iosfwd>

namespace osc { class Mesh; }

namespace osc
{
    class StlWriter final {
    public:
        explicit StlWriter(std::ostream& outputStream_) :
            m_OutputStream(outputStream_)
        {
        }
        StlWriter(StlWriter const&) = delete;
        StlWriter(StlWriter&&) noexcept = default;
        StlWriter& operator=(StlWriter const&) = delete;
        StlWriter& operator=(StlWriter&&) noexcept = default;
        ~StlWriter() noexcept = default;

        void write(Mesh const&);

    private:
        std::reference_wrapper<std::ostream> m_OutputStream;
    };
}