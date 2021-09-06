#include "Algorithms.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

int osc::NumBitsSetIn(int v) noexcept {
    unsigned uv = static_cast<unsigned>(v);
    unsigned i = 0;
    while (uv) {
        uv &= (uv - 1);
        ++i;
    }
    return static_cast<int>(i);
}

int osc::LeastSignificantBitIndex(int v) noexcept {
    unsigned uv = static_cast<unsigned>(v);
    unsigned i = 0;
    while (!(uv & 0x1)) {
        uv >>= 1;
        ++i;
    }
    return static_cast<int>(i);
}

bool osc::IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b) noexcept {
    // this is a more verbose implementation of:
    //
    //     https://stackoverflow.com/questions/33379846/case-insensitive-sorting-of-an-array-of-strings

    auto [itA, itB] = std::mismatch(a.begin(), a.end(), b.begin(), b.end(), [](auto c1, auto c2) {
        return std::tolower(c1) == std::tolower(c2);
    });

    if (itB == b.end()) {
        return false;  // b is not greater than a (they are equal)
    }

    if (itA == a.end()) {
        return true;  // b is greater than a (common prefix, but has more letters)
    }

    if (tolower(*itA) < tolower(*itB)) {
        return true;  // b is greater than a (first mismatching character is greater)
    } else {
        return false;  // a is greater than b (first mismatching character is less)
    }
}


// returns true if `b` is lexographically greater than `a`, ignoring case
//
// e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
bool osc::IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2) noexcept {
    return IsStringCaseInsensitiveGreaterThan(p1.filename().string(), p2.filename().string());
}
