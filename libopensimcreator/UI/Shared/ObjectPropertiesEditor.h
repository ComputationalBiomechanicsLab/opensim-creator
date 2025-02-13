#pragma once

#include <libopensimcreator/Documents/Model/ObjectPropertyEdit.h>

#include <liboscar/Platform/Widget.h>

#include <functional>
#include <memory>
#include <optional>

namespace OpenSim { class Object; }
namespace osc { class IVersionedComponentAccessor; }
namespace osc { class Widget; }

namespace osc
{
    class ObjectPropertiesEditor final : public Widget {
    public:
        explicit ObjectPropertiesEditor(
            Widget* parent,
            std::shared_ptr<const IVersionedComponentAccessor> targetComponent,
            std::function<const OpenSim::Object*()> objectGetter
        );

        void insertInBlacklist(std::string_view propertyName);

        // does not actually apply any property changes - the caller should check+apply the return value
        std::optional<ObjectPropertyEdit> onDraw();

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
