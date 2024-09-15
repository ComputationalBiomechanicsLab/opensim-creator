#pragma once

#include <OpenSimCreator/UI/MeshImporter/IMeshImporterUILayerHost.h>

namespace osc { class Event; }

// UI layering support
//
// the visualizer can push the 3D visualizer into different modes (here, "layers") that
// have different behavior. E.g.:
//
// - normal mode (editing stuff)
// - picking another body in the scene mode
namespace osc::mi
{
    // a layer that is hosted by the parent
    class MeshImporterUILayer {
    protected:
        explicit MeshImporterUILayer(IMeshImporterUILayerHost& parent) : m_Parent{&parent}
        {
        }
        MeshImporterUILayer(const MeshImporterUILayer&) = default;
        MeshImporterUILayer(MeshImporterUILayer&&) noexcept = default;
        MeshImporterUILayer& operator=(const MeshImporterUILayer&) = default;
        MeshImporterUILayer& operator=(MeshImporterUILayer&&) noexcept = default;
    public:
        virtual ~MeshImporterUILayer() noexcept = default;

        bool onEvent(const Event& e)
        {
            return implOnEvent(e);
        }

        void tick(float dt)
        {
            implTick(dt);
        }

        void onDraw()
        {
            implOnDraw();
        }

    protected:
        void requestPop()
        {
            m_Parent->requestPop(*this);
        }

    private:
        virtual bool implOnEvent(const Event&) = 0;
        virtual void implTick(float) = 0;
        virtual void implOnDraw() = 0;

        IMeshImporterUILayerHost* m_Parent;
    };
}
