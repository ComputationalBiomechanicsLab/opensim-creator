#include "ParamBlock.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <variant>

using osc::ParamValue;

namespace
{
    struct Param final {
        Param(std::string_view name_, std::string_view description_, ParamValue value_) :
            name{name_},
            description{description_},
            value{value_}
        {
        }

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

    std::string const& getName(int idx) const
    {
        return get(idx).name;
    }

    std::string const& getDescription(int idx) const
    {
        return get(idx).description;
    }

    ParamValue const& getValue(int idx) const
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

    void setValue(std::string const& name, ParamValue value)
    {
        Param* p = find(name);

        if (p)
        {
            p->value = value;
        }
        else
        {
            throw std::runtime_error{"ParamBlock::setValue(std::string const&, ParamValue): failed: cannot find a param with that name"};
        }
    }

private:
    std::optional<ParamValue> findValue(std::string const& name) const
    {
        Param const* p = find(name);
        return p ? std::optional<ParamValue>{p->value} : std::nullopt;
    }

    Param& get(int idx)
    {
        return m_Params.at(static_cast<size_t>(idx));
    }

    Param const& get(int idx) const
    {
        return m_Params.at(static_cast<size_t>(idx));
    }


    // helper, to prevent writing a const-/non-const-version of a member method
    template<std::ranges::range Container>
    static auto find(Container& c, std::string const& name) -> decltype(&(*c.begin()))
    {
        auto const it = std::find_if(c.begin(), c.end(), [&name](Param const& el)
        {
            return el.name == name;
        });
        return it != c.end() ? &(*it) : nullptr;
    }

    Param* find(std::string const& name)
    {
        return Impl::find(m_Params, name);
    }

    Param const* find(std::string const& name) const
    {
        return Impl::find(m_Params, name);
    }

    std::vector<Param> m_Params;
};


// public API

osc::ParamBlock::ParamBlock() : m_Impl{std::make_unique<Impl>()} {}
osc::ParamBlock::ParamBlock(ParamBlock const&) = default;
osc::ParamBlock::ParamBlock(ParamBlock&&) noexcept = default;
osc::ParamBlock& osc::ParamBlock::operator=(ParamBlock const&) = default;
osc::ParamBlock& osc::ParamBlock::operator=(ParamBlock&&) noexcept = default;
osc::ParamBlock::~ParamBlock() noexcept = default;

int osc::ParamBlock::size() const
{
    return m_Impl->size();
}

std::string const& osc::ParamBlock::getName(int idx) const
{
    return m_Impl->getName(idx);
}

std::string const& osc::ParamBlock::getDescription(int idx) const
{
    return m_Impl->getDescription(idx);
}

osc::ParamValue const& osc::ParamBlock::getValue(int idx) const
{
    return m_Impl->getValue(idx);
}

std::optional<osc::ParamValue> osc::ParamBlock::findValue(std::string_view name) const
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

void osc::ParamBlock::setValue(std::string const& name, ParamValue value)
{
    m_Impl->setValue(name, value);
}
