#include <oscar_learnopengl/LearnOpenGLTabRegistry.hpp>

#include <oscar/oscar.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

namespace {
    TabRegistry c_LearnOpenGLTabs = []()
    {
        TabRegistry r;
        RegisterLearnOpenGLTabs(r);
        return r;
    }();

    std::vector<std::string> c_LearnOpenGLTabNames = []()
    {
        std::vector<std::string> rv;
        rv.reserve(c_LearnOpenGLTabs.size());
        for (size_t i = 0; i < c_LearnOpenGLTabs.size(); ++i) {
            rv.emplace_back(c_LearnOpenGLTabs[i].getName());
        }
        return rv;
    }();

    std::unique_ptr<App> g_App;
    class RegisteredLearnOpenGLTabsFixture : public testing::TestWithParam<std::string> {
    protected:
        static void SetUpTestSuite()
        {
            g_App = std::make_unique<App>(AppMetadata{"someorg", "testoscar_learnopengl"});
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
    testing::ValuesIn(c_LearnOpenGLTabNames)
);

TEST_P(RegisteredLearnOpenGLTabsFixture, Check)
{
    std::string s = GetParam();
    if (auto entry = c_LearnOpenGLTabs.getByName(s)) {
        g_App->show<TabTestingScreen>(*entry);
    }
    else {
        throw std::runtime_error{"cannot find tab in registry"};
    }
}
