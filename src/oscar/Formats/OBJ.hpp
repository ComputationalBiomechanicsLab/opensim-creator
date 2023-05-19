#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <utility>

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
        explicit ObjWriter(std::shared_ptr<std::ostream> outputStream_) :
            m_OutputStream{std::move(outputStream_)}
        {
        }
        ObjWriter(ObjWriter const&) = delete;
        ObjWriter(ObjWriter&&) noexcept = default;
        ObjWriter& operator=(ObjWriter const&) = delete;
        ObjWriter& operator=(ObjWriter&&) noexcept = default;
        ~ObjWriter() noexcept = default;

        void write(Mesh const&, ObjWriterFlags = ObjWriterFlags_Default);

    private:
        std::shared_ptr<std::ostream> m_OutputStream;
    };
}