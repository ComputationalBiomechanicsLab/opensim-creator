#pragma once

namespace osc::mi { class MeshImporterUILayer; }

namespace osc::mi
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

        void requestPop(MeshImporterUILayer& layer)
        {
            implRequestPop(layer);
        }

    private:
        virtual void implRequestPop(MeshImporterUILayer&) = 0;
    };
}
