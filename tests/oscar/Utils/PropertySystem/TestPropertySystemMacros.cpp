#include "oscar/Utils/PropertySystem/PropertySystemMacros.hpp"

#include "oscar/Utils/PropertySystem/Component.hpp"
#include "oscar/Utils/PropertySystem/PropertyType.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <gtest/gtest.h>
#include <string>

// helper classes that use the macros
namespace
{
    class BlankComponentWithComponentMacro final : public osc::Component {
        OSC_COMPONENT(BlankComponentWithComponentMacro);
    };

    class ComponentWithStringProp final : public osc::Component {
        OSC_COMPONENT(ComponentWithStringProp);

    public:
        std::string const& getString()
        {
            return *m_StringProperty;
        }

        std::string& updString()
        {
            return *m_StringProperty;
        }

        OSC_PROPERTY(
            std::string,
            m_StringProperty,
            "defaultValue",
            "stringName",
            "some description"
        );
    };
}

TEST(PropertySystemMacros, CanDefineComponent)
{
    BlankComponentWithComponentMacro isConstructable{};
}

TEST(PropertySystemMacros, CanDefineStringProperties)
{
    ComponentWithStringProp c;
}

TEST(PropertySystemMacros, MacroDefinedPropertyGetOwnerReturnsComponent)
{
    ComponentWithStringProp c;
    ASSERT_EQ(&c.m_StringProperty.getOwner(), &c);
}

TEST(PropertySystemMacros, MacroDefinedPropertyUpdOwnerReturnsComponent)
{
    ComponentWithStringProp c;
    ASSERT_EQ(&c.m_StringProperty.updOwner(), &c);
}

TEST(PropertySystemMacros, StringMacroDefinedPropertyHasExpectedPropertyType)
{
    ComponentWithStringProp c;
    ASSERT_EQ(c.m_StringProperty.getPropertyType(), osc::PropertyType::String);
}

TEST(PropertySystemMacros, MacroDefinedPropertyHasExpectedDefaultValue)
{
    ComponentWithStringProp c;
    ASSERT_EQ(c.m_StringProperty.getValue(), "defaultValue");
}

TEST(PropertySystemMacros, MacroDefinedPropertyHasExpectedName)
{
    ComponentWithStringProp c;
    ASSERT_EQ(c.m_StringProperty.getName(), "stringName");
}

TEST(PropertySystemMacros, MacroDefinedPropertyHasExpectedDescription)
{
    ComponentWithStringProp c;
    ASSERT_EQ(c.m_StringProperty.getDescription(), "some description");
}

TEST(PropertySystemMacros, MacroDefinedPropertyCanAlsoBeReadViaMemberMethod)
{
    ComponentWithStringProp c;
    ASSERT_EQ(c.m_StringProperty.getValue(), c.getString());
}

TEST(PropertySystemMacros, MacroDefinedPropertyCanBeUpdatedViaMemberMethod)
{
    ComponentWithStringProp c;
    c.updString() = "newValue";
    ASSERT_EQ(c.m_StringProperty.getValue(), "newValue");
}
