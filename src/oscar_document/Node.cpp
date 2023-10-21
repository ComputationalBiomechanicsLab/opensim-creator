#include "Node.hpp"

#include <oscar_document/NodePath.hpp>
#include <oscar_document/Variant.hpp>

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

class osc::doc::Node::Impl final {
public:
    CStringView getName() const
    {
        return "todo";
    }

    void setName(std::string_view)
    {
        // todo
    }

    Node const* getParent() const
    {
        return nullptr;  // todo
    }

    Node* updParent()
    {
        return nullptr;  // todos
    }

    size_t getNumChildren() const
    {
        return 0;  // todo
    }

    Node const* getChild(size_t) const
    {
        return nullptr;  // todo
    }

    Node const* getChild(std::string_view)
    {
        return nullptr;  // todo
    }

    Node* updChild(size_t) const
    {
        return nullptr;  // todo
    }

    Node* updChild(std::string_view)
    {
        return nullptr;  // todo
    }

    Node& addChild(std::unique_ptr<Node> p)
    {
        return *p;  // todo
    }

    bool removeChild(size_t)
    {
        return false;  // todo
    }

    bool removeChild(Node&)
    {
        return false;  // todo
    }

    bool removeChild(std::string_view)
    {
        return false;  // todo
    }

    NodePath getAbsolutePath() const
    {
        return {};  // todo
    }

    Node const* find(NodePath const&) const
    {
        return nullptr;  // todo
    }

    Node* findMut(NodePath const&)
    {
        return nullptr;  // todo
    }

    size_t getNumProperties() const
    {
        return 0;  // todo
    }

    bool hasProperty(std::string_view) const
    {
        return false;  // todo
    }

    CStringView const* getPropertyName(size_t) const
    {
        return nullptr;  // todo
    }

    Variant const* getPropertyValue(size_t) const
    {
        return nullptr;
    }

    Variant const* getPropertyValue(std::string_view) const
    {
        return nullptr;
    }

    bool setPropertyValue(size_t, Variant const&)
    {
        return false;
    }

    bool setPropertyValue(std::string_view, Variant const&)
    {
        return false;
    }

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }
private:
};


// public API

osc::doc::Node::Node() : m_Impl{std::make_unique<Impl>()} {}
osc::doc::Node::Node(Node const&) = default;
osc::doc::Node::Node(Node&&) noexcept = default;
osc::doc::Node& osc::doc::Node::operator=(Node const&) = default;
osc::doc::Node& osc::doc::Node::operator=(Node&&) noexcept = default;
osc::doc::Node::~Node() noexcept = default;

osc::CStringView osc::doc::Node::getName() const
{
    return m_Impl->getName();
}

void osc::doc::Node::setName(std::string_view newName)
{
    m_Impl->setName(newName);
}

template<>
osc::doc::Node const* osc::doc::Node::getParent() const
{
    return m_Impl->getParent();
}

template<>
osc::doc::Node* osc::doc::Node::updParent()
{
    return m_Impl->updParent();
}

size_t osc::doc::Node::getNumChildren() const
{
    return m_Impl->getNumChildren();
}

template<>
osc::doc::Node const* osc::doc::Node::getChild(size_t i) const
{
    return m_Impl->getChild(i);
}

template<>
osc::doc::Node const* osc::doc::Node::getChild(std::string_view childName) const
{
    return m_Impl->getChild(childName);
}

template<>
osc::doc::Node* osc::doc::Node::updChild(size_t i)
{
    return m_Impl->updChild(i);
}

template<>
osc::doc::Node* osc::doc::Node::updChild(std::string_view childName)
{
    return m_Impl->updChild(childName);
}

template<>
osc::doc::Node& osc::doc::Node::addChild(std::unique_ptr<Node> p)
{
    return m_Impl->addChild(std::move(p));
}

bool osc::doc::Node::removeChild(size_t i)
{
    return m_Impl->removeChild(i);
}

bool osc::doc::Node::removeChild(Node& node)
{
    return m_Impl->removeChild(node);
}

bool osc::doc::Node::removeChild(std::string_view childName)
{
    return m_Impl->removeChild(childName);
}

osc::doc::NodePath osc::doc::Node::getAbsolutePath() const
{
    return m_Impl->getAbsolutePath();
}

template<>
osc::doc::Node const* osc::doc::Node::find(NodePath const& np) const
{
    return m_Impl->find(np);
}

template<>
osc::doc::Node* osc::doc::Node::findMut(NodePath const& np)
{
    return m_Impl->findMut(np);
}

size_t osc::doc::Node::getNumProperties() const
{
    return m_Impl->getNumProperties();
}

bool osc::doc::Node::hasProperty(std::string_view propName) const
{
    return m_Impl->hasProperty(propName);
}

osc::CStringView const* osc::doc::Node::getPropertyName(size_t i) const
{
    return m_Impl->getPropertyName(i);
}

osc::doc::Variant const* osc::doc::Node::getPropertyValue(size_t i) const
{
    return m_Impl->getPropertyValue(i);
}

osc::doc::Variant const* osc::doc::Node::getPropertyValue(std::string_view propName) const
{
    return m_Impl->getPropertyValue(propName);
}

bool osc::doc::Node::setPropertyValue(size_t i, Variant const& v)
{
    return m_Impl->setPropertyValue(i, v);
}

bool osc::doc::Node::setPropertyValue(std::string_view propName, Variant const& v)
{
    return m_Impl->setPropertyValue(propName, v);
}

osc::doc::PropertyDescriptions const&  osc::doc::Node::implGetPropertyList() const
{
    static const PropertyDescriptions s_BlankPropertyDescriptions;
    return s_BlankPropertyDescriptions;
}
