#include "open_sim_creator_tab_registry.h"

#include <libopensimcreator/platform/open_sim_creator_app.h>

#include <gtest/gtest.h>
#include <liboscar/ui/screens/tab_testing_screen.h>
#include <liboscar/ui/tabs/tab_registry.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    const TabRegistry& get_tabs()
    {
        static const TabRegistry s_tabs = [] {
            TabRegistry r;
            register_open_sim_creator_tabs(r);
            return r;
        }();
        return s_tabs;
    }

    const std::vector<std::string>& get_tab_names()
    {
        static const std::vector<std::string> s_tab_names = [] {
            const TabRegistry& tabs = get_tabs();
            std::vector<std::string> rv;
            rv.reserve(tabs.size());
            for (size_t i = 0; i < tabs.size(); ++i) {
                rv.emplace_back(tabs[i].name());
            }
            return rv;
        }();
        return s_tab_names;
    }

    // NOLINTNEXTLINE
    std::unique_ptr<OpenSimCreatorApp> g_app;
    class RegisteredOpenSimCreatorTabsFixture : public testing::TestWithParam<std::string> {
    protected:
        static void SetUpTestSuite() { g_app = std::make_unique<OpenSimCreatorApp>(); }

        static void TearDownTestSuite() { g_app.reset(); }
    };
}

INSTANTIATE_TEST_SUITE_P(
    RegisteredOpenSimCreatorTabsTest,
    RegisteredOpenSimCreatorTabsFixture,
    testing::ValuesIn(get_tab_names()));

TEST_P(RegisteredOpenSimCreatorTabsFixture, Check)
{
    std::string s = GetParam();
    if (auto entry = get_tabs().find_by_name(s)) {
        g_app->show<TabTestingScreen>(*entry);
    } else {
        throw std::runtime_error{"cannot find tab in registry"};
    }
}
