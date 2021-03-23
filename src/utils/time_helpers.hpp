#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace osmv {
    static std::string format_as_time_and_date(std::chrono::system_clock::time_point const& tp) {
        auto in_time_t = std::chrono::system_clock::to_time_t(tp);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }
}
