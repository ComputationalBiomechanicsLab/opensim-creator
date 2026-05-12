#include "series.h"

#include <string>
#include <utility>
#include <vector>

using namespace opyn;

opyn::Series::Series(std::string name, std::vector<double> values) :
    name_{std::move(name)},
    values_{std::move(values)}
{}

std::vector<double> opyn::Series::to_list() const { return values_; }
