#include <oscar_demos/OscarDemosTabRegistry.h>

#include <oscar/oscar.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

namespace rgs = std::ranges;
using namespace osc;

namespace
{
    TabRegistry get_all_tab_entries()
    {
        TabRegistry r;
        register_demo_tabs(r);
        return r;
    }

    std::unique_ptr<App> g_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    class RegisteredDemoTabsFixture : public testing::TestWithParam<TabRegistryEntry> {
    protected:
        static void SetUpTestSuite()
        {
            g_app = std::make_unique<App>();
        }

        static void TearDownTestSuite()
        {
            g_app.reset();
        }
    };
}

INSTANTIATE_TEST_SUITE_P(
    RegisteredDemoTabsTest,
    RegisteredDemoTabsFixture,
    testing::ValuesIn(get_all_tab_entries()),
    [](const testing::TestParamInfo<TabRegistryEntry>& info)
    {
        std::string rv{info.param.name()};
        rgs::replace(rv, '/', '_');
        return rv;
    }
);

TEST_P(RegisteredDemoTabsFixture, Check)
{
    g_app->show<TabTestingScreen>(GetParam());
}
