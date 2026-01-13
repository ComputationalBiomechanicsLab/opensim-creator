#include "Widget.h"

#include <liboscar/platform/WidgetPrivate.h>
#include <liboscar/utils/LifetimedPtr.h>

using namespace osc;

osc::Widget::Widget(Widget* parent) :
    data_{std::make_unique<WidgetPrivate>(*this, parent)}
{}
osc::Widget::~Widget() noexcept = default;

Widget* osc::Widget::parent() { return data_->parent(); }
const Widget* osc::Widget::parent() const { return data_->parent(); }
void osc::Widget::set_parent(Widget* new_parent) { data_->set_parent(new_parent); }

LifetimedPtr<Widget> osc::Widget::weak_ref()
{
    return {private_data().lifetime_watcher(), this};
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
