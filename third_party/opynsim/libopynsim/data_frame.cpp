#include "data_frame.h"

#include <liboscar/utilities/assertions.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    std::unordered_map<std::string, size_t> build_index_lookup(const std::vector<std::string>& column_names)
    {
        std::unordered_map<std::string, size_t> rv;
        rv.reserve(column_names.size());
        for (size_t i = 0; i < column_names.size(); ++i) {
            rv.insert_or_assign(column_names[i], i);
        }
        return rv;
    }
}

opyn::DataFrame::DataFrame(
    std::vector<std::string> column_names,
    std::vector<std::vector<double>> column_data,
    std::unordered_map<std::string, std::string> attrs) :

    column_names_{std::move(column_names)},
    column_data_{std::move(column_data)},
    column_to_index_lookup_{build_index_lookup(column_names_)},
    attrs_{std::move(attrs)}
{
    OSC_ASSERT_ALWAYS(column_names_.size() == column_data_.size() && "The number of column names (headers) must match the number of column data vectors");
}

std::vector<std::string> opyn::DataFrame::columns() const
{
    return column_names_;
}

std::pair<size_t, size_t> opyn::DataFrame::shape() const
{
    const size_t num_columns = column_data_.size();
    return num_columns > 0 ?
        std::pair{column_data_[0].size(), num_columns} :
        std::pair{0uz,                    num_columns};
}

std::unordered_map<std::string, std::string> opyn::DataFrame::attrs() const
{
    return attrs_;
}
