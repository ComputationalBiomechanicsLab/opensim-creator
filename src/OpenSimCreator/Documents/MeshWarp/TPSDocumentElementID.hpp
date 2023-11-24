#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>

#include <oscar/Utils/HashHelpers.hpp>

#include <cstddef>
#include <functional>
#include <string>
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
            std::string elementID_) :

            whichInput{whichInput_},
            elementType{elementType_},
            elementID{std::move(elementID_)}
        {
        }

        friend bool operator==(TPSDocumentElementID const&, TPSDocumentElementID const&) = default;

        TPSDocumentInputIdentifier whichInput;
        TPSDocumentInputElementType elementType;
        std::string elementID;
    };
}

template<>
struct std::hash<osc::TPSDocumentElementID> final {
    size_t operator()(osc::TPSDocumentElementID const& el) const
    {
        return osc::HashOf(el.whichInput, el.elementType, el.elementID);
    }
};
