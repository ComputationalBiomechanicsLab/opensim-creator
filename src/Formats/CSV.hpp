#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace osc
{
    class CSVReader final {
    public:
        explicit CSVReader(std::istream&);
        CSVReader(CSVReader const&) = delete;
        CSVReader(CSVReader&&) noexcept;
        CSVReader& operator=(CSVReader const&) = delete;
        CSVReader& operator=(CSVReader&&) noexcept;
        ~CSVReader() noexcept;

        std::optional<std::vector<std::string>> next();

    private:
        class Impl;
        Impl* m_Impl;
    };
}