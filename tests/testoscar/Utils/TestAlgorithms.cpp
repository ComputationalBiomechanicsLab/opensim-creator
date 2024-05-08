#include <oscar/Utils/Algorithms.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <unordered_map>

using namespace osc;
namespace rgs = std::ranges;

TEST(all_of, WorksAsExpected)
{
    auto vs = std::to_array({-1, -2, -3, 0, 1, 2, 3});
    ASSERT_TRUE(rgs::all_of(vs, [](int v){ return v > -4; }));
    ASSERT_TRUE(rgs::all_of(vs, [](int v){ return v <  4; }));
    ASSERT_FALSE(rgs::all_of(vs, [](int v){ return v > 0; }));
}
TEST(at, WorksAsExpected)
{
    auto vs = std::to_array({-1, -2, -3, 0, 1, 2, 3});
    ASSERT_EQ(at(vs, 0), -1);
    ASSERT_EQ(at(vs, 1), -2);
    ASSERT_EQ(at(vs, 2), -3);
    ASSERT_EQ(at(vs, 3), -0);
    ASSERT_EQ(at(vs, 4),  1);
    ASSERT_EQ(at(vs, 5),  2);
    ASSERT_EQ(at(vs, 6),  3);
    ASSERT_THROW({ at(vs, 7); }, std::out_of_range);
    ASSERT_THROW({ at(vs, 8); }, std::out_of_range);

}

TEST(at, WorksAtCompileTime)
{
    constexpr auto ary = std::to_array({-1, 0, 1});
    static_assert(at(ary, 1) == 0);
}

TEST(lookup_or_nullopt, WorksWithStdUnorderedMap)
{
    std::unordered_map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(lookup_or_nullopt(um, -20), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, -15), 20);
    ASSERT_EQ(lookup_or_nullopt(um, -2), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, -1), 98);
    ASSERT_EQ(lookup_or_nullopt(um, 0), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, 5), 10);
}

TEST(lookup_or_nullopt, WorksWithStdMap)
{
    std::map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(lookup_or_nullopt(um, -20), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, -15), 20);
    ASSERT_EQ(lookup_or_nullopt(um, -2), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, -1), 98);
    ASSERT_EQ(lookup_or_nullopt(um, 0), std::nullopt);
    ASSERT_EQ(lookup_or_nullopt(um, 5), 10);
}

TEST(lookup_or_nullptr, WorksWithUnorderedMap)
{
    std::unordered_map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(lookup_or_nullptr(um, -20), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -15), 20);
    ASSERT_EQ(lookup_or_nullptr(um, -2), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -1), 98);
    ASSERT_EQ(lookup_or_nullptr(um, 0), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, 5), 10);
}

TEST(lookup_or_nullptr, WorksWithStdMap)
{
    std::map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(lookup_or_nullptr(um, -20), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -15), 20);
    ASSERT_EQ(lookup_or_nullptr(um, -2), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -1), 98);
    ASSERT_EQ(lookup_or_nullptr(um, 0), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, 5), 10);
}

TEST(lookup_or_nullptr, WorksWithConstQualifiedStdUnorderedMap)
{
    std::unordered_map<int, int> const um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(lookup_or_nullptr(um, -20), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -15), 20);
    ASSERT_EQ(lookup_or_nullptr(um, -2), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, -1), 98);
    ASSERT_EQ(lookup_or_nullptr(um, 0), nullptr);
    ASSERT_EQ(*lookup_or_nullptr(um, 5), 10);
}

TEST(lookup_or_nullptr, CanMutateViaReturnedPointer)
{
    std::unordered_map<int, int> um{
        {20, 30},
    };

    int* v = lookup_or_nullptr(um, 20);
    *v = -40;
    ASSERT_TRUE(lookup_or_nullptr(um, 20));
    ASSERT_EQ(*lookup_or_nullptr(um, 20), -40);
}

TEST(min_element, WorksAsExpected)
{
    auto const els = std::to_array({1, 5, 8, -4, 13});
    ASSERT_EQ(rgs::min_element(els), els.begin() + 3);
}

TEST(min, WorksAsExpected)
{
    auto const els = std::to_array({1, 5, 8, -4, 13});
    ASSERT_EQ(rgs::min(els), -4);
}

TEST(minmax_element, WorksAsExpected)
{
    auto const els = std::to_array({1, 5, 8, -4, -4, 13, 13, 13});
    auto const [minit, maxit] = rgs::minmax_element(els);
    ASSERT_EQ(minit, els.begin() + 3);
    ASSERT_EQ(maxit, els.end() - 1);
}

TEST(minmax, WorksAsExpected)
{
    auto const els = std::to_array({1, 5, 8, -4, -4, 13, 13, 13});
    auto const [min, max] = rgs::minmax(els);
    ASSERT_EQ(min, -4);
    ASSERT_EQ(max, 13);
}

namespace
{
    class Base {
    protected:
        Base() = default;
        Base(const Base&) = default;
        Base(Base&&) noexcept = default;
        Base& operator=(const Base&) = default;
        Base& operator=(Base&&) noexcept = default;
    public:
        virtual ~Base() noexcept = default;
        constexpr friend bool operator==(const Base&, const Base&) = default;
    };
    class Derived1 : public Base {
    public:
        explicit constexpr Derived1(int data) : m_Data{data} {}
        constexpr friend bool operator==(const Derived1&, const Derived1&) = default;
    private:
        int m_Data;
    };
    class Derived2 : public Base {
    public:
        explicit constexpr Derived2(double data) : m_Data{data} {}
        constexpr friend bool operator==(const Derived2&, const Derived2&) = default;
    private:
        double m_Data;
    };
}

TEST(is_eq_downcasted, WorksAsExpected)
{
    // basic case: both types are the same and don't require downcasting
    ASSERT_TRUE(is_eq_downcasted<Derived1>(Derived1{1}, Derived1{1}));
    ASSERT_FALSE(is_eq_downcasted<Derived1>(Derived1{1}, Derived1{2}));
    ASSERT_TRUE(is_eq_downcasted<Derived2>(Derived2{1.0}, Derived2{1.0}));
    ASSERT_FALSE(is_eq_downcasted<Derived2>(Derived2{1.0}, Derived2{2.0}));

    // correct downcast case
    ASSERT_TRUE(is_eq_downcasted<Derived1>(Derived1{1}, static_cast<const Base&>(Derived1{1})));
    ASSERT_FALSE(is_eq_downcasted<Derived1>(Derived1{1}, static_cast<const Base&>(Derived1{2})));
    ASSERT_TRUE(is_eq_downcasted<Derived1>(static_cast<const Base&>(Derived1{1}), Derived1{1}));
    ASSERT_FALSE(is_eq_downcasted<Derived1>(static_cast<const Base&>(Derived1{2}), Derived1{1}));

    // incorrect downcast case (i.e. should never compare equal)
    ASSERT_FALSE(is_eq_downcasted<Derived1>(Derived1{1}, static_cast<const Base&>(Derived2{1.0})));
    ASSERT_FALSE(is_eq_downcasted<Derived1>(static_cast<const Base&>(Derived2{1.0}), Derived1(1)));
}
