#include <oscar/Platform/ResourceLoader.hpp>

#include <gtest/gtest.h>
#include <oscar/Platform/IResourceLoader.hpp>
#include <oscar/Platform/ResourcePath.hpp>
#include <oscar/Platform/ResourceStream.hpp>

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
        ResourceStream implOpen(ResourcePath const& p) override
        {
            m_State->lastOpenCallArg = p;
            return ResourceStream{};
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
