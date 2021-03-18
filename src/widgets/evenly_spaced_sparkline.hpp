#pragma once

#include <imgui.h>

#include <array>
#include <cstddef>
#include <limits>

namespace osmv {

    // holds a fixed number of Y datapoints that are assumed to be roughly evenly spaced in X
    //
    // if the number of datapoints "pushed" onto the sparkline exceeds the (fixed) capacity then
    // the datapoints will be halved (reducing resolution) to make room for more, which is how
    // it guarantees constant size
    template<size_t MaxDatapoints = 256>
    struct Evenly_spaced_sparkline final {
        static constexpr float min_x_step = 0.001f;
        static_assert(MaxDatapoints % 2 == 0, "num datapoints must be even because the impl uses integer division");

        std::array<float, MaxDatapoints> data;
        size_t n = 0;
        float x_step = min_x_step;
        float latest_x = -min_x_step;
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();

        constexpr Evenly_spaced_sparkline() = default;

        // reset the data, but not the output being monitored
        void clear() {
            n = 0;
            x_step = min_x_step;
            latest_x = -min_x_step;
            min = std::numeric_limits<float>::max();
            max = std::numeric_limits<float>::min();
        }

        void push_datapoint(float x, float y) {
            if (x < latest_x + x_step) {
                return;  // too close to previous datapoint: do not record
            }

            if (n == MaxDatapoints) {
                // too many datapoints recorded: half the resolution of the
                // sparkline to accomodate more datapoints being added
                size_t halfway = n / 2;
                for (size_t i = 0; i < halfway; ++i) {
                    size_t first = 2 * i;
                    data[i] = (data[first] + data[first + 1]) / 2.0f;
                }
                n = halfway;
                x_step *= 2.0f;
            }

            data[n++] = y;
            latest_x = x;
            min = std::min(min, y);
            max = std::max(max, y);
        }

        void draw(float height = 60.0f) const {
            ImGui::PlotLines(
                "",
                data.data(),
                static_cast<int>(n),
                0,
                nullptr,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::max(),
                ImVec2(0, height));
        }

        float last_datapoint() const {
            OSMV_ASSERT(n > 0);
            return data[n - 1];
        }
    };
}
