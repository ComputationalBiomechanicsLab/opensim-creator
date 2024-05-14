#include "CSV.h"

#include <array>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr auto c_special_csv_chars = std::to_array({ ',', '\r', '\n', '"'});

    constexpr bool should_be_quoted(std::string_view str)
    {
        return rgs::find_first_of(str, c_special_csv_chars) != str.end();
    }
}

std::optional<std::vector<std::string>> osc::read_csv_row(
    std::istream& in)
{
    std::optional<std::vector<std::string>> columns;
    columns.emplace();

    if (not read_csv_row_into_vector(in, *columns)) {
        columns.reset();
    }
    return columns;
}

bool osc::read_csv_row_into_vector(
    std::istream& in,
    std::vector<std::string>& r_columns)
{
    if (in.eof()) {
        return false;
    }

    std::vector<std::string> columns;
    std::string str;
    bool inside_quotes = false;

    while (not in.bad()) {
        const auto c = in.get();

        if (c == std::istream::traits_type::eof()) {
            // EOF
            columns.push_back(str);
            break;
        }
        else if (c == '\n' and not inside_quotes) {
            // standard newline
            columns.push_back(str);
            break;
        }
        else if (c == '\r' and in.peek() == '\n' and not inside_quotes) {
            // windows newline

            in.get();  // skip the \n
            columns.push_back(str);
            break;
        }
        else if (c == '"' and str.empty() and not inside_quotes) {
            // quote at beginning of quoted column
            inside_quotes = true;
            continue;
        }
        else if (c == '"' and in.peek() == '"') {
            // escaped quote

            in.get();  // skip the second '"'
            str += '"';
            continue;
        }
        else if (c == '"' and inside_quotes) {
            // quote at end of of quoted column
            inside_quotes = false;
            continue;
        }
        else if (c == ',' and not inside_quotes) {
            // comma delimiter at end of column

            columns.push_back(str);
            str.clear();
            continue;
        }
        else {
            // normal text
            str += static_cast<std::string::value_type>(c);
            continue;
        }
    }

    if (not columns.empty()) {
        r_columns = std::move(columns);
        return true;
    }
    else {
        return false;
    }
}

void osc::write_csv_row(
    std::ostream& out,
    std::span<const std::string> columns)
{
    std::string_view delimeter;
    for (const auto& column : columns) {
        const bool quoted = should_be_quoted(column);

        out << delimeter;
        if (quoted) {
            out << '"';
        }

        for (auto c : column) {
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

        delimeter = ",";
    }
    out << '\n';
}
