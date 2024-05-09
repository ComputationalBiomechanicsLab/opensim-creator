#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/PerfClock.h>
#include <oscar/Utils/PerfMeasurementMetadata.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace osc
{
    class PerfMeasurement final {
    public:
        explicit PerfMeasurement(
            const std::shared_ptr<const PerfMeasurementMetadata>& metadata) :
            metadata_{metadata}
        {}

        size_t id() const { return metadata_->id(); }

        CStringView label() const { return metadata_->label(); }

        CStringView filename() const { return metadata_->filename(); }

        unsigned int line() const { return metadata_->line(); }

        size_t call_count() const { return call_count_; }

        PerfClock::duration last_duration() const { return last_duration_; }

        PerfClock::duration average_duration() const
        {
            return call_count_ > 0 ? total_duration_/static_cast<ptrdiff_t>(call_count_) : PerfClock::duration{0};
        }

        PerfClock::duration total_duration() const { return total_duration_; }

        void submit(PerfClock::time_point start, PerfClock::time_point end)
        {
            last_duration_ = end - start;
            total_duration_ += last_duration_;
            call_count_++;
        }

        void clear()
        {
            call_count_ = 0;
            total_duration_ = PerfClock::duration{0};
            last_duration_ = PerfClock::duration{0};
        }

    private:
        std::shared_ptr<const PerfMeasurementMetadata> metadata_;
        size_t call_count_ = 0;
        PerfClock::duration total_duration_{0};
        PerfClock::duration last_duration_{0};
    };
}
