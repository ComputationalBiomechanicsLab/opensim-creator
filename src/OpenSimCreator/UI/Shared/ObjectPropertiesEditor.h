#pragma once

#include <OpenSimCreator/Documents/Model/ObjectPropertyEdit.h>

#include <functional>
#include <memory>
#include <optional>

namespace OpenSim { class Object; }
namespace osc { class IPopupAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ObjectPropertiesEditor final {
    public:
        ObjectPropertiesEditor(
            IPopupAPI*,
            std::shared_ptr<const UndoableModelStatePair> targetModel,
            std::function<const OpenSim::Object*()> objectGetter
        );
        ObjectPropertiesEditor(const ObjectPropertiesEditor&) = delete;
        ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept;
        ObjectPropertiesEditor& operator=(const ObjectPropertiesEditor&) = delete;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor&&) noexcept;
        ~ObjectPropertiesEditor() noexcept;

        // does not actually apply any property changes - the caller should check+apply the return value
        [[nodiscard]] std::optional<ObjectPropertyEdit> onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
