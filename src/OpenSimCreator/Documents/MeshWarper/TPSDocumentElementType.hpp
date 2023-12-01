#pragma once

namespace osc
{
    // identifies a specific part of the input of the TPS document
    enum class TPSDocumentElementType {
        Landmark = 0,
        NonParticipatingLandmark,
        NUM_OPTIONS,
    };
}
