#include "Widget.h"

#include <oscar/Platform/Event.h>
#include <oscar/Platform/WidgetPrivate.h>
#include <oscar/Utils/LifetimedPtr.h>

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

osc::Widget::Widget(std::unique_ptr<WidgetPrivate>&& data) :
    data_{std::move(data)}
{}
