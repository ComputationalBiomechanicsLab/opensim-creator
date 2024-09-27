#include "Widget.h"

#include <oscar/Platform/WidgetPrivate.h>
#include <oscar/Utils/LifetimedPtr.h>

using namespace osc;

osc::Widget::~Widget() noexcept = default;
osc::Widget::Widget(Widget&&) noexcept = default;
Widget& osc::Widget::operator=(Widget&&) = default;

osc::Widget::Widget(std::unique_ptr<WidgetPrivate>&& data) :
    data_{std::move(data)}
{}

LifetimedPtr<Widget> osc::Widget::weak_ref()
{
    return {private_data().lifetime(), this};
}
