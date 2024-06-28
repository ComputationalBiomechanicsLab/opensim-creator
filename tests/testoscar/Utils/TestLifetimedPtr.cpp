#include <oscar/Utils/LifetimedPtr.h>

#include <oscar/Utils/SharedLifetimeBlock.h>

#include <gtest/gtest.h>

using namespace osc;

namespace
{
    class SomeBaseClass {};
    class SomeDerivingObject : public SomeBaseClass {
    public:
        void some_method() {}
    };

    class AlwaysExpiredLifetime final {
    public:
        LifetimeWatcher watch() const { return {}; }
    };
}

TEST(LifetimedPtr, can_default_construct)
{
    [[maybe_unused]] LifetimedPtr<SomeDerivingObject> ptr;
}

TEST(LifetimedPtr, default_constructed_implicitly_converts_to_false)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_FALSE(ptr);
}

TEST(LifetimedPtr, default_constructed_get_returns_nullptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_EQ(ptr.get(), nullptr);
}

TEST(LifetimedPtr, can_be_constructed_from_nullptr)
{
    [[maybe_unused]] LifetimedPtr<SomeDerivingObject> ptr{nullptr};
}

TEST(LifetimedPtr, nullptr_constructed_implicitly_converts_to_false)
{
    LifetimedPtr<SomeDerivingObject> ptr{nullptr};
    ASSERT_FALSE(ptr);
}

TEST(LifetimedPtr, nullptr_constructed_get_returns_nullptr)
{
    LifetimedPtr<SomeDerivingObject> ptr{nullptr};
    ASSERT_EQ(ptr.get(), nullptr);
}

TEST(LifetimedPtr, when_constructed_with_expired_lifetime_produced_expired_ptr)
{
    AlwaysExpiredLifetime expired_lifetime;
    SomeDerivingObject obj;

    LifetimedPtr<SomeDerivingObject> ptr{expired_lifetime, &obj};

    ASSERT_TRUE(ptr.expired());
}

TEST(LifetimedPtr, when_constructed_with_in_life_lifetime_produces_not_expired_ptr)
{
    SharedLifetimeBlock valid_lifetime;
    SomeDerivingObject obj;

    LifetimedPtr<SomeDerivingObject> ptr{valid_lifetime, &obj};

    ASSERT_FALSE(ptr.expired());
}

TEST(LifetimedPtr, can_upcast_to_a_base_class)
{
    SharedLifetimeBlock valid_lifetime;
    SomeDerivingObject obj;

    LifetimedPtr<SomeDerivingObject> ptr{valid_lifetime, &obj};
    [[maybe_unused]] LifetimedPtr<SomeBaseClass> base_ptr = ptr;
}

TEST(LifetimedPtr, when_upcasted_is_also_not_expired)
{
    SharedLifetimeBlock valid_lifetime;
    SomeDerivingObject obj;

    LifetimedPtr<SomeDerivingObject> ptr{valid_lifetime, &obj};
    ASSERT_FALSE(ptr.expired());
    LifetimedPtr<SomeBaseClass> base_ptr = ptr;
    ASSERT_FALSE(ptr.expired());
}

TEST(LifetimedPtr, upcasted_ptr_is_attached_to_same_lifetime_as_derived_ptr)
{
    LifetimedPtr<SomeBaseClass> base_ptr;

    ASSERT_FALSE(base_ptr);
    {
        SharedLifetimeBlock valid_lifetime;
        SomeDerivingObject obj;
        LifetimedPtr<SomeBaseClass> ptr{valid_lifetime, &obj};
        base_ptr = LifetimedPtr<SomeBaseClass>{ptr};
        ASSERT_FALSE(ptr.expired());
        ASSERT_FALSE(base_ptr.expired());
    }
    ASSERT_TRUE(base_ptr.expired());
}

TEST(LifetimePtr, reset_resets_the_ptr)
{
    SharedLifetimeBlock lifetime;
    SomeDerivingObject obj;
    LifetimedPtr<SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_FALSE(ptr.expired());
    ASSERT_TRUE(ptr);
    ptr.reset();
    ASSERT_TRUE(ptr.expired());
    ASSERT_FALSE(ptr);
}

TEST(LifetimePtr, get_returns_nullptr_for_nullptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_EQ(ptr, nullptr);
}

TEST(LifetimePtr, get_throws_if_non_nullptr_but_with_expired_lifetime)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    SomeDerivingObject obj; // doesn't matter if this is in-lifetime
    {
        SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<SomeDerivingObject>{lifetime, &obj};
    }
    ASSERT_TRUE(ptr.expired());
    ASSERT_ANY_THROW({ ptr.get(); });
}

TEST(LifetimedPtr, throws_if_called_on_non_expired_ptr)
{
    SharedLifetimeBlock lifetime;
    SomeDerivingObject obj;
    LifetimedPtr<SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_NO_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_asterisk_throws_if_called_on_nullptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_ANY_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_asterisk_throws_if_called_on_non_nullptr_with_expired_lifetime)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    SomeDerivingObject obj;
    {
        SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<SomeDerivingObject>{lifetime, &obj};
        ptr = LifetimedPtr<SomeDerivingObject>{ptr};
    }
    ASSERT_ANY_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_arrow_works_on_non_expired_ptr)
{
    SharedLifetimeBlock lifetime;
    SomeDerivingObject obj;
    LifetimedPtr<SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_NO_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, operator_arrow_throws_when_called_on_nullptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_ANY_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, operator_arrow_throws_when_called_on_expired_ptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    SomeDerivingObject obj;
    {
        SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<SomeDerivingObject>{lifetime, &obj};
    }
    ASSERT_ANY_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, equality_returns_expected_results)
{
    SharedLifetimeBlock first_lifetime;
    SharedLifetimeBlock second_lifetime;

    SomeDerivingObject first_obj;
    SomeDerivingObject second_obj;

    LifetimedPtr<SomeDerivingObject> default_constructed;
    LifetimedPtr<SomeDerivingObject> nullptr_constructed;
    LifetimedPtr<SomeDerivingObject> first_first{first_lifetime, &first_obj};
    LifetimedPtr<SomeDerivingObject> first_second{first_lifetime, &second_obj};
    LifetimedPtr<SomeDerivingObject> second_first{second_lifetime, &first_obj};
    LifetimedPtr<SomeDerivingObject> second_second{second_lifetime, &second_obj};

    ASSERT_EQ(default_constructed, default_constructed);
    ASSERT_EQ(default_constructed, nullptr_constructed);
    ASSERT_EQ(first_first, first_first);
    ASSERT_EQ(first_first, second_first) << "equality is only for the pointer";
    ASSERT_EQ(first_second, second_second) << "equality is only for the pointer";
    ASSERT_NE(default_constructed, first_first);
    ASSERT_NE(first_first, first_second);
    ASSERT_NE(first_first, second_second);
}
