#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>

namespace osc { class Mesh; }

namespace osc
{
    using ObjWriterFlags = int32_t;
    enum ObjWriterFlags_ {
        ObjWriterFlags_None = 0,
        ObjWriterFlags_IgnoreNormals = 1<<0,
        ObjWriterFlags_Default = ObjWriterFlags_None,
    };

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

        void write(Mesh const&, ObjWriterFlags = ObjWriterFlags_Default);

    private:
        std::reference_wrapper<std::ostream> m_OutputStream;
    };
}