#include <oscar_learnopengl/LearnOpenGLTabRegistry.h>

#include <oscar/oscar.h>

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

namespace {
    TabRegistry const c_Tabs = []()
    {
        TabRegistry r;
        RegisterLearnOpenGLTabs(r);
        return r;
    }();

    std::vector<std::string> const c_TabNames = []()
    {
        std::vector<std::string> rv;
        rv.reserve(c_Tabs.size());
        for (size_t i = 0; i < c_Tabs.size(); ++i) {
            rv.emplace_back(c_Tabs[i].getName());
        }
        return rv;
    }();

    std::unique_ptr<App> g_App;
    class RegisteredLearnOpenGLTabsFixture : public testing::TestWithParam<std::string> {
    protected:
        static void SetUpTestSuite()
        {
            g_App = std::make_unique<App>();
        }

        static void TearDownTestSuite()
        {
            g_App.reset();
        }
    };
}

INSTANTIATE_TEST_SUITE_P(
    RegisteredLearnOpenGLTabsTest,
    RegisteredLearnOpenGLTabsFixture,
    testing::ValuesIn(c_TabNames)
);

TEST_P(RegisteredLearnOpenGLTabsFixture, Check)
{
    std::string s = GetParam();
    if (auto entry = c_Tabs.getByName(s)) {
        g_App->show<TabTestingScreen>(*entry);
    }
    else {
        throw std::runtime_error{"cannot find tab in registry"};
    }
}
