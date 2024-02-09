#pragma once

#include <OpenSimCreator/UI/MeshImporter/IMeshImporterUILayerHost.h>

#include <SDL_events.h>

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
        MeshImporterUILayer(MeshImporterUILayer const&) = default;
        MeshImporterUILayer(MeshImporterUILayer&&) noexcept = default;
        MeshImporterUILayer& operator=(MeshImporterUILayer const&) = default;
        MeshImporterUILayer& operator=(MeshImporterUILayer&&) noexcept = default;
    public:
        virtual ~MeshImporterUILayer() noexcept = default;

        bool onEvent(SDL_Event const& e)
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
        virtual bool implOnEvent(SDL_Event const&) = 0;
        virtual void implTick(float) = 0;
        virtual void implOnDraw() = 0;

        IMeshImporterUILayerHost* m_Parent;
    };
}
