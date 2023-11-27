#pragma once

namespace osc
{
    // identifies one of the two inputs (source/destination) of the TPS document
    enum class TPSDocumentInputIdentifier {
        Source = 0,
        Destination,
        NUM_OPTIONS,
    };
}
