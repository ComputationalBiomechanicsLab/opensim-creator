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
        std::optional<ResourcePath> last_open_call_path;
    };

    class MockResourceLoader : public IResourceLoader {
    public:
        explicit MockResourceLoader(std::shared_ptr<MockState> state_) :
            state_{std::move(state_)}
        {}
    private:
        ResourceStream impl_open(const ResourcePath& resource_path) override
        {
            state_->last_open_call_path = resource_path;
            return ResourceStream{};
        }

        std::function<std::optional<ResourceDirectoryEntry>()> impl_iterate_directory(const ResourcePath&) override
        {
            return []() { return std::nullopt; };
        }

        std::shared_ptr<MockState> state_;
    };
}

TEST(ResourceLoader, inplace_constructor_works_as_intended)
{
    const auto mock_state = std::make_shared<MockState>();
    const ResourcePath resource_path{"some/path"};

    ResourceLoader resource_loader = make_resource_loader<MockResourceLoader>(mock_state);
    resource_loader.open(resource_path);

    ASSERT_EQ(mock_state->last_open_call_path, resource_path);
}

TEST(ResourceLoader, WithPrefixCausesIResourceLoaderToBeCalledWithPrefixedPath)
{
    const auto mock_state = std::make_shared<MockState>();

    ResourceLoader resource_loader = make_resource_loader<MockResourceLoader>(mock_state);
    ResourceLoader prefixed_loader = resource_loader.with_prefix("prefix");

    resource_loader.open(ResourcePath{"path"});
    ASSERT_EQ(mock_state->last_open_call_path, ResourcePath{"path"}) << "with_prefix doesn't affect original ResourceLoader";
    prefixed_loader.open(ResourcePath{"path"});
    ASSERT_EQ(mock_state->last_open_call_path, ResourcePath{"prefix/path"}) << "with_prefix should return a loader the prefixes each open call";
}
