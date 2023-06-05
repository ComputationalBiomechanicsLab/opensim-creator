#include "ModelSceneDecorationsParams.hpp"

#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/VirtualConstModelStatePair.hpp"

osc::ModelSceneDecorationsParams::ModelSceneDecorationsParams() = default;
osc::ModelSceneDecorationsParams::ModelSceneDecorationsParams(
    osc::VirtualConstModelStatePair const& msp_,
    osc::CustomDecorationOptions const& decorationOptions_,
    osc::CustomRenderingOptions const& renderingOptions_) :

    modelVersion{msp_.getModelVersion()},
    stateVersion{msp_.getStateVersion()},
    selection{GetAbsolutePathOrEmpty(msp_.getSelected())},
    hover{GetAbsolutePathOrEmpty(msp_.getHovered())},
    fixupScaleFactor{msp_.getFixupScaleFactor()},
    decorationOptions{decorationOptions_},
    renderingOptions{renderingOptions_}
{
}

bool osc::operator==(ModelSceneDecorationsParams const& a, ModelSceneDecorationsParams const& b) noexcept
{
    return
        a.modelVersion == b.modelVersion &&
        a.stateVersion == b.stateVersion &&
        a.selection == b.selection &&
        a.hover == b.hover &&
        a.fixupScaleFactor == b.fixupScaleFactor &&
        a.decorationOptions == b.decorationOptions &&
        a.renderingOptions == b.renderingOptions;
}
bool osc::operator!=(ModelSceneDecorationsParams const& a, ModelSceneDecorationsParams const& b) noexcept
{
    return !(a == b);
}