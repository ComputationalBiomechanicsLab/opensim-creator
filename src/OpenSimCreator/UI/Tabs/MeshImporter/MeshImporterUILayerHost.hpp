#pragma once

namespace osc { class MeshImporterLayer; }

namespace osc
{
    // the "parent" thing that is hosting the layer
    class MeshImporterUILayerHost {
    protected:
        MeshImporterUILayerHost() = default;
        MeshImporterUILayerHost(MeshImporterUILayerHost const&) = default;
        MeshImporterUILayerHost(MeshImporterUILayerHost&&) noexcept = default;
        MeshImporterUILayerHost& operator=(MeshImporterUILayerHost const&) = default;
        MeshImporterUILayerHost& operator=(MeshImporterUILayerHost&&) noexcept = default;
    public:
        virtual ~MeshImporterUILayerHost() noexcept = default;

        void requestPop(MeshImporterLayer& layer)
        {
            implRequestPop(layer);
        }

    private:
        virtual void implRequestPop(MeshImporterLayer&) = 0;
    };
}
