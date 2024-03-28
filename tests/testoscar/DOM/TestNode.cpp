#include <oscar/DOM/Node.h>

/*

#include <oscar/Object/NodePath.h>
#include <oscar/Variant/Variant.h>

#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <string_view>
#include <utility>

using osc::literals::operator""_np;
using namespace std::literals::string_view_literals;

namespace
{
    int RandomInt()
    {
        static auto s_PRNG = std::default_random_engine{std::random_device{}()};
        return std::uniform_int_distribution{}(s_PRNG);
    }

    class MinimalNodeImpl : public osc::Node {
    public:
        friend bool operator==(MinimalNodeImpl const& lhs, MinimalNodeImpl const& rhs)
        {
            return lhs.m_Data == rhs.m_Data;
        }
    private:
        std::unique_ptr<Node> impl_clone() const override { return std::make_unique<MinimalNodeImpl>(*this); }

        int m_Data = RandomInt();
    };

    class DifferentMinimalNodeImpl final : public osc::Node {
    public:
        friend bool operator==(DifferentMinimalNodeImpl const& lhs, DifferentMinimalNodeImpl const& rhs)
        {
            return lhs.m_Data == rhs.m_Data;
        }
    private:
        std::unique_ptr<Node> impl_clone() const final { return std::make_unique<DifferentMinimalNodeImpl>(*this); }
        int m_Data = RandomInt();
    };

    class NodeWithCtorArg final : public osc::Node {
    public:
        explicit NodeWithCtorArg(int arg) : m_Arg{arg} {}
        int arg() const { return m_Arg; }
    private:
        std::unique_ptr<Node> impl_clone() const final { return std::make_unique<NodeWithCtorArg>(*this); }

        int m_Arg;
    };

    class DerivedFromMinimalNodeImpl final : public MinimalNodeImpl {
    public:
        friend bool operator==(DerivedFromMinimalNodeImpl const& lhs, DerivedFromMinimalNodeImpl const& rhs)
        {
            return
                static_cast<MinimalNodeImpl const&>(lhs) == static_cast<MinimalNodeImpl const&>(rhs) &&
                lhs.m_DerivedData == rhs.m_DerivedData;
        }
    private:
        std::unique_ptr<Node> impl_clone() const override { return std::make_unique<DerivedFromMinimalNodeImpl>(*this); }

        int m_DerivedData = RandomInt();
    };

    class NodeWithName final : public osc::Node {
    public:
        explicit NodeWithName(std::string_view name)
        {
            set_name(name);
        }
    private:
        std::unique_ptr<Node> impl_clone() const final { return std::make_unique<NodeWithName>(*this); }
    };
}

TEST(Node, CanDefaultConstructMinimalExample)
{
    ASSERT_NO_THROW({ MinimalNodeImpl{}; });
}

TEST(Node, CopyConstructionOfMinimalExampleWorksAsExpected)
{
    MinimalNodeImpl a;
    ASSERT_EQ(a, MinimalNodeImpl{a});
}

TEST(Node, MoveConstructionOfMinimalExampleDoesNotThrow)
{
    MinimalNodeImpl a;
    ASSERT_NO_THROW({ MinimalNodeImpl{std::move(a)}; });
}

TEST(Node, VirtualCloneOfMinimalExampleWorksAsExpected)
{
    MinimalNodeImpl a;
    auto cloned = a.clone();

    ASSERT_TRUE(dynamic_cast<MinimalNodeImpl*>(cloned.get()));
    ASSERT_EQ(dynamic_cast<MinimalNodeImpl&>(*cloned), a);
}

TEST(Node, NameOfMinimalExampleDefaultsToSomethingNonEmpty)
{
    ASSERT_FALSE(MinimalNodeImpl{}.name().empty());
}

TEST(Node, SetNameOfMinimalExampleWorksAsExpected)
{
    MinimalNodeImpl node;
    node.set_name("newName");
    ASSERT_EQ(node.name(), "newName"sv);
}

TEST(Node, GetParentOfMinimalExampleIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.parent());
}

TEST(Node, GetParentOfMinimalExampleTypedIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.parent<MinimalNodeImpl>());
}

TEST(Node, UpdParentOfMinimalExampleIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.upd_parent());
}

TEST(Node, UpdParentOfMinimalExampleTypedIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.upd_parent<MinimalNodeImpl>());
}

TEST(Node, GetNumChildrenOfMinimalExampleIsZero)
{
    ASSERT_EQ(MinimalNodeImpl{}.num_children(), 0);
}

TEST(Node, GetChildOfMinimalExampleReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.child(0));
    ASSERT_FALSE(MinimalNodeImpl{}.child(1));  // i.e. bounds-checked
}

TEST(Node, GetChildOfMinimalExampleTypedReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.child<MinimalNodeImpl>(0));
    ASSERT_FALSE(MinimalNodeImpl{}.child<MinimalNodeImpl>(1));  // i.e. bounds-checked
}

TEST(Node, UpdChildOfMinimalExampleReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.upd_child(0));
    ASSERT_FALSE(MinimalNodeImpl{}.upd_child(1));  // i.e. bounds-checked
}

TEST(Node, UpdChildOfMinimalExampleTypedReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.upd_child<MinimalNodeImpl>(0));
    ASSERT_FALSE(MinimalNodeImpl{}.upd_child<MinimalNodeImpl>(1));  // i.e. bounds-checked
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.parent(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentTypedReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.parent<MinimalNodeImpl>(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(child.parent<DifferentMinimalNodeImpl>());
}

TEST(Node, AddChildOfDifferentTypeToMinimalExampleFollowedByGetParentOfOriginalTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    DifferentMinimalNodeImpl& child = parent.add_child(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(child.parent<MinimalNodeImpl>());
}

TEST(Node, AddChildOfDerivedTypeToMinimalExampleFollowedByGetParentOfBaseTypeReturnsTruey)
{
    DerivedFromMinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(child.parent<MinimalNodeImpl>());
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.upd_parent(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentTypedReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.upd_parent<MinimalNodeImpl>(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(child.upd_parent<DifferentMinimalNodeImpl>());
}

TEST(Node, AddChildOfDifferentTypeToMinimalExampleFollowedByUpdParentOfOriginalTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    DifferentMinimalNodeImpl& child = parent.add_child(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(child.upd_parent<MinimalNodeImpl>());
}

TEST(Node, AddChildOfDerivedTypeToMinimalExampleFollowedByUpdParentOfBaseTypeReturnsTruey)
{
    DerivedFromMinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(child.upd_parent<MinimalNodeImpl>());
}

TEST(Node, AddChildToMinimalExampleFollowedByGetNumChildrenReturns1)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_EQ(child.num_children(), 1);
}

TEST(Node, AddChildDoesNotChangeChildsName)
{
    MinimalNodeImpl parent;
    auto const& child = [&parent]()
    {
        auto up = std::make_unique<MinimalNodeImpl>();
        up->set_name("newname");
        auto* ptr = up.get();
        parent.add_child(std::move(up));
        return *ptr;
    }();

    ASSERT_EQ(child.name(), "newname"sv);
}

TEST(Node, AddChildWithSameNameAsParentDoesNotChangeName)
{
    auto const name = "somename"sv;

    MinimalNodeImpl parent;
    parent.set_name(name);
    ASSERT_EQ(parent.name(), name);

    auto const& child = [&parent, &name]()
    {
        auto up = std::make_unique<MinimalNodeImpl>();
        up->set_name(name);
        auto* ptr = up.get();
        parent.add_child(std::move(up));
        return *ptr;
    }();

    ASSERT_EQ(parent.name(), child.name());
}

TEST(Node, AddChildFollowedByGetChildReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.child(0), &child);
}

TEST(Node, AddChildFollowedByGetChildTypedReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.child<MinimalNodeImpl>(0), &child);
}

TEST(Node, AddChildFollowedByGetChildOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(parent.child<DifferentMinimalNodeImpl>(0));
}

TEST(Node, AddDifferentChildFollowedByGetChildOfFirstTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(parent.child<MinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByGetDerivedClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.child<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByGetBaseClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.child<MinimalNodeImpl>(0));
}

TEST(Node, AddBaseChildFollowedByGetDerivedClassReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(parent.child<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddChildFollowedByUpdChildReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.upd_child(0), &child);
}

TEST(Node, AddChildFollowedByUpdChildTypedReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.upd_child<MinimalNodeImpl>(0), &child);
}

TEST(Node, AddChildFollowedByUpdChildOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(parent.upd_child<DifferentMinimalNodeImpl>(0));
}

TEST(Node, AddDifferentChildFollowedByUpdChildOfFirstTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(parent.upd_child<MinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByUpdDerivedClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.upd_child<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByUpdBaseClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.upd_child<MinimalNodeImpl>(0));
}

TEST(Node, AddBaseChildFollowedByUpdDerivedClassReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.add_child(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(parent.upd_child<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, EmplaceChildBehavesEssentiallyIdentiallyToAddChild)
{
    // test basic logic
    {
        MinimalNodeImpl parent;
        auto& child = parent.emplace_child<MinimalNodeImpl>();

        ASSERT_EQ(parent.num_children(), 1);
        ASSERT_EQ(parent.child(0), &child);
        ASSERT_FALSE(parent.child(1));
        ASSERT_EQ(parent.child<MinimalNodeImpl>(0), &child);
        ASSERT_FALSE(parent.child<MinimalNodeImpl>(1));
        ASSERT_EQ(parent.upd_child(0), &child);
        ASSERT_FALSE(parent.upd_child(1));
        ASSERT_EQ(parent.upd_child<MinimalNodeImpl>(0), &child);
        ASSERT_FALSE(parent.upd_child<MinimalNodeImpl>(1));
    }

    {
        DerivedFromMinimalNodeImpl parent;
        MinimalNodeImpl& child = parent.emplace_child<MinimalNodeImpl>();

        ASSERT_EQ(child.parent(), &parent);
        ASSERT_TRUE(child.parent<DerivedFromMinimalNodeImpl>());
    }

    {
        MinimalNodeImpl parent;
        auto& child = parent.emplace_child<DerivedFromMinimalNodeImpl>();

        ASSERT_EQ(child.parent(), &parent);
        ASSERT_TRUE(child.parent<MinimalNodeImpl>());
        ASSERT_FALSE(child.parent<DerivedFromMinimalNodeImpl>());
    }
}

TEST(Node, EmplaceChildForwardsArgs)
{
    int const arg = 1337;

    MinimalNodeImpl parent;
    auto& child = parent.emplace_child<NodeWithCtorArg>(arg);

    ASSERT_EQ(child.arg(), arg);
}

TEST(Node, RemoveChildReturnsFalseIfIndexOutOfBounds)
{
    ASSERT_FALSE(MinimalNodeImpl{}.remove_child(0));
}

TEST(Node, RemoveChildReturnsFalseIfGivenInvalidName)
{
    ASSERT_FALSE(MinimalNodeImpl{}.remove_child("invalidname"));
}

TEST(Node, RemoveChildReturnsFalseIfGivenInvalidNode)
{
    MinimalNodeImpl notAChild{};
    ASSERT_FALSE(MinimalNodeImpl{}.remove_child(notAChild));
}

TEST(Node, RemoveChildByIndexReturnsTrueIfChildExists)
{
    MinimalNodeImpl parent;
    parent.emplace_child<MinimalNodeImpl>();

    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_TRUE(parent.remove_child(0));
    ASSERT_EQ(parent.num_children(), 0);
}

TEST(Node, RemoveChildByIndexReturnsFalseIfItHasChildrenButIsMisIndexed)
{
    MinimalNodeImpl parent;
    parent.emplace_child<MinimalNodeImpl>();
    ASSERT_FALSE(parent.remove_child(1));
}

TEST(Node, RemoveChildByReferenceReturnsTrueIfChildExists)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplace_child<MinimalNodeImpl>();

    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_TRUE(parent.remove_child(child));
    ASSERT_EQ(parent.num_children(), 0);

    // careful: child is now dead
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfReferenceIsntInTree)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl someOtherNode;

    ASSERT_FALSE(parent.remove_child(someOtherNode));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfReferenceIsParentRatherThanChild)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplace_child<MinimalNodeImpl>();

    ASSERT_FALSE(child.remove_child(parent));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfOtherNodeIsASibling)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplace_child<MinimalNodeImpl>();
    auto& sibling = parent.emplace_child<MinimalNodeImpl>();

    ASSERT_EQ(parent.num_children(), 2);
    ASSERT_FALSE(child.remove_child(sibling));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfNodeIsDeeperDescendent)
{
    MinimalNodeImpl grandparent;
    auto& parent = grandparent.emplace_child<MinimalNodeImpl>();
    auto& child = parent.emplace_child<MinimalNodeImpl>();

    ASSERT_EQ(grandparent.num_children(), 1);
    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_EQ(child.num_children(), 0);
    {
        ASSERT_FALSE(grandparent.remove_child(child));
    }
    ASSERT_EQ(child.num_children(), 0);
    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_EQ(grandparent.num_children(), 1);

    ASSERT_TRUE(parent.remove_child(child));
    ASSERT_EQ(grandparent.num_children(), 1);
    ASSERT_EQ(parent.num_children(), 0);
}

TEST(Node, RemoveChildByNameReturnsFalseyIfGivenNonExistentChildName)
{
    auto const notAName = "somenode"sv;

    MinimalNodeImpl parent;
    parent.emplace_child<MinimalNodeImpl>();

    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_FALSE(parent.remove_child(notAName));
    ASSERT_EQ(parent.num_children(), 1);
}

TEST(Node, RemoveChildByNameReturnsTrueyAndRemovesNodeIfGivenAChildName)
{
    auto const name = "somenode"sv;

    MinimalNodeImpl parent;
    parent.emplace_child<NodeWithName>(name);

    ASSERT_EQ(parent.num_children(), 1);
    ASSERT_TRUE(parent.remove_child(name));
    ASSERT_EQ(parent.num_children(), 0);
}

TEST(Node, SetNameOfChildCausesGetChildByNameToWorkWithNewName)
{
    auto const newName = "somenewname"sv;

    MinimalNodeImpl parent;
    auto& child = parent.emplace_child<MinimalNodeImpl>();
    std::string const oldName{child.name()};

    ASSERT_NE(child.name(), newName);
    ASSERT_TRUE(parent.child(oldName));
    child.set_name(newName);
    ASSERT_FALSE(parent.child(oldName));
    ASSERT_TRUE(parent.child(newName));
}

TEST(Node, GetAbsolutePathOfRootNodeReturnsForwardSlash)
{
    MinimalNodeImpl root;
    ASSERT_EQ(root.absolute_path(), "/"sv);
}

TEST(Node, GetAbsolutePathOfRootWithChildStillReturnsForwardSlash)
{
    MinimalNodeImpl root;
    root.emplace_child<MinimalNodeImpl>();
    ASSERT_EQ(root.num_children(), 1);
    ASSERT_EQ(root.absolute_path(), "/"sv);
}

TEST(Node, GetAbsolutePathOfChildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& b = a.emplace_child<NodeWithName>("b");

    ASSERT_EQ(b.absolute_path(), "/a/b"sv);
}

TEST(Node, GetAbsolutePathOfGrandchildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& c = a.emplace_child<NodeWithName>("b").emplace_child<NodeWithName>("c");

    ASSERT_EQ(c.absolute_path(), "/a/b/c"sv);
}

TEST(Node, GetAbsolutePathToGreatGrandchildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& d = a.emplace_child<NodeWithName>("b").emplace_child<NodeWithName>("c").emplace_child<NodeWithName>("d");

    ASSERT_EQ(d.absolute_path(), "/a/b/c/d"sv);
}

TEST(Node, GetAbsolutePathToRootChangesIfNameIsChanged)
{
    NodeWithName root{"root"};
    root.set_name("new");

    ASSERT_EQ(root.absolute_path(), "/new"sv);
}

TEST(Node, GetAbsolutePathToChildChangesIfChildNameIsChanged)
{
    NodeWithName a{"a"};
    auto& child = a.emplace_child<NodeWithName>("b");

    child.set_name("new");
    ASSERT_EQ(child.absolute_path(), "/a/new"sv);
}

TEST(Node, GetAbsolutePathToChildChangesIfRootNameIsChanged)
{
    NodeWithName a{"a"};
    auto& child = a.emplace_child<NodeWithName>("b");
    a.set_name("new");

    ASSERT_EQ(child.absolute_path(), "/new/b"sv);
}

TEST(Node, FindReturnsRootIfPassedRootPath)
{
    NodeWithName a{"a"};

    ASSERT_EQ(a.find("/"_np), &a);
}

TEST(Node, FindReturnsRootIfPassedIdentityPath)
{
    NodeWithName a{"a"};
    ASSERT_EQ(a.find("."_np), &a);
}

TEST(Node, FindReturnsExpectedPathsForSingleChild)
{
    NodeWithName a{"a"};
    auto& b = a.emplace_child<NodeWithName>("b");

    ASSERT_EQ(a.find("b"_np), &b);
    ASSERT_EQ(a.find("/b"_np), &b);
    ASSERT_EQ(a.find("/b/.."_np), &a);
    ASSERT_EQ(b.find("."_np), &b);
    ASSERT_EQ(b.find(".."_np), &a);
    ASSERT_EQ(b.find("../b"_np), &b);
    ASSERT_EQ(b.find("/a"_np), &a);
    ASSERT_EQ(b.find("/b"_np), &b);
    ASSERT_EQ(b.find("/"_np), &a);
    ASSERT_EQ(b.find("/b/.."_np), &a);
}

TEST(Node, FindReturnsExpectedPathsForGrandchild)
{
    NodeWithName a{"a"};
    auto& b = a.emplace_child<NodeWithName>("b");
    auto& c = b.emplace_child<NodeWithName>("c");

    struct TestCase final {
        TestCase(
            std::string_view inputPath_,
            NodeWithName* resultFromA_,
            NodeWithName* resultFromB_,
            NodeWithName* resultFromC_) :

            inputPath{inputPath_},
            resultFromA{resultFromA_},
            resultFromB{resultFromB_},
            resultFromC{resultFromC_}
        {
        }

        std::string_view inputPath;
        NodeWithName* resultFromA;
        NodeWithName* resultFromB;
        NodeWithName* resultFromC;
    };

    auto const testCases = osc::to_array<TestCase>(
    {
        //                      | from a  | from b  | from c  |
        {""                     , nullptr , nullptr , nullptr },
        {"/"                    ,   &a    ,   &a    ,   &a    },
        {"/////"                ,   &a    ,   &a    ,   &a    },  // see: NodePath tests: should collapse multiple slashes
        {"."                    ,   &a    ,   &b    ,   &c    },
        {"./."                  ,   &a    ,   &a    ,   &a    },
        {".."                   ,   &a    ,   &a    ,   &b    },
        {"../."                 ,   &a    ,   &a    ,   &b    },
        {"../.."                ,   &a    ,   &a    ,   &a    },
        {"../../b"              ,   &b    ,   &b    ,   &b    },
        {"../../b/"             ,   &b    ,   &b    ,   &b    },  // see: NodePath tests: should ignore trailing slashes
        {"../../c"              , nullptr , nullptr , nullptr },
        {"../../c/"             , nullptr , nullptr , nullptr },
        {".././b/.."            ,   &a    ,   &a    , nullptr },
        {"/.././././../"        ,   &a    ,   &a    ,   &a    },
        {"/.././././.."         ,   &a    ,   &a    ,   &a    },
        {"/./b/../b/./c"        ,   &c    ,   &c    ,   &c    },
        {"/./b/../b/bullshit/c" , nullptr , nullptr , nullptr },
        {"../bullshit/.."       , nullptr , nullptr , nullptr },
        {"bullshit/../../"      , nullptr , nullptr , nullptr },
        {"a"                    , nullptr , nullptr , nullptr },
        {"/a"                   , nullptr , nullptr , nullptr },
        {"/b/a"                 , nullptr , nullptr , nullptr },
        {"/b/../a"              , nullptr , nullptr , nullptr },
        {"b"                    ,   &b    , nullptr , nullptr },
        {"/b"                   ,   &b    ,   &b    ,   &b    },
        {"b/a"                  , nullptr , nullptr , nullptr },
        {"/b/a"                 , nullptr , nullptr , nullptr },
        {"b/.."                 ,   &a    , nullptr , nullptr },
        {"/b/../"               ,   &a    ,   &a    ,   &a    },
        {"b/c/.."               ,   &b    , nullptr , nullptr },
        {"/b/c/../."            ,   &b    ,   &b    ,   &b    },
        {"c"                    , nullptr ,   &c    , nullptr },
        {"/c"                   , nullptr , nullptr , nullptr },
        {"c/."                  , nullptr ,   &c    , nullptr },
        {"c/.."                 , nullptr ,   &b    , nullptr },
    });

    for (TestCase const& tc : testCases)
    {
        osc::NodePath const p{tc.inputPath};
        ASSERT_EQ(a.find(p), tc.resultFromA);
        ASSERT_EQ(b.find(p), tc.resultFromB);
        ASSERT_EQ(c.find(p), tc.resultFromC);
    }
}

TEST(Node, GetNumPropertiesReturnsZeroForMinimalExample)
{
    ASSERT_EQ(MinimalNodeImpl{}.num_properties(), 0);
}

TEST(Node, HasPropertyReturnsFalseForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.hasProperty("something"));
}

TEST(Node, GetPropertyNameByIndexReturnsFalseyForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getPropertyName(0));
    ASSERT_FALSE(MinimalNodeImpl{}.getPropertyName(1));
}

TEST(Node, GetPropertyValueByIndexReturnsFalseyForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.property_value_or_throw(0));
    ASSERT_FALSE(MinimalNodeImpl{}.property_value_or_throw(1));
}

TEST(Node, GetPropertyValueByNameReturnsFalseyForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.property_value_or_throw("something"));
}

TEST(Node, SetPropertyValueByIndexReturnsFalseForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.set_property_value_or_throw(0, osc::Variant(true)));
}

TEST(Node, SetPropertyValueByNameReturnsFalseForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.set_property_value_or_throw("someprop", osc::Variant(true)));
}

*/
