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
    [[maybe_unused]] const LifetimedPtr<const SomeDerivingObject> ptr;
}

TEST(LifetimedPtr, default_constructed_implicitly_converts_to_false)
{
    const LifetimedPtr<const SomeDerivingObject> ptr;
    ASSERT_FALSE(ptr);
}

TEST(LifetimedPtr, default_constructed_get_returns_nullptr)
{
    const LifetimedPtr<const SomeDerivingObject> ptr;
    ASSERT_EQ(ptr.get(), nullptr);
}

TEST(LifetimedPtr, can_be_constructed_from_nullptr)
{
    [[maybe_unused]] const LifetimedPtr<const SomeDerivingObject> ptr{nullptr};
}

TEST(LifetimedPtr, nullptr_constructed_implicitly_converts_to_false)
{
    const LifetimedPtr<const SomeDerivingObject> ptr{nullptr};
    ASSERT_FALSE(ptr);
}

TEST(LifetimedPtr, nullptr_constructed_get_returns_nullptr)
{
    const LifetimedPtr<const SomeDerivingObject> ptr{nullptr};
    ASSERT_EQ(ptr.get(), nullptr);
}

TEST(LifetimedPtr, when_constructed_with_expired_lifetime_produced_expired_ptr)
{
    const AlwaysExpiredLifetime expired_lifetime;
    const SomeDerivingObject obj;

    const LifetimedPtr<const SomeDerivingObject> ptr{expired_lifetime, &obj};

    ASSERT_TRUE(ptr.expired());
}

TEST(LifetimedPtr, when_constructed_with_in_life_lifetime_produces_not_expired_ptr)
{
    const SharedLifetimeBlock valid_lifetime;
    const SomeDerivingObject obj;

    const LifetimedPtr<const SomeDerivingObject> ptr{valid_lifetime, &obj};

    ASSERT_FALSE(ptr.expired());
}

TEST(LifetimedPtr, can_upcast_to_a_base_class)
{
    const SharedLifetimeBlock valid_lifetime;
    const SomeDerivingObject obj;

    const LifetimedPtr<const SomeDerivingObject> ptr{valid_lifetime, &obj};
    [[maybe_unused]] const LifetimedPtr<const SomeBaseClass> base_ptr = ptr;
}

TEST(LifetimedPtr, when_upcasted_is_also_not_expired)
{
    const SharedLifetimeBlock valid_lifetime;
    const SomeDerivingObject obj;

    const LifetimedPtr<const SomeDerivingObject> ptr{valid_lifetime, &obj};
    ASSERT_FALSE(ptr.expired());
    const LifetimedPtr<const SomeBaseClass> base_ptr = ptr;
    ASSERT_FALSE(ptr.expired());
}

TEST(LifetimedPtr, upcasted_ptr_is_attached_to_same_lifetime_as_derived_ptr)
{
    LifetimedPtr<const SomeBaseClass> base_ptr;

    ASSERT_FALSE(base_ptr);
    {
        const SharedLifetimeBlock valid_lifetime;
        const SomeDerivingObject obj;
        const LifetimedPtr<const SomeBaseClass> ptr{valid_lifetime, &obj};
        base_ptr = LifetimedPtr<const SomeBaseClass>{ptr};
        ASSERT_FALSE(ptr.expired());
        ASSERT_FALSE(base_ptr.expired());
    }
    ASSERT_TRUE(base_ptr.expired());
}

TEST(LifetimePtr, reset_resets_the_ptr)
{
    const SharedLifetimeBlock lifetime;
    const SomeDerivingObject obj;
    LifetimedPtr<const SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_FALSE(ptr.expired());
    ASSERT_TRUE(ptr);
    ptr.reset();
    ASSERT_TRUE(ptr.expired());
    ASSERT_FALSE(ptr);
}

TEST(LifetimePtr, get_returns_nullptr_for_nullptr)
{
    const LifetimedPtr<const SomeDerivingObject> ptr;
    ASSERT_EQ(ptr, nullptr);
}

TEST(LifetimePtr, get_throws_if_non_nullptr_but_with_expired_lifetime)
{
    LifetimedPtr<const SomeDerivingObject> ptr;
    const SomeDerivingObject obj; // doesn't matter if this is in-lifetime
    {
        const SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<const SomeDerivingObject>{lifetime, &obj};
    }
    ASSERT_TRUE(ptr.expired());
    ASSERT_ANY_THROW({ ptr.get(); });
}

TEST(LifetimedPtr, throws_if_called_on_non_expired_ptr)
{
    const SharedLifetimeBlock lifetime;
    const SomeDerivingObject obj;
    const LifetimedPtr<const SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_NO_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_asterisk_throws_if_called_on_nullptr)
{
    const LifetimedPtr<const SomeDerivingObject> ptr;
    ASSERT_ANY_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_asterisk_throws_if_called_on_non_nullptr_with_expired_lifetime)
{
    LifetimedPtr<const SomeDerivingObject> ptr;
    const SomeDerivingObject obj;
    {
        const SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<const SomeDerivingObject>{lifetime, &obj};
        ptr = LifetimedPtr<const SomeDerivingObject>{ptr};
    }
    ASSERT_ANY_THROW({ *ptr; });
}

TEST(LifetimedPtr, operator_arrow_works_on_non_expired_ptr)
{
    const SharedLifetimeBlock lifetime;
    SomeDerivingObject obj;
    const LifetimedPtr<SomeDerivingObject> ptr{lifetime, &obj};
    ASSERT_NO_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, operator_arrow_throws_when_called_on_nullptr)
{
    const LifetimedPtr<SomeDerivingObject> ptr;
    ASSERT_ANY_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, operator_arrow_throws_when_called_on_expired_ptr)
{
    LifetimedPtr<SomeDerivingObject> ptr;
    SomeDerivingObject obj;
    {
        const SharedLifetimeBlock lifetime;
        ptr = LifetimedPtr<SomeDerivingObject>{lifetime, &obj};
    }
    ASSERT_ANY_THROW({ ptr->some_method(); });
}

TEST(LifetimedPtr, equality_returns_expected_results)
{
    const SharedLifetimeBlock first_lifetime;
    const SharedLifetimeBlock second_lifetime;

    const SomeDerivingObject first_obj;
    const SomeDerivingObject second_obj;

    const LifetimedPtr<const SomeDerivingObject> default_constructed;
    const LifetimedPtr<const SomeDerivingObject> nullptr_constructed;
    const LifetimedPtr<const SomeDerivingObject> first_first{first_lifetime, &first_obj};
    const LifetimedPtr<const SomeDerivingObject> first_second{first_lifetime, &second_obj};
    const LifetimedPtr<const SomeDerivingObject> second_first{second_lifetime, &first_obj};
    const LifetimedPtr<const SomeDerivingObject> second_second{second_lifetime, &second_obj};

    ASSERT_EQ(default_constructed, default_constructed);
    ASSERT_EQ(default_constructed, nullptr_constructed);
    ASSERT_EQ(first_first, first_first);
    ASSERT_EQ(first_first, second_first) << "equality is only for the pointer";
    ASSERT_EQ(first_second, second_second) << "equality is only for the pointer";
    ASSERT_NE(default_constructed, first_first);
    ASSERT_NE(first_first, first_second);
    ASSERT_NE(first_first, second_second);
}
