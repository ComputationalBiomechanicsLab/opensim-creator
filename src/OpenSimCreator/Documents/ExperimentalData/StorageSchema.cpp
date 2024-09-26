#include "StorageSchema.h"

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

using namespace osc;
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
            (sizeof...(ColumnHeaderStrings) == numElementsIn(DataType)) and
            (std::constructible_from<CStringView, ColumnHeaderStrings> && ...)
            static DataSeriesPattern forDatatype(ColumnHeaderStrings&&... headerSuffixes)
        {
            return DataSeriesPattern{DataType, std::initializer_list<CStringView>{CStringView{std::forward<ColumnHeaderStrings>(headerSuffixes)}...}};  // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        }

        // Returns the `DataPointType` matched by this pattern.
        DataPointType datatype() const { return m_Type; }

        // Returns `true` if the given column headers match this pattern.
        bool matches(std::span<const std::string> headers) const
        {
            if (headers.size() < m_HeaderSuffixes.size()) {
                return false;
            }
            for (size_t i = 0; i < m_HeaderSuffixes.size(); ++i) {
                if (not headers[i].ends_with(m_HeaderSuffixes[i])) {
                    return false;
                }
            }
            return true;
        }

        // If the given `columnHeader` matches a suffix in this pattern, returns a substring view
        // of the provided string view, minus the suffix. Otherwise, returns the provided string
        // view.
        std::string_view remove_suffix(std::string_view columHeader) const
        {
            for (const auto& suffix : m_HeaderSuffixes) {
                if (columHeader.ends_with(suffix)) {
                    return columHeader.substr(0, columHeader.size() - suffix.size());
                }
            }
            return columHeader;  // couldn't remove it
        }
    private:
        DataSeriesPattern(DataPointType type, std::initializer_list<CStringView> header_suffxes) :
            m_Type{type},
            m_HeaderSuffixes{header_suffxes}
        {}

        DataPointType m_Type;
        std::vector<CStringView> m_HeaderSuffixes;
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
            const auto it = rgs::find_if(m_Patterns, [&headers](const auto& pattern) { return pattern.matches(headers); });
            return it != m_Patterns.end() ? &(*it) : nullptr;
        }
    private:
        std::vector<DataSeriesPattern> m_Patterns = {
            DataSeriesPattern::forDatatype<DataPointType::ForcePoint>("_vx", "_vy", "_vz", "_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_vx", "_vy", "_vz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_tx", "_ty", "_tz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Orientation>("_1", "_2", "_3", "_4"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_1", "_2", "_3"),
            DataSeriesPattern::forDatatype<DataPointType::BodyForce>("_fx", "_fy", "_fz"),

            // extra
            DataSeriesPattern::forDatatype<DataPointType::Point>("_x", "_y", "_z"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("x", "y", "z"),
        };
    };
}

StorageSchema osc::StorageSchema::parse(const OpenSim::Storage& storage)
{
    const DataSeriesPatterns patterns;
    const auto& labels = storage.getColumnLabels();  // includes time

    std::vector<DataSeriesAnnotation> annotations;
    int offset = 1;  // offset 0 == "time" (skip it)

    while (offset < labels.size()) {
        const std::span<std::string> remainingLabels{&labels[offset], static_cast<size_t>(labels.size() - offset)};
        if (const DataSeriesPattern* pattern = patterns.try_match(remainingLabels)) {
            annotations.push_back({
                .dataColumnOffset = offset-1,  // drop time for this index
                .label = std::string{pattern->remove_suffix(remainingLabels.front())},
                .dataType = pattern->datatype(),
            });
            offset += static_cast<int>(numElementsIn(pattern->datatype()));
        }
        else {
            annotations.push_back({
                .dataColumnOffset = offset-1,  // drop time for this index
                .label = remainingLabels.front(),
                .dataType = DataPointType::Unknown,
            });
            offset += 1;
        }
    }
    return StorageSchema{std::move(annotations)};
}
