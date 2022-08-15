#pragma once

#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class AddComponentPopup final {
    public:
        AddComponentPopup(std::shared_ptr<UndoableModelStatePair>,
                          std::unique_ptr<OpenSim::Component> prototype,
                          std::string_view popupName);
        AddComponentPopup(AddComponentPopup const&) = delete;
        AddComponentPopup(AddComponentPopup&&) noexcept;
        AddComponentPopup& operator=(AddComponentPopup const&) = delete;
        AddComponentPopup& operator=(AddComponentPopup&&) noexcept;
        ~AddComponentPopup() noexcept;

        void open();
        void close();
        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
