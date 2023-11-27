#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>

#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <cstddef>
#include <functional>
#include <utility>

namespace osc
{
    // an associative identifier that points to a specific part of a TPS document
    //
    // (handy for selection logic etc.)
    struct TPSDocumentElementID final {

        TPSDocumentElementID(
            TPSDocumentInputIdentifier whichInput_,
            TPSDocumentInputElementType elementType_,
            StringName elementID_) :

            whichInput{whichInput_},
            elementType{elementType_},
            elementID{std::move(elementID_)}
        {
        }

        friend bool operator==(TPSDocumentElementID const&, TPSDocumentElementID const&) = default;

        TPSDocumentInputIdentifier whichInput;
        TPSDocumentInputElementType elementType;
        StringName elementID;
    };
}

template<>
struct std::hash<osc::TPSDocumentElementID> final {
    size_t operator()(osc::TPSDocumentElementID const& el) const
    {
        return osc::HashOf(el.whichInput, el.elementType, el.elementID);
    }
};
