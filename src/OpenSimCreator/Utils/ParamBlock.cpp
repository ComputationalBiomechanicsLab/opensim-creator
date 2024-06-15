#include "ParamBlock.h"

#include <oscar/Utils/Algorithms.h>

#include <algorithm>
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
        Param(std::string_view name_, std::string_view description_, ParamValue value_) :
            name{name_},
            description{description_},
            value{value_}
        {}

        std::string name;
        std::string description;
        ParamValue value;
    };
}


class osc::ParamBlock::Impl final {
public:
    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    int size() const
    {
        return static_cast<int>(m_Params.size());
    }

    const std::string& getName(int idx) const
    {
        return get(idx).name;
    }

    const std::string& getDescription(int idx) const
    {
        return get(idx).description;
    }

    const ParamValue& getValue(int idx) const
    {
        return get(idx).value;
    }

    std::optional<ParamValue> findValue(std::string_view name) const
    {
        return findValue(std::string{name});
    }

    void pushParam(std::string_view name, std::string_view description, ParamValue v)
    {
        Param* p = find(std::string{name});
        if (p)
        {
            *p = Param{name, description, v};
        }
        else
        {
            m_Params.emplace_back(name, description, v);
        }
    }

    void setValue(int idx, ParamValue v)
    {
        get(idx).value = v;
    }

    void setValue(const std::string& name, ParamValue value)
    {
        Param* p = find(name);

        if (p) {
            p->value = value;
        }
        else {
            throw std::runtime_error{"ParamBlock::setValue(const std::string&, ParamValue): failed: cannot find a param with that name"};
        }
    }

private:
    std::optional<ParamValue> findValue(const std::string& name) const
    {
        const Param* p = find(name);
        return p ? std::optional<ParamValue>{p->value} : std::nullopt;
    }

    Param& get(int idx)
    {
        return m_Params.at(static_cast<size_t>(idx));
    }

    const Param& get(int idx) const
    {
        return m_Params.at(static_cast<size_t>(idx));
    }


    // helper, to prevent writing a const-/non-const-version of a member method
    template<rgs::range Range>
    static auto find(Range& range, const std::string& name) -> decltype(rgs::data(range))
    {
        return find_or_nullptr(range, name, [](const auto& el) { return el.name; });
    }

    Param* find(const std::string& name)
    {
        return Impl::find(m_Params, name);
    }

    const Param* find(const std::string& name) const
    {
        return Impl::find(m_Params, name);
    }

    std::vector<Param> m_Params;
};


// public API

osc::ParamBlock::ParamBlock() : m_Impl{std::make_unique<Impl>()} {}
osc::ParamBlock::ParamBlock(const ParamBlock&) = default;
osc::ParamBlock::ParamBlock(ParamBlock&&) noexcept = default;
osc::ParamBlock& osc::ParamBlock::operator=(const ParamBlock&) = default;
osc::ParamBlock& osc::ParamBlock::operator=(ParamBlock&&) noexcept = default;
osc::ParamBlock::~ParamBlock() noexcept = default;

int osc::ParamBlock::size() const
{
    return m_Impl->size();
}

const std::string& osc::ParamBlock::getName(int idx) const
{
    return m_Impl->getName(idx);
}

const std::string& osc::ParamBlock::getDescription(int idx) const
{
    return m_Impl->getDescription(idx);
}

const ParamValue& osc::ParamBlock::getValue(int idx) const
{
    return m_Impl->getValue(idx);
}

std::optional<ParamValue> osc::ParamBlock::findValue(std::string_view name) const
{
    return m_Impl->findValue(name);
}

void osc::ParamBlock::pushParam(std::string_view name, std::string_view description, ParamValue value)
{
    m_Impl->pushParam(name, description, value);
}

void osc::ParamBlock::setValue(int idx, ParamValue value)
{
    m_Impl->setValue(idx, value);
}

void osc::ParamBlock::setValue(const std::string& name, ParamValue value)
{
    m_Impl->setValue(name, value);
}
