#pragma once

namespace osc
{
    class MeshImporterUILayer;

    // the "parent" thing that is hosting the layer
    class MeshImporterUILayerHost {
    protected:
        MeshImporterUILayerHost() = default;
        MeshImporterUILayerHost(const MeshImporterUILayerHost&) = default;
        MeshImporterUILayerHost(MeshImporterUILayerHost&&) noexcept = default;
        MeshImporterUILayerHost& operator=(const MeshImporterUILayerHost&) = default;
        MeshImporterUILayerHost& operator=(MeshImporterUILayerHost&&) noexcept = default;

    public:
        virtual ~MeshImporterUILayerHost() noexcept = default;

        void request_pop(MeshImporterUILayer& layer) { impl_request_pop(layer); }

    private:
        virtual void impl_request_pop(MeshImporterUILayer&) = 0;
    };
}
