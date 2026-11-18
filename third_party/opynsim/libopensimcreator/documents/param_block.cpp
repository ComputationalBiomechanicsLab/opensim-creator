#include "param_block.h"

#include <liboscar/utilities/algorithms.h>

#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    struct Param final {
        Param(std::string_view name, std::string_view description, ParamValue value) :
            name{name},
            description{description},
            value{value}
        {}

        std::string name;
        std::string description;
        ParamValue value;
    };
}

class osc::ParamBlock::Impl final {
public:
    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    int size() const { return static_cast<int>(params_.size()); }

    const std::string& get_name(int idx) const { return get(idx).name; }

    const std::string& get_description(int idx) const { return get(idx).description; }

    const ParamValue& get_value(int idx) const { return get(idx).value; }

    std::optional<ParamValue> find_value(std::string_view name) const
    {
        return find_value(std::string{name});
    }

    void push_param(std::string_view name, std::string_view description, ParamValue v)
    {
        Param* p = find(std::string{name});
        if (p) {
            *p = Param{name, description, v};
        } else {
            params_.emplace_back(name, description, v);
        }
    }

    void set_value(int idx, ParamValue v) { get(idx).value = v; }

    void set_value(const std::string& name, ParamValue value)
    {
        Param* p = find(name);

        if (p) {
            p->value = value;
        } else {
            throw std::runtime_error{
                "ParamBlock::setValue(const std::string&, ParamValue): failed: cannot find a param with that name"};
        }
    }

private:
    std::optional<ParamValue> find_value(const std::string& name) const
    {
        const Param* p = find(name);
        return p ? std::optional<ParamValue>{p->value} : std::nullopt;
    }

    Param& get(int idx) { return params_.at(static_cast<size_t>(idx)); }

    const Param& get(int idx) const { return params_.at(static_cast<size_t>(idx)); }

    // helper, to prevent writing a const-/non-const-version of a member method
    template<rgs::range Range>
    static auto find(Range& range, const std::string& name) -> decltype(rgs::data(range))
    {
        return find_or_nullptr(range, name, [](const auto& el) { return el.name; });
    }

    Param* find(const std::string& name) { return Impl::find(params_, name); }

    const Param* find(const std::string& name) const { return Impl::find(params_, name); }

    std::vector<Param> params_;
};

osc::ParamBlock::ParamBlock() :
    impl_{std::make_unique<Impl>()}
{}
osc::ParamBlock::ParamBlock(const ParamBlock&) = default;
osc::ParamBlock::ParamBlock(ParamBlock&&) noexcept = default;
osc::ParamBlock& osc::ParamBlock::operator=(const ParamBlock&) = default;
osc::ParamBlock& osc::ParamBlock::operator=(ParamBlock&&) noexcept = default;
osc::ParamBlock::~ParamBlock() noexcept = default;

int osc::ParamBlock::size() const
{
    return impl_->size();
}

const std::string& osc::ParamBlock::get_name(int idx) const
{
    return impl_->get_name(idx);
}

const std::string& osc::ParamBlock::get_description(int idx) const
{
    return impl_->get_description(idx);
}

const ParamValue& osc::ParamBlock::get_value(int idx) const
{
    return impl_->get_value(idx);
}

std::optional<ParamValue> osc::ParamBlock::find_value(std::string_view name) const
{
    return impl_->find_value(name);
}

void osc::ParamBlock::push_param(
    std::string_view name, std::string_view description, ParamValue value)
{
    impl_->push_param(name, description, value);
}

void osc::ParamBlock::set_value(int idx, ParamValue value)
{
    impl_->set_value(idx, value);
}

void osc::ParamBlock::set_value(const std::string& name, ParamValue value)
{
    impl_->set_value(name, value);
}
