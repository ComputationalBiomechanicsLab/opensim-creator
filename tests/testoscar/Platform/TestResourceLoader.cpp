#include <oscar/Platform/ResourceLoader.h>

#include <gtest/gtest.h>
#include <oscar/Platform/IResourceLoader.h>
#include <oscar/Platform/ResourceDirectoryEntry.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

using namespace osc;

namespace
{
    struct MockState final {
        std::optional<ResourcePath> lastOpenCallArg;
    };

    class MockResourceLoader : public IResourceLoader {
    public:
        explicit MockResourceLoader(std::shared_ptr<MockState> state_) :
            m_State{std::move(state_)}
        {}
    private:
        ResourceStream impl_open(const ResourcePath& p) override
        {
            m_State->lastOpenCallArg = p;
            return ResourceStream{};
        }

        std::function<std::optional<ResourceDirectoryEntry>()> impl_iterate_directory(const ResourcePath&) override
        {
            return []() { return std::nullopt; };
        }

        std::shared_ptr<MockState> m_State;
    };
}

TEST(ResourceLoader, InplaceConstructorWorksAsIntended)
{
    auto state = std::make_shared<MockState>();
    ResourcePath p{"some/path"};

    ResourceLoader rl = make_resource_loader<MockResourceLoader>(state);
    rl.open(p);

    ASSERT_EQ(state->lastOpenCallArg, p);
}

TEST(ResourceLoader, WithPrefixCausesIResourceLoaderToBeCalledWithPrefixedPath)
{
    auto state = std::make_shared<MockState>();

    ResourceLoader rl = make_resource_loader<MockResourceLoader>(state);
    ResourceLoader prefixedLoader = rl.with_prefix("prefix");

    rl.open(ResourcePath{"path"});
    ASSERT_EQ(state->lastOpenCallArg, ResourcePath{"path"}) << "with_prefix doesn't affect original ResourceLoader";
    prefixedLoader.open(ResourcePath{"path"});
    ASSERT_EQ(state->lastOpenCallArg, ResourcePath{"prefix/path"}) << "with_prefix should return a loader the prefixes each open call";
}
