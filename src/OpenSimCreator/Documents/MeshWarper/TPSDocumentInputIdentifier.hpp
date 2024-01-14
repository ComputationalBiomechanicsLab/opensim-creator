#pragma once

namespace osc
{
    // identifies one of the two inputs (source/destination) of the TPS document
    enum class TPSDocumentInputIdentifier {
        Source,
        Destination,
        NUM_OPTIONS,
    };
}
