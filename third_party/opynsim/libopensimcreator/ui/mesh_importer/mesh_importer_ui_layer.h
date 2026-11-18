#pragma once

#include <libopensimcreator/ui/mesh_importer/mesh_importer_ui_layer_host.h>

// UI layering support
//
// the visualizer can push the 3D visualizer into different modes (here, "layers") that
// have different behavior. E.g.:
//
// - normal mode (editing stuff)
// - picking another body in the scene mode
namespace osc
{
    class Event;

    // a layer that is hosted by the parent
    class MeshImporterUILayer {
    protected:
        explicit MeshImporterUILayer(MeshImporterUILayerHost& parent) :
            parent_{&parent}
        {}
        MeshImporterUILayer(const MeshImporterUILayer&) = default;
        MeshImporterUILayer(MeshImporterUILayer&&) noexcept = default;
        MeshImporterUILayer& operator=(const MeshImporterUILayer&) = default;
        MeshImporterUILayer& operator=(MeshImporterUILayer&&) noexcept = default;

    public:
        virtual ~MeshImporterUILayer() noexcept = default;

        bool on_event(Event& e) { return impl_on_event(e); }

        void tick(float dt) { impl_tick(dt); }

        void on_draw() { impl_on_draw(); }

    protected:
        void request_pop() { parent_->request_pop(*this); }

    private:
        virtual bool impl_on_event(Event&) = 0;
        virtual void impl_tick(float) = 0;
        virtual void impl_on_draw() = 0;

        MeshImporterUILayerHost* parent_;
    };
}
