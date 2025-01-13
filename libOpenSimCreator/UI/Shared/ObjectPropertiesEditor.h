#pragma once

#include <libOpenSimCreator/Documents/Model/ObjectPropertyEdit.h>

#include <functional>
#include <memory>
#include <optional>

namespace OpenSim { class Object; }
namespace osc { class IVersionedComponentAccessor; }
namespace osc { class Widget; }

namespace osc
{
    class ObjectPropertiesEditor final {
    public:
        explicit ObjectPropertiesEditor(
            Widget* parentWidget,
            std::shared_ptr<const IVersionedComponentAccessor> targetComponent,
            std::function<const OpenSim::Object*()> objectGetter
        );
        ObjectPropertiesEditor(const ObjectPropertiesEditor&) = delete;
        ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept;
        ObjectPropertiesEditor& operator=(const ObjectPropertiesEditor&) = delete;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor&&) noexcept;
        ~ObjectPropertiesEditor() noexcept;

        void insertInBlacklist(std::string_view propertyName);

        // does not actually apply any property changes - the caller should check+apply the return value
        std::optional<ObjectPropertyEdit> onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
