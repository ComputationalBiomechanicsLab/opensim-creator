#include "Assertions.hpp"

#include "oscar/Platform/Log.hpp"

#include <exception>
#include <sstream>
#include <stdexcept>

void osc::OnAssertionFailure(
    CStringView failingCode,
    CStringView func,
    CStringView file,
    unsigned int line) noexcept
{
    std::string const msg = [&]()
    {
        std::stringstream ss;
        ss << file << ':' << func << ':' << ':' << line << ": assert(" << failingCode << ") failed";
        return std::move(ss).str();
    }();

    log::error("%s", msg.c_str());
    std::terminate();
}

void osc::OnThrowingAssertionFailure(
    CStringView failingCode,
    CStringView func,
    CStringView file,
    unsigned int line)
{
    std::string const msg = [&]()
    {
        std::stringstream ss;
        ss << file << ':' << func << ':' << ':' << line << ": throw_if_not(" << failingCode << ") failed";
        return std::move(ss).str();
    }();

    log::error("%s", msg.c_str());
    throw std::runtime_error{msg};
}
