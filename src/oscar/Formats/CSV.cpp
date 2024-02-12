#include "CSV.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

// helpers
namespace
{
    bool IsSpecialCSVCharacter(std::string_view::value_type c)
    {
        return c == ',' || c == '\r' || c == '\n' || c == '"';
    }

    bool ShouldBeQuoted(std::string_view v)
    {
        return std::any_of(v.begin(), v.end(), IsSpecialCSVCharacter);
    }
}

// public API

std::optional<std::vector<std::string>> osc::ReadCSVRow(
    std::istream& in)
{
    std::optional<std::vector<std::string>> cols;
    cols.emplace();

    if (!ReadCSVRowIntoVector(in, *cols))
    {
        cols.reset();
    }
    return cols;
}

bool osc::ReadCSVRowIntoVector(
    std::istream& in,
    std::vector<std::string>& out)
{
    if (in.eof())
    {
        return false;
    }

    std::vector<std::string> cols;
    std::string s;
    bool insideQuotes = false;

    while (!in.bad())
    {
        auto const c = in.get();

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
            s += static_cast<std::string::value_type>(c);
            continue;
        }
    }

    if (!cols.empty())
    {
        out = std::move(cols);
        return true;
    }
    else
    {
        return false;
    }
}

void osc::WriteCSVRow(
    std::ostream& out,
    std::span<std::string const> columns)
{
    std::string_view delim;
    for (std::string const& column : columns)
    {
        bool const quoted = ShouldBeQuoted(column);

        out << delim;
        if (quoted)
        {
            out << '"';
        }

        for (std::string::value_type c : column)
        {
            if (c != '"')
            {
                out << c;
            }
            else
            {
                out << '"' << '"';
            }
        }

        if (quoted)
        {
            out << '"';
        }

        delim = ",";
    }
    out << '\n';
}
