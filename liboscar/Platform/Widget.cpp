#include "Widget.h"

#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/Utils/LifetimedPtr.h>

using namespace osc;

osc::Widget::Widget() :
    data_{std::make_unique<WidgetPrivate>(*this, nullptr)}
{}
osc::Widget::~Widget() noexcept = default;

Widget* osc::Widget::parent() { return data_->parent(); }
const Widget* osc::Widget::parent() const { return data_->parent(); }

LifetimedPtr<Widget> osc::Widget::weak_ref()
{
    return {private_data().lifetime(), this};
}

CStringView osc::Widget::name() const
{
    return private_data().name();
}

void osc::Widget::set_name(std::string_view name)
{
    private_data().set_name(name);
}

osc::Widget::Widget(std::unique_ptr<WidgetPrivate>&& data) :
    data_{std::move(data)}
{}
