#pragma once

namespace osc { class MeshImporterUILayer; }

namespace osc
{
    // the "parent" thing that is hosting the layer
    class IMeshImporterUILayerHost {
    protected:
        IMeshImporterUILayerHost() = default;
        IMeshImporterUILayerHost(const IMeshImporterUILayerHost&) = default;
        IMeshImporterUILayerHost(IMeshImporterUILayerHost&&) noexcept = default;
        IMeshImporterUILayerHost& operator=(const IMeshImporterUILayerHost&) = default;
        IMeshImporterUILayerHost& operator=(IMeshImporterUILayerHost&&) noexcept = default;
    public:
        virtual ~IMeshImporterUILayerHost() noexcept = default;

        void requestPop(MeshImporterUILayer& layer)
        {
            implRequestPop(layer);
        }

    private:
        virtual void implRequestPop(MeshImporterUILayer&) = 0;
    };
}
