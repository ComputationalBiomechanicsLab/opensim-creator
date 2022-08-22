#include "CSV.hpp"

#include "src/Utils/Assertions.hpp"

#include <iostream>
#include <utility>

class osc::CSVReader::Impl final {
public:
    Impl(std::istream& input) : m_Input{&input}
    {
        OSC_ASSERT(m_Input != nullptr);
    }

    std::optional<std::vector<std::string>> next()
    {
        std::istream& in = *m_Input;
        std::vector<std::string> cols;
        std::string s;
        bool insideQuotes = false;

        while (!in.bad())
        {
            int c = in.get();

            if (c == std::istream::traits_type::eof())
            {
                // EOF
                cols.push_back(s);
                break;
            }
            else if (c == '\n' && !insideQuotes)
            {
                // standard newline
                cols.push_back(s);
                break;
            }
            else if (c == '\r' && in.peek() == '\n' && !insideQuotes)
            {
                // windows newline

                in.get();  // skip the \n
                cols.push_back(s);
                break;
            }
            else if (c == '"' && s.empty() && !insideQuotes)
            {
                // quote at beginning of quoted column
                insideQuotes = true;
                continue;
            }
            else if (c == '"' && in.peek() == '"')
            {
                // escaped quote

                in.get();  // skip the second '"'
                s += '"';
                continue;
            }
            else if (c == '"' && insideQuotes)
            {
                // quote at end of of quoted column
                insideQuotes = false;
                continue;
            }
            else if (c == ',' && !insideQuotes)
            {
                // comma delimiter at end of column

                cols.push_back(s);
                s.clear();
                continue;
            }
            else
            {
                // normal text
                s += static_cast<char>(c);
                continue;
            }
        }

        if (!cols.empty())
        {
            return cols;
        }
        else
        {
            return std::nullopt;
        }
    }

private:
    std::istream* m_Input;
};


// public API (PIMPL)

osc::CSVReader::CSVReader(std::istream& input) :
    m_Impl{new Impl{input}}
{
}

osc::CSVReader::CSVReader(CSVReader&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CSVReader& osc::CSVReader::operator=(CSVReader&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::CSVReader::~CSVReader() noexcept
{
    delete m_Impl;
}

std::optional<std::vector<std::string>> osc::CSVReader::next()
{
    return m_Impl->next();
}