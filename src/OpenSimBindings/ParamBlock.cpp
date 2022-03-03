#include "ParamBlock.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace
{
    struct Param final {
        Param(std::string_view name_, std::string_view description_, osc::ParamValue value_) :
            name{std::move(name_)},
            description{std::move(description_)},
            value{std::move(value_)}
        {
        }

        std::string name;
        std::string description;
        osc::ParamValue value;
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

    std::optional<ParamValue> findValue(std::string const& name) const
    {
        Param const* p = find(name);
        return p ? std::optional<ParamValue>{p->value} : std::nullopt;
    }

    void pushParam(std::string_view name, std::string_view description, ParamValue v)
    {
        Param* p = find(std::string{name});
        if (p)
        {
            *p = Param{std::move(name), std::move(description), std::move(v)};
        }
        else
        {
            m_Params.emplace_back(std::move(name), std::move(description), std::move(v));
        }
    }

    void setValue(int idx, ParamValue v)
    {
        get(idx).value = std::move(v);
    }

    void setValue(std::string const& name, ParamValue value)
    {
        Param* p = find(name);

        if (p)
        {
            p->value = std::move(value);
        }
        else
        {
            throw std::runtime_error{"ParamBlock::setValue(std::string const&, ParamValue): failed: cannot find a param with that name"};
        }
    }

private:
    Param& get(int idx)
    {
        return m_Params.at(static_cast<size_t>(idx));
    }

    Param const& get(int idx) const
    {
        return const_cast<Impl&>(*this).get(idx);
    }

    Param* find(std::string const& name)
    {
        auto it = std::find_if(m_Params.begin(), m_Params.end(), [&name](Param const& param)
        {
            return param.name == name;
        });

        return it != m_Params.end() ? &(*it) : nullptr;
    }

    Param const* find(std::string const& name) const
    {
        return const_cast<Impl&>(*this).find(name);
    }

    std::vector<Param> m_Params;
};


// public API

osc::ParamBlock::ParamBlock() : m_Impl{new Impl{}} {}
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
    return m_Impl->getName(std::move(idx));
}

std::string const& osc::ParamBlock::getDescription(int idx) const
{
    return m_Impl->getDescription(std::move(idx));
}

osc::ParamValue const& osc::ParamBlock::getValue(int idx) const
{
    return m_Impl->getValue(std::move(idx));
}

std::optional<osc::ParamValue> osc::ParamBlock::findValue(std::string const& name) const
{
    return m_Impl->findValue(name);
}

void osc::ParamBlock::pushParam(std::string_view name, std::string_view description, ParamValue value)
{
    m_Impl->pushParam(std::move(name), std::move(description), std::move(value));
}

void osc::ParamBlock::setValue(int idx, ParamValue value)
{
    m_Impl->setValue(std::move(idx), std::move(value));
}

void osc::ParamBlock::setValue(std::string const& name, ParamValue value)
{
    m_Impl->setValue(name, std::move(value));
}
