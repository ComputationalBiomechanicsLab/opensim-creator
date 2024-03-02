#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace osc
{
    class PerfMeasurementMetadata final {
    public:
        PerfMeasurementMetadata(
            size_t id_,
            std::string_view label_,
            std::string_view filename_,
            unsigned int fileLine_) :

            m_ID{id_},
            m_Label{label_},
            m_Filename{filename_},
            m_FileLine{fileLine_}
        {
        }

        size_t getID() const
        {
            return m_ID;
        }

        std::string const& getLabel() const
        {
            return m_Label;
        }

        std::string const& getFilename() const
        {
            return m_Filename;
        }

        unsigned int getLine() const
        {
            return m_FileLine;
        }
    private:
        size_t m_ID;
        std::string m_Label;
        std::string m_Filename;
        unsigned int m_FileLine;
    };
}
