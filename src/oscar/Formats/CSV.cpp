#include "CSV.h"

#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

// helpers
namespace
{
    constexpr bool isSpecialCSVCharacter(std::string_view::value_type c)
    {
        return c == ',' or c == '\r' or c == '\n' or c == '"';
    }

    constexpr bool shouldBeQuoted(std::string_view v)
    {
        return any_of(v, isSpecialCSVCharacter);
    }
}

// public API

std::optional<std::vector<std::string>> osc::readCSVRow(
    std::istream& in)
{
    std::optional<std::vector<std::string>> cols;
    cols.emplace();

    if (!readCSVRowIntoVector(in, *cols)) {
        cols.reset();
    }
    return cols;
}

bool osc::readCSVRowIntoVector(
    std::istream& in,
    std::vector<std::string>& out)
{
    if (in.eof()) {
        return false;
    }

    std::vector<std::string> cols;
    std::string s;
    bool insideQuotes = false;

    while (!in.bad()) {
        const auto c = in.get();

        if (c == std::istream::traits_type::eof()) {
            // EOF
            cols.push_back(s);
            break;
        }
        else if (c == '\n' and not insideQuotes) {
            // standard newline
            cols.push_back(s);
            break;
        }
        else if (c == '\r' and in.peek() == '\n' and not insideQuotes) {
            // windows newline

            in.get();  // skip the \n
            cols.push_back(s);
            break;
        }
        else if (c == '"' and s.empty() and not insideQuotes) {
            // quote at beginning of quoted column
            insideQuotes = true;
            continue;
        }
        else if (c == '"' and in.peek() == '"') {
            // escaped quote

            in.get();  // skip the second '"'
            s += '"';
            continue;
        }
        else if (c == '"' and insideQuotes) {
            // quote at end of of quoted column
            insideQuotes = false;
            continue;
        }
        else if (c == ',' and not insideQuotes) {
            // comma delimiter at end of column

            cols.push_back(s);
            s.clear();
            continue;
        }
        else {
            // normal text
            s += static_cast<std::string::value_type>(c);
            continue;
        }
    }

    if (not cols.empty()) {
        out = std::move(cols);
        return true;
    }
    else {
        return false;
    }
}

void osc::writeCSVRow(
    std::ostream& out,
    std::span<const std::string> columns)
{
    std::string_view delim;
    for (const auto& column : columns) {
        const bool quoted = shouldBeQuoted(column);

        out << delim;
        if (quoted) {
            out << '"';
        }

        for (std::string::value_type c : column) {
            if (c != '"') {
                out << c;
            }
            else {
                out << '"' << '"';
            }
        }

        if (quoted) {
            out << '"';
        }

        delim = ",";
    }
    out << '\n';
}
