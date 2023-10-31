#include <oscar/DOM/Node.hpp>

#include <oscar/DOM/NodePath.hpp>
#include <oscar/Utils/Variant.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <string_view>
#include <utility>

/*

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
        std::unique_ptr<Node> implClone() const override { return std::make_unique<MinimalNodeImpl>(*this); }

        int m_Data = RandomInt();
    };

    class DifferentMinimalNodeImpl final : public osc::Node {
    public:
        friend bool operator==(DifferentMinimalNodeImpl const& lhs, DifferentMinimalNodeImpl const& rhs)
        {
            return lhs.m_Data == rhs.m_Data;
        }
    private:
        std::unique_ptr<Node> implClone() const final { return std::make_unique<DifferentMinimalNodeImpl>(*this); }
        int m_Data = RandomInt();
    };

    class NodeWithCtorArg final : public osc::Node {
    public:
        explicit NodeWithCtorArg(int arg) : m_Arg{arg} {}
        int arg() const { return m_Arg; }
    private:
        std::unique_ptr<Node> implClone() const final { return std::make_unique<NodeWithCtorArg>(*this); }

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
        std::unique_ptr<Node> implClone() const override { return std::make_unique<DerivedFromMinimalNodeImpl>(*this); }

        int m_DerivedData = RandomInt();
    };

    class NodeWithName final : public osc::Node {
    public:
        explicit NodeWithName(std::string_view name)
        {
            setName(name);
        }
    private:
        std::unique_ptr<Node> implClone() const final { return std::make_unique<NodeWithName>(*this); }
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
    ASSERT_FALSE(MinimalNodeImpl{}.getName().empty());
}

TEST(Node, SetNameOfMinimalExampleWorksAsExpected)
{
    MinimalNodeImpl node;
    node.setName("newName");
    ASSERT_EQ(node.getName(), "newName"sv);
}

TEST(Node, GetParentOfMinimalExampleIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getParent());
}

TEST(Node, GetParentOfMinimalExampleTypedIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getParent<MinimalNodeImpl>());
}

TEST(Node, UpdParentOfMinimalExampleIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.updParent());
}

TEST(Node, UpdParentOfMinimalExampleTypedIsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.updParent<MinimalNodeImpl>());
}

TEST(Node, GetNumChildrenOfMinimalExampleIsZero)
{
    ASSERT_EQ(MinimalNodeImpl{}.getNumChildren(), 0);
}

TEST(Node, GetChildOfMinimalExampleReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getChild(0));
    ASSERT_FALSE(MinimalNodeImpl{}.getChild(1));  // i.e. bounds-checked
}

TEST(Node, GetChildOfMinimalExampleTypedReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getChild<MinimalNodeImpl>(0));
    ASSERT_FALSE(MinimalNodeImpl{}.getChild<MinimalNodeImpl>(1));  // i.e. bounds-checked
}

TEST(Node, UpdChildOfMinimalExampleReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.updChild(0));
    ASSERT_FALSE(MinimalNodeImpl{}.updChild(1));  // i.e. bounds-checked
}

TEST(Node, UpdChildOfMinimalExampleTypedReturnsFalsey)
{
    ASSERT_FALSE(MinimalNodeImpl{}.updChild<MinimalNodeImpl>(0));
    ASSERT_FALSE(MinimalNodeImpl{}.updChild<MinimalNodeImpl>(1));  // i.e. bounds-checked
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.getParent(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentTypedReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.getParent<MinimalNodeImpl>(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByGetParentOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(child.getParent<DifferentMinimalNodeImpl>());
}

TEST(Node, AddChildOfDifferentTypeToMinimalExampleFollowedByGetParentOfOriginalTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    DifferentMinimalNodeImpl& child = parent.addChild(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(child.getParent<MinimalNodeImpl>());
}

TEST(Node, AddChildOfDerivedTypeToMinimalExampleFollowedByGetParentOfBaseTypeReturnsTruey)
{
    DerivedFromMinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(child.getParent<MinimalNodeImpl>());
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.updParent(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentTypedReturnsTheParent)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(child.updParent<MinimalNodeImpl>(), &parent);
}

TEST(Node, AddChildToMinimalExampleFollowedByUpdParentOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(child.updParent<DifferentMinimalNodeImpl>());
}

TEST(Node, AddChildOfDifferentTypeToMinimalExampleFollowedByUpdParentOfOriginalTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    DifferentMinimalNodeImpl& child = parent.addChild(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(child.updParent<MinimalNodeImpl>());
}

TEST(Node, AddChildOfDerivedTypeToMinimalExampleFollowedByUpdParentOfBaseTypeReturnsTruey)
{
    DerivedFromMinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(child.updParent<MinimalNodeImpl>());
}

TEST(Node, AddChildToMinimalExampleFollowedByGetNumChildrenReturns1)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_EQ(child.getNumChildren(), 1);
}

TEST(Node, AddChildDoesNotChangeChildsName)
{
    MinimalNodeImpl parent;
    auto const& child = [&parent]()
    {
        auto up = std::make_unique<MinimalNodeImpl>();
        up->setName("newname");
        auto* ptr = up.get();
        parent.addChild(std::move(up));
        return *ptr;
    }();

    ASSERT_EQ(child.getName(), "newname"sv);
}

TEST(Node, AddChildWithSameNameAsParentDoesNotChangeName)
{
    auto const name = "somename"sv;

    MinimalNodeImpl parent;
    parent.setName(name);
    ASSERT_EQ(parent.getName(), name);

    auto const& child = [&parent, &name]()
    {
        auto up = std::make_unique<MinimalNodeImpl>();
        up->setName(name);
        auto* ptr = up.get();
        parent.addChild(std::move(up));
        return *ptr;
    }();

    ASSERT_EQ(parent.getName(), child.getName());
}

TEST(Node, AddChildFollowedByGetChildReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.getChild(0), &child);
}

TEST(Node, AddChildFollowedByGetChildTypedReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.getChild<MinimalNodeImpl>(0), &child);
}

TEST(Node, AddChildFollowedByGetChildOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(parent.getChild<DifferentMinimalNodeImpl>(0));
}

TEST(Node, AddDifferentChildFollowedByGetChildOfFirstTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(parent.getChild<MinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByGetDerivedClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.getChild<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByGetBaseClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.getChild<MinimalNodeImpl>(0));
}

TEST(Node, AddBaseChildFollowedByGetDerivedClassReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(parent.getChild<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddChildFollowedByUpdChildReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.updChild(0), &child);
}

TEST(Node, AddChildFollowedByUpdChildTypedReturnsChild)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl& child = parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_EQ(parent.updChild<MinimalNodeImpl>(0), &child);
}

TEST(Node, AddChildFollowedByUpdChildOfDifferentTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_FALSE(parent.updChild<DifferentMinimalNodeImpl>(0));
}

TEST(Node, AddDifferentChildFollowedByUpdChildOfFirstTypeReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DifferentMinimalNodeImpl>());

    ASSERT_FALSE(parent.updChild<MinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByUpdDerivedClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.updChild<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, AddDerivedChildFollowedByUpdBaseClassReturnsTruey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<DerivedFromMinimalNodeImpl>());

    ASSERT_TRUE(parent.updChild<MinimalNodeImpl>(0));
}

TEST(Node, AddBaseChildFollowedByUpdDerivedClassReturnsFalsey)
{
    MinimalNodeImpl parent;
    parent.addChild(std::make_unique<MinimalNodeImpl>());

    ASSERT_TRUE(parent.updChild<DerivedFromMinimalNodeImpl>(0));
}

TEST(Node, EmplaceChildBehavesEssentiallyIdentiallyToAddChild)
{
    // test basic logic
    {
        MinimalNodeImpl parent;
        auto& child = parent.emplaceChild<MinimalNodeImpl>();

        ASSERT_EQ(parent.getNumChildren(), 1);
        ASSERT_EQ(parent.getChild(0), &child);
        ASSERT_FALSE(parent.getChild(1));
        ASSERT_EQ(parent.getChild<MinimalNodeImpl>(0), &child);
        ASSERT_FALSE(parent.getChild<MinimalNodeImpl>(1));
        ASSERT_EQ(parent.updChild(0), &child);
        ASSERT_FALSE(parent.updChild(1));
        ASSERT_EQ(parent.updChild<MinimalNodeImpl>(0), &child);
        ASSERT_FALSE(parent.updChild<MinimalNodeImpl>(1));
    }

    {
        DerivedFromMinimalNodeImpl parent;
        MinimalNodeImpl& child = parent.emplaceChild<MinimalNodeImpl>();

        ASSERT_EQ(child.getParent(), &parent);
        ASSERT_TRUE(child.getParent<DerivedFromMinimalNodeImpl>());
    }

    {
        MinimalNodeImpl parent;
        auto& child = parent.emplaceChild<DerivedFromMinimalNodeImpl>();

        ASSERT_EQ(child.getParent(), &parent);
        ASSERT_TRUE(child.getParent<MinimalNodeImpl>());
        ASSERT_FALSE(child.getParent<DerivedFromMinimalNodeImpl>());
    }
}

TEST(Node, EmplaceChildForwardsArgs)
{
    int const arg = 1337;

    MinimalNodeImpl parent;
    auto& child = parent.emplaceChild<NodeWithCtorArg>(arg);

    ASSERT_EQ(child.arg(), arg);
}

TEST(Node, RemoveChildReturnsFalseIfIndexOutOfBounds)
{
    ASSERT_FALSE(MinimalNodeImpl{}.removeChild(0));
}

TEST(Node, RemoveChildReturnsFalseIfGivenInvalidName)
{
    ASSERT_FALSE(MinimalNodeImpl{}.removeChild("invalidname"));
}

TEST(Node, RemoveChildReturnsFalseIfGivenInvalidNode)
{
    MinimalNodeImpl notAChild{};
    ASSERT_FALSE(MinimalNodeImpl{}.removeChild(notAChild));
}

TEST(Node, RemoveChildByIndexReturnsTrueIfChildExists)
{
    MinimalNodeImpl parent;
    parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_TRUE(parent.removeChild(0));
    ASSERT_EQ(parent.getNumChildren(), 0);
}

TEST(Node, RemoveChildByIndexReturnsFalseIfItHasChildrenButIsMisIndexed)
{
    MinimalNodeImpl parent;
    parent.emplaceChild<MinimalNodeImpl>();
    ASSERT_FALSE(parent.removeChild(1));
}

TEST(Node, RemoveChildByReferenceReturnsTrueIfChildExists)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_TRUE(parent.removeChild(child));
    ASSERT_EQ(parent.getNumChildren(), 0);

    // careful: child is now dead
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfReferenceIsntInTree)
{
    MinimalNodeImpl parent;
    MinimalNodeImpl someOtherNode;

    ASSERT_FALSE(parent.removeChild(someOtherNode));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfReferenceIsParentRatherThanChild)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_FALSE(child.removeChild(parent));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfOtherNodeIsASibling)
{
    MinimalNodeImpl parent;
    auto& child = parent.emplaceChild<MinimalNodeImpl>();
    auto& sibling = parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_EQ(parent.getNumChildren(), 2);
    ASSERT_FALSE(child.removeChild(sibling));
}

TEST(Node, RemoveChildByReferenceReturnsFalseIfNodeIsDeeperDescendent)
{
    MinimalNodeImpl grandparent;
    auto& parent = grandparent.emplaceChild<MinimalNodeImpl>();
    auto& child = parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_EQ(grandparent.getNumChildren(), 1);
    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_EQ(child.getNumChildren(), 0);
    {
        ASSERT_FALSE(grandparent.removeChild(child));
    }
    ASSERT_EQ(child.getNumChildren(), 0);
    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_EQ(grandparent.getNumChildren(), 1);

    ASSERT_TRUE(parent.removeChild(child));
    ASSERT_EQ(grandparent.getNumChildren(), 1);
    ASSERT_EQ(parent.getNumChildren(), 0);
}

TEST(Node, RemoveChildByNameReturnsFalseyIfGivenNonExistentChildName)
{
    auto const notAName = "somenode"sv;

    MinimalNodeImpl parent;
    parent.emplaceChild<MinimalNodeImpl>();

    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_FALSE(parent.removeChild(notAName));
    ASSERT_EQ(parent.getNumChildren(), 1);
}

TEST(Node, RemoveChildByNameReturnsTrueyAndRemovesNodeIfGivenAChildName)
{
    auto const name = "somenode"sv;

    MinimalNodeImpl parent;
    parent.emplaceChild<NodeWithName>(name);

    ASSERT_EQ(parent.getNumChildren(), 1);
    ASSERT_TRUE(parent.removeChild(name));
    ASSERT_EQ(parent.getNumChildren(), 0);
}

TEST(Node, SetNameOfChildCausesGetChildByNameToWorkWithNewName)
{
    auto const newName = "somenewname"sv;

    MinimalNodeImpl parent;
    auto& child = parent.emplaceChild<MinimalNodeImpl>();
    std::string const oldName{child.getName()};

    ASSERT_NE(child.getName(), newName);
    ASSERT_TRUE(parent.getChild(oldName));
    child.setName(newName);
    ASSERT_FALSE(parent.getChild(oldName));
    ASSERT_TRUE(parent.getChild(newName));
}

TEST(Node, GetAbsolutePathOfRootNodeReturnsForwardSlash)
{
    MinimalNodeImpl root;
    ASSERT_EQ(root.getAbsolutePath(), "/"sv);
}

TEST(Node, GetAbsolutePathOfRootWithChildStillReturnsForwardSlash)
{
    MinimalNodeImpl root;
    root.emplaceChild<MinimalNodeImpl>();
    ASSERT_EQ(root.getNumChildren(), 1);
    ASSERT_EQ(root.getAbsolutePath(), "/"sv);
}

TEST(Node, GetAbsolutePathOfChildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& b = a.emplaceChild<NodeWithName>("b");

    ASSERT_EQ(b.getAbsolutePath(), "/a/b"sv);
}

TEST(Node, GetAbsolutePathOfGrandchildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& c = a.emplaceChild<NodeWithName>("b").emplaceChild<NodeWithName>("c");

    ASSERT_EQ(c.getAbsolutePath(), "/a/b/c"sv);
}

TEST(Node, GetAbsolutePathToGreatGrandchildReturnsExpectedAbsolutePath)
{
    NodeWithName a{"a"};
    auto const& d = a.emplaceChild<NodeWithName>("b").emplaceChild<NodeWithName>("c").emplaceChild<NodeWithName>("d");

    ASSERT_EQ(d.getAbsolutePath(), "/a/b/c/d"sv);
}

TEST(Node, GetAbsolutePathToRootChangesIfNameIsChanged)
{
    NodeWithName root{"root"};
    root.setName("new");

    ASSERT_EQ(root.getAbsolutePath(), "/new"sv);
}

TEST(Node, GetAbsolutePathToChildChangesIfChildNameIsChanged)
{
    NodeWithName a{"a"};
    auto& child = a.emplaceChild<NodeWithName>("b");

    child.setName("new");
    ASSERT_EQ(child.getAbsolutePath(), "/a/new"sv);
}

TEST(Node, GetAbsolutePathToChildChangesIfRootNameIsChanged)
{
    NodeWithName a{"a"};
    auto& child = a.emplaceChild<NodeWithName>("b");
    a.setName("new");

    ASSERT_EQ(child.getAbsolutePath(), "/new/b"sv);
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
    auto& b = a.emplaceChild<NodeWithName>("b");

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
    auto& b = a.emplaceChild<NodeWithName>("b");
    auto& c = b.emplaceChild<NodeWithName>("c");

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
    ASSERT_EQ(MinimalNodeImpl{}.getNumProperties(), 0);
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
    ASSERT_FALSE(MinimalNodeImpl{}.getPropertyValue(0));
    ASSERT_FALSE(MinimalNodeImpl{}.getPropertyValue(1));
}

TEST(Node, GetPropertyValueByNameReturnsFalseyForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.getPropertyValue("something"));
}

TEST(Node, SetPropertyValueByIndexReturnsFalseForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.setPropertyValue(0, osc::Variant(true)));
}

TEST(Node, SetPropertyValueByNameReturnsFalseForMinimalExample)
{
    ASSERT_FALSE(MinimalNodeImpl{}.setPropertyValue("someprop", osc::Variant(true)));
}

*/
