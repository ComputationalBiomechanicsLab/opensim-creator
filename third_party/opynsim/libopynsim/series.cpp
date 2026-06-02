#include "series.h"

#include <string>
#include <utility>
#include <vector>

using namespace opyn;

class opyn::Series::Impl final {
public:
    Impl() = default;

    Impl(std::string name, std::vector<double> values) :
        name_{std::move(name)},
        values_{std::move(values)}
    {}

    std::string_view name() const { return name_; }
    std::tuple<size_t> shape() const { return values_.size(); }
    size_type size() const { return values_.size(); }
    const_reference operator[](size_type pos) const { return values_[pos]; }
    const_iterator begin() const { return values_.data(); }
    const_iterator end() const { return values_.data() + values_.size(); }
    std::vector<double> to_list() const { return values_; }

private:
    std::string name_;
    std::vector<double> values_;
};

opyn::Series::Series() :
    impl_{osc::make_cow<Impl>()}
{}
opyn::Series::Series(std::string name, std::vector<double> values) :
    impl_{osc::make_cow<Impl>(std::move(name), std::move(values))}
{}

std::string_view opyn::Series::name() const { return impl_->name(); }
std::tuple<size_t> opyn::Series::shape() const { return impl_->shape(); }
opyn::Series::size_type opyn::Series::size() const { return impl_->size(); }
opyn::Series::const_reference opyn::Series::operator[](size_type pos) const { return (*impl_)[pos]; }
opyn::Series::const_iterator opyn::Series::begin() const { return impl_->begin(); }
opyn::Series::const_iterator opyn::Series::end() const { return impl_->end(); }
std::vector<double> opyn::Series::to_list() const { return impl_->to_list(); }
