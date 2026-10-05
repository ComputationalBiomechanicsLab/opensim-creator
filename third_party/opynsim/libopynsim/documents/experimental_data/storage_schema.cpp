#include "storage_schema.h"

#include <OpenSim/Common/Storage.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    // Describes how a pattern of individual column headers in the source data could
    // be used to materialize a series of datapoints of type `DataPointType`.
    class DataSeriesPattern final {
    public:

        // Returns a `DataSeriesPattern` for the given `DataType`.
        template<DataPointType DataType, typename... ColumnHeaderStrings>
        requires
            (sizeof...(ColumnHeaderStrings) == num_elements_in(DataType)) and
            (std::constructible_from<osc::CStringView, ColumnHeaderStrings> && ...)
            static DataSeriesPattern for_datatype(ColumnHeaderStrings&&... header_suffixes)
        {
            return DataSeriesPattern{DataType, std::initializer_list<osc::CStringView>{osc::CStringView{std::forward<ColumnHeaderStrings>(header_suffixes)}...}};  // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        }

        // Returns the `DataPointType` matched by this pattern.
        DataPointType datatype() const { return data_point_type_; }

        // Returns `true` if the given column headers match this pattern.
        bool matches(std::span<const std::string> headers) const
        {
            if (headers.size() < header_suffixes_.size()) {
                return false;
            }
            for (size_t i = 0; i < header_suffixes_.size(); ++i) {
                if (not headers[i].ends_with(header_suffixes_[i])) {
                    return false;
                }
            }
            return true;
        }

        // If the given `columnHeader` matches a suffix in this pattern, returns a substring view
        // of the provided string view, minus the suffix. Otherwise, returns the provided string
        // view.
        std::string_view remove_suffix(std::string_view colum_header) const
        {
            for (const auto& suffix : header_suffixes_) {
                if (colum_header.ends_with(suffix)) {
                    return colum_header.substr(0, colum_header.size() - suffix.size());
                }
            }
            return colum_header;  // couldn't remove it
        }
    private:
        DataSeriesPattern(DataPointType type, std::initializer_list<osc::CStringView> header_suffxes) :
            data_point_type_{type},
            header_suffixes_{header_suffxes}
        {}

        DataPointType data_point_type_;
        std::vector<osc::CStringView> header_suffixes_;
    };

    // Describes a collection of patterns that _might_ match against the column headers
    // of the source data.
    //
    // Note: These patterns are based on how OpenSim 4.5 matches data in the 'Preview
    //       Experimental Data' part of the official OpenSim GUI.
    class DataSeriesPatterns final {
    public:

        // If the given headers matches a pattern, returns a pointer to the pattern. Otherwise,
        // returns `nullptr`.
        const DataSeriesPattern* try_match(std::span<const std::string> headers) const
        {
            const auto it = rgs::find_if(patterns_, [&headers](const auto& pattern) { return pattern.matches(headers); });
            return it != patterns_.end() ? &(*it) : nullptr;
        }
    private:
        std::vector<DataSeriesPattern> patterns_ = {
            DataSeriesPattern::for_datatype<DataPointType::ForcePoint>("_vx", "_vy", "_vz", "_px", "_py", "_pz"),
            DataSeriesPattern::for_datatype<DataPointType::Point>("_vx", "_vy", "_vz"),
            DataSeriesPattern::for_datatype<DataPointType::Point>("_tx", "_ty", "_tz"),
            DataSeriesPattern::for_datatype<DataPointType::Point>("_px", "_py", "_pz"),
            DataSeriesPattern::for_datatype<DataPointType::Orientation>("_1", "_2", "_3", "_4"),
            DataSeriesPattern::for_datatype<DataPointType::Point>("_1", "_2", "_3"),
            DataSeriesPattern::for_datatype<DataPointType::BodyForce>("_fx", "_fy", "_fz"),

            // extra
            DataSeriesPattern::for_datatype<DataPointType::Point>("_x", "_y", "_z"),
            DataSeriesPattern::for_datatype<DataPointType::Point>("x", "y", "z"),
        };
    };
}

StorageSchema opyn::StorageSchema::parse(const OpenSim::Storage& storage)
{
    const DataSeriesPatterns patterns;
    const auto& labels = storage.getColumnLabels();  // includes time

    std::vector<DataSeriesAnnotation> annotations;
    int offset = 1;  // offset 0 == "time" (skip it)

    while (offset < labels.size()) {
        const std::span<std::string> remaining_labels{&labels[offset], static_cast<size_t>(labels.size()) - static_cast<size_t>(offset)};
        if (const DataSeriesPattern* pattern = patterns.try_match(remaining_labels)) {
            annotations.push_back({
                .dataColumnOffset = offset-1,  // drop time for this index
                .label = std::string{pattern->remove_suffix(remaining_labels.front())},
                .dataType = pattern->datatype(),
            });
            offset += static_cast<int>(num_elements_in(pattern->datatype()));
        }
        else {
            annotations.push_back({
                .dataColumnOffset = offset-1,  // drop time for this index
                .label = remaining_labels.front(),
                .dataType = DataPointType::Unknown,
            });
            offset += 1;
        }
    }
    return StorageSchema{std::move(annotations)};
}
