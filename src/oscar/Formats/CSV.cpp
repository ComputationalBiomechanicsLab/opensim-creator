#include "CSV.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

// helpers
namespace
{
    constexpr bool is_special_csv_char(std::string_view::value_type c)
    {
        return c == ',' or c == '\r' or c == '\n' or c == '"';
    }

    constexpr bool should_be_quoted(std::string_view v)
    {
        return rgs::any_of(v, is_special_csv_char);
    }
}

std::optional<std::vector<std::string>> osc::read_csv_row(
    std::istream& in)
{
    std::optional<std::vector<std::string>> cols;
    cols.emplace();

    if (not read_csv_row_into_vector(in, *cols)) {
        cols.reset();
    }
    return cols;
}

bool osc::read_csv_row_into_vector(
    std::istream& in,
    std::vector<std::string>& assigned_columns)
{
    if (in.eof()) {
        return false;
    }

    std::vector<std::string> cols;
    std::string s;
    bool inside_quotes = false;

    while (not in.bad()) {
        const auto c = in.get();

        if (c == std::istream::traits_type::eof()) {
            // EOF
            cols.push_back(s);
            break;
        }
        else if (c == '\n' and not inside_quotes) {
            // standard newline
            cols.push_back(s);
            break;
        }
        else if (c == '\r' and in.peek() == '\n' and not inside_quotes) {
            // windows newline

            in.get();  // skip the \n
            cols.push_back(s);
            break;
        }
        else if (c == '"' and s.empty() and not inside_quotes) {
            // quote at beginning of quoted column
            inside_quotes = true;
            continue;
        }
        else if (c == '"' and in.peek() == '"') {
            // escaped quote

            in.get();  // skip the second '"'
            s += '"';
            continue;
        }
        else if (c == '"' and inside_quotes) {
            // quote at end of of quoted column
            inside_quotes = false;
            continue;
        }
        else if (c == ',' and not inside_quotes) {
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
        assigned_columns = std::move(cols);
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
    std::string_view delim;
    for (const auto& column : columns) {
        const bool quoted = should_be_quoted(column);

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
