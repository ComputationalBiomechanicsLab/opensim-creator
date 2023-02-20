#pragma once

#include <iosfwd>
#include <memory>
#include <utility>

namespace osc { class Mesh; }

namespace osc
{
    class StlWriter final {
    public:
        explicit StlWriter(std::shared_ptr<std::ostream> outputStream_) :
            m_OutputStream(std::move(outputStream_))
        {
        }
        StlWriter(StlWriter const&) = delete;
        StlWriter(StlWriter&&) noexcept = default;
        StlWriter& operator=(StlWriter const&) = delete;
        StlWriter& operator=(StlWriter&&) noexcept = default;
        ~StlWriter() noexcept = default;

        void write(Mesh const&);

    private:
        std::shared_ptr<std::ostream> m_OutputStream;
    };
}