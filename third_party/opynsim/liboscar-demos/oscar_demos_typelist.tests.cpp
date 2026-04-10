#include "oscar_demos_typelist.h"

#include <liboscar/platform/app.h>
#include <liboscar/ui/screens/widget_testing_screen.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

namespace rgs = std::ranges;
using namespace osc;

namespace
{
    class NamedWidget final {
    public:
        template<typename TWidget>
        requires (requires { TWidget::id(); })
        static NamedWidget create()
        {
            return NamedWidget{TWidget::id(), [](Widget* owner) { return std::make_unique<TWidget>(owner); }};
        }

        const auto& name() const { return name_; }
        std::unique_ptr<Widget> construct(Widget* owner) const { return constructor_(owner); }
    private:
        template<typename StringLike, typename Constructor>
        requires (std::constructible_from<std::string, StringLike> and std::invocable<Constructor, Widget*>)
        NamedWidget(StringLike&& name, Constructor&& constructor) :
            name_{std::forward<StringLike>(name)},
            constructor_{std::forward<Constructor>(constructor)}
        {}

        std::string name_;
        std::function<std::unique_ptr<Widget>(Widget*)> constructor_;
    };

    std::vector<NamedWidget> all_demos()
    {
        std::vector<NamedWidget> rv;
        [&rv]<typename... Tabs>(Typelist<Tabs...>)
        {
            static_assert(sizeof...(Tabs) > 0);
            (rv.push_back(NamedWidget::create<Tabs>()), ...);
        }(OscarDemosTypelist{});
        return rv;
    }

    std::unique_ptr<App> g_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    class RegisteredDemoTabsFixture : public testing::TestWithParam<NamedWidget> {
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
    testing::ValuesIn(all_demos()),
    [](const testing::TestParamInfo<NamedWidget>& info)
    {
        std::string rv{info.param.name()};
        rgs::replace(rv, '/', '_');
        return rv;
    }
);

TEST_P(RegisteredDemoTabsFixture, Check)
{
    g_app->show<WidgetTestingScreen>(GetParam().construct(nullptr));
}
