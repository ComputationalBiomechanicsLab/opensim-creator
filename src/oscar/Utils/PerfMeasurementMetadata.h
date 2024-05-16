#pragma once

#include <oscar/Utils/CStringView.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace osc
{
    class PerfMeasurementMetadata final {
    public:
        PerfMeasurementMetadata(
            size_t id,
            std::string_view label,
            std::string_view filename,
            unsigned int line) :

            id_{id},
            label_{label},
            filename_{filename},
            line_{line}
        {}

        size_t id() const { return id_; }
        CStringView label() const { return label_; }
        CStringView filename() const { return filename_; }
        unsigned int line() const { return line_; }

    private:
        size_t id_;
        std::string label_;
        std::string filename_;
        unsigned int line_;
    };
}
