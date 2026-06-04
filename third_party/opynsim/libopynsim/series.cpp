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

    friend bool operator==(const Impl&, const Impl&) = default;

    std::string_view name() const { return name_; }
    std::tuple<size_t> shape() const { return values_.size(); }
    [[nodiscard]] bool empty() const { return values_.empty(); }
    size_type size() const { return values_.size(); }
    const_reference operator[](size_type pos) const { return values_[pos]; }
    const_iterator begin() const { return values_.data(); }
    const_iterator end() const { return values_.data() + values_.size(); }
    std::vector<double> to_list() const { return values_; }
    Series map_elements(const std::function<double(double)>& f) const
    {
        std::vector<double> new_values;
        new_values.reserve(values_.size());
        for (const auto& value : values_) {
            new_values.push_back(f(value));
        }
        return Series{name_, new_values};
    }

    friend Series operator*(double lhs, const Impl& rhs)
    {
        std::vector<double> new_values;
        new_values.reserve(rhs.values_.size());
        for (const double& value : rhs.values_) {
            new_values.push_back(lhs * value);
        }
        return Series{rhs.name_, new_values};
    }

    friend Series operator*(const Impl& lhs, double rhs)
    {
        // Forward to the opposite version (multiplication is reflexive)
        return rhs * lhs;
    }
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
bool opyn::Series::empty() const { return impl_->empty(); }
opyn::Series::size_type opyn::Series::size() const { return impl_->size(); }
opyn::Series::const_reference opyn::Series::operator[](size_type pos) const { return (*impl_)[pos]; }
opyn::Series::const_iterator opyn::Series::begin() const { return impl_->begin(); }
opyn::Series::const_iterator opyn::Series::end() const { return impl_->end(); }
std::vector<double> opyn::Series::to_list() const { return impl_->to_list(); }
Series opyn::Series::map_elements(const std::function<double(double)>& f) const { return impl_->map_elements(f); }
bool opyn::operator==(const Series& lhs, const Series& rhs) { return lhs.impl_ == rhs.impl_ or *lhs.impl_ == *rhs.impl_; }
Series opyn::operator*(double lhs, const Series& rhs) { return lhs * *rhs.impl_; }
Series opyn::operator*(const Series& lhs, double rhs) { return *lhs.impl_ * rhs; }
