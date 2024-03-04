#include <OpenSimCreator/Documents/ModelWarper/InMemoryMesh.h>

#include <gtest/gtest.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

using namespace osc;

TEST(InMemoryMesh, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ InMemoryMesh instance; });
}

TEST(InMemoryMesh, DefaultConstructedEmitsABlankMesh)
{
    OpenSim::Model model;
    InMemoryMesh& mesh = AddComponent<InMemoryMesh>(model);
    mesh.connectSocket_frame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);
    SimTK::State& state = InitializeState(model);

    SimTK::Array_<SimTK::DecorativeGeometry> decs;
    mesh.generateDecorations(true, model.getDisplayHints(), state, decs);

    ASSERT_EQ(decs.size(), 1);
    SimTK::DecorativeGeometry const& dec = decs.front();

    class Visitor : public SimTK::DecorativeGeometryImplementation {
    public:
        int mesh_count() const { return m_NumMeshes; }
        int other_count() const { return m_NumOther; }
        int vert_count() const { return m_NumVerts; }
        int face_count() const { return m_NumFaces; }
    private:
        void implementPointGeometry(const SimTK::DecorativePoint&) override { ++m_NumOther; }
        void implementLineGeometry(const SimTK::DecorativeLine&) override { ++m_NumOther; }
        void implementBrickGeometry(const SimTK::DecorativeBrick&) override { ++m_NumOther; }
        void implementCylinderGeometry(const SimTK::DecorativeCylinder&) override { ++m_NumOther; }
        void implementCircleGeometry(const SimTK::DecorativeCircle&) override { ++m_NumOther; }
        void implementSphereGeometry(const SimTK::DecorativeSphere&) override { ++m_NumOther; }
        void implementEllipsoidGeometry(const SimTK::DecorativeEllipsoid&) override { ++m_NumOther; }
        void implementFrameGeometry(const SimTK::DecorativeFrame&) override { ++m_NumOther; }
        void implementTextGeometry(const SimTK::DecorativeText&) override { ++m_NumOther; }
        void implementMeshGeometry(const SimTK::DecorativeMesh& m) override
        {
            ++m_NumMeshes;
            m_NumVerts += m.getMesh().getNumVertices();
            m_NumFaces += m.getMesh().getNumFaces();
        }
        void implementMeshFileGeometry(const SimTK::DecorativeMeshFile&) override { ++m_NumOther; }
        void implementTorusGeometry(const SimTK::DecorativeTorus&) override { ++m_NumOther; }
        void implementArrowGeometry(const SimTK::DecorativeArrow&) override { ++m_NumOther; }
        void implementConeGeometry(const SimTK::DecorativeCone&) override { ++m_NumOther; }

        int m_NumMeshes = 0;
        int m_NumVerts = 0;
        int m_NumFaces = 0;
        int m_NumOther = 0;
    };

    Visitor v;
    dec.implementGeometry(v);

    ASSERT_EQ(v.mesh_count(), 1);
    ASSERT_EQ(v.other_count(), 0);
    ASSERT_EQ(v.vert_count(), 0);
    ASSERT_EQ(v.face_count(), 0);
}

TEST(InMemoryMesh, CanConstructFromOSCMesh)
{

}
