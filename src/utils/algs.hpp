#pragma once

#include <algorithm>

namespace osc {

    // remove all elements `e` in `Container` `c` for which `p(e)` returns `true`
    template<typename Container, typename UnaryPredicate>
    void remove_erase(Container& c, UnaryPredicate p) {
        auto it = std::remove_if(c.begin(), c.end(), p);
        c.erase(it, c.end());
    }

    // returns the number of bits set in the input integer
    //
    // e.g. 0x1 --> 1, 0x2 --> 1, 0x3 --> 2, 0xf --> 4
    [[nodiscard]] constexpr inline int num_bits_set_in(int v) noexcept {
        unsigned uv = static_cast<unsigned>(v);
        unsigned i = 0;
        while (uv) {
            uv &= (uv - 1);
            ++i;
        }
        return static_cast<int>(i);
    }

    // returns the bit-index of the least significant bit that it set
    //
    // e.g. 0x1 --> 0, 0x2 --> 1, 0x3 --> 0, 0x4 --> 2
    [[nodiscard]] constexpr inline int lsb_index(int v) noexcept {
        unsigned uv = static_cast<unsigned>(v);
        unsigned i = 0;
        while (!(uv & 0x1)) {
            uv >>= 1;
            ++i;
        }
        return static_cast<int>(i);
    }

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
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

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    [[nodiscard]] inline bool filename_lexographically_gt(std::filesystem::path const& p1, std::filesystem::path const& p2) {
        // from
        return case_insensitive_gt(p1.filename().string(), p2.filename().string());
    }
}
