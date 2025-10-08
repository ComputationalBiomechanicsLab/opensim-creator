#include "OpenSimCreatorTabRegistry.h"

#include <libopensimcreator/Platform/OpenSimCreatorApp.h>

#include <gtest/gtest.h>
#include <liboscar/UI/Screens/TabTestingScreen.h>
#include <liboscar/UI/Tabs/TabRegistry.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

namespace {
    const TabRegistry c_Tabs = []()
    {
        TabRegistry r;
        RegisterOpenSimCreatorTabs(r);
        return r;
    }();

    const std::vector<std::string> c_TabNames = []()
    {
        std::vector<std::string> rv;
        rv.reserve(c_Tabs.size());
        for (size_t i = 0; i < c_Tabs.size(); ++i) {
            rv.emplace_back(c_Tabs[i].name());
        }
        return rv;
    }();

    std::unique_ptr<OpenSimCreatorApp> g_App;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    class RegisteredOpenSimCreatorTabsFixture : public testing::TestWithParam<std::string> {
    protected:
        static void SetUpTestSuite()
        {
            g_App = std::make_unique<OpenSimCreatorApp>();
        }

        static void TearDownTestSuite()
        {
            g_App.reset();
        }
    };
}

INSTANTIATE_TEST_SUITE_P(
    RegisteredOpenSimCreatorTabsTest,
    RegisteredOpenSimCreatorTabsFixture,
    testing::ValuesIn(c_TabNames)
);

TEST_P(RegisteredOpenSimCreatorTabsFixture, Check)
{
    std::string s = GetParam();
    if (auto entry = c_Tabs.find_by_name(s)) {
        g_App->show<TabTestingScreen>(*entry);
    }
    else {
        throw std::runtime_error{"cannot find tab in registry"};
    }
}
