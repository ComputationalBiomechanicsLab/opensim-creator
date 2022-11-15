#pragma once

#include <functional>
#include <iosfwd>

namespace osc { class Mesh; }

namespace osc
{
    class ObjWriter final {
    public:
        explicit ObjWriter(std::ostream& outputStream_) :
            m_OutputStream{outputStream_}
        {
        }
        ObjWriter(ObjWriter const&) = delete;
        ObjWriter(ObjWriter&&) noexcept = default;
        ObjWriter& operator=(ObjWriter const&) = delete;
        ObjWriter& operator=(ObjWriter&&) noexcept = default;
        ~ObjWriter() noexcept = default;

        void write(Mesh const&);

    private:
        std::reference_wrapper<std::ostream> m_OutputStream;
    };
}