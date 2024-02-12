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
            std::shared_ptr<UndoableModelStatePair const> targetModel,
            std::function<OpenSim::Object const*()> objectGetter
        );
        ObjectPropertiesEditor(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor const&) = delete;
        ObjectPropertiesEditor& operator=(ObjectPropertiesEditor&&) noexcept;
        ~ObjectPropertiesEditor() noexcept;

        // does not actually apply any property changes - the caller should check+apply the return value
        [[nodiscard]] std::optional<ObjectPropertyEdit> onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
