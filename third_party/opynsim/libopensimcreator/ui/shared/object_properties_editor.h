#pragma once

#include <libopynsim/documents/model/object_property_edit.h>

#include <liboscar/platform/widget.h>

#include <functional>
#include <memory>
#include <optional>

namespace OpenSim
{
    class Object;
}

namespace opyn
{
    class VersionedComponentAccessor;
}

namespace osc
{
    class Widget;

    class ObjectPropertiesEditor final : public Widget {
    public:
        explicit ObjectPropertiesEditor(
            Widget* parent,
            std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
            std::function<const OpenSim::Object*()> object_getter);

        void insert_in_blacklist(std::string_view property_name);

        // does not actually apply any property changes - the caller should check+apply the return
        // value
        std::optional<ObjectPropertyEdit> on_draw();

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
