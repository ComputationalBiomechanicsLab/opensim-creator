#pragma once

#define OSC_METHOD_TEST(testname, method)                                      \
template<typename T>                                                           \
class testname final {                                                         \
    using YesType = char[2];                                                   \
    using NoType = char[1];                                                    \
                                                                               \
    template<typename C>                                                       \
    static YesType& test(decltype(&C::method));                                \
                                                                               \
    template<typename C>                                                       \
    static NoType& test(...);                                                  \
                                                                               \
public:                                                                        \
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(YesType);       \
};                                                                             \
                                                                               \
template<typename T>                                                           \
inline constexpr bool testname ## _v = testname<T>::value;
