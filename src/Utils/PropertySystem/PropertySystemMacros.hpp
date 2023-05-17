#pragma once

#include "src/Utils/PropertySystem/Component.hpp"
#include "src/Utils/PropertySystem/PropertyDefinition.hpp"
#include "src/Utils/PropertySystem/SocketDefinition.hpp"

#include <cstddef>
#include <memory>

#define OSC_COMPONENT(ClassType) \
private: \
    using Self = ClassType; \
public: \
    std::unique_ptr<ClassType> clone() const \
    { \
        return std::make_unique<ClassType>(*this); \
    } \
private: \
    std::unique_ptr<osc::Component> implClone() const final \
    { \
        return clone(); \
    }

#define OSC_SOCKET(ConnecteeType, name, description) \
    static constexpr char _property_##name##_name[] = #name; \
    static constexpr char _property_##name##_description[] = #description;  \
    static constexpr size_t _property_##name##_offset_getter() \
    { \
        return offsetof(Self, name); \
    } \
    osc::SocketDefinition< \
        Self, \
        ConnecteeType, \
        _property_##name##_offset_getter, \
        _property_##name##_name, \
        _property_##name##_description \
    > name

#define OSC_PROPERTY(ValueType, memberName, defaultValue, stringName, description) \
    static constexpr char _property_##memberName##_name[] = stringName; \
    static constexpr char _property_##memberName##_description[] = description;  \
    static constexpr size_t _property_##memberName##_offset_getter() \
    { \
        return offsetof(Self, memberName); \
    } \
    osc::PropertyDefinition< \
        Self, \
        ValueType, \
        _property_##memberName##_offset_getter, \
        _property_##memberName##_name, \
        _property_##memberName##_description \
    > memberName{defaultValue}
