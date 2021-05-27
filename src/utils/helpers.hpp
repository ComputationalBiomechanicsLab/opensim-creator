#pragma once

#include <filesystem>
#include <string>
#include <algorithm>

namespace osc {
    inline int num_bits_set_in(int v) {
        unsigned uv = static_cast<unsigned>(v);
        unsigned i = 0;
        while (uv) {
            uv &= (uv - 1);
            ++i;
        }
        return static_cast<int>(i);
    }

    inline int lsb_index(int v) {
        unsigned uv = static_cast<unsigned>(v);
        unsigned i = 0;
        while (!(uv & 0x1)) {
            uv >>= 1;
            ++i;
        }
        return static_cast<int>(i);
    }

    std::string slurp_into_string(std::filesystem::path const&);

    template<typename Str>
    [[nodiscard]] inline bool case_insensitive_gt(Str const& a, Str const& b) {
        // this is a more verbose implementation of:
        //
        //     https://stackoverflow.com/questions/33379846/case-insensitive-sorting-of-an-array-of-strings

        auto [it_a, it_b] = std::mismatch(a.begin(), a.end(), b.begin(), b.end(), [](auto c1, auto c2) {
            return std::tolower(c1) == std::tolower(c2);
        });

        if (it_b == b.end()) {
            return false;  // b is not greater than a (they are equal)
        }

        if (it_a == a.end()) {
            return true;  // b is greater than a (common prefix, but has more letters)
        }

        if (tolower(*it_a) < tolower(*it_b)) {
            return true;  // b is greater than a (first mismatching character is greater)
        } else {
            return false;  // a is greater than b (first mismatching character is less)
        }
    }

    [[nodiscard]] inline bool filename_lexographically_gt(std::filesystem::path const& p1, std::filesystem::path const& p2) {
        // from
        return case_insensitive_gt(p1.filename().string(), p2.filename().string());
    }
}
