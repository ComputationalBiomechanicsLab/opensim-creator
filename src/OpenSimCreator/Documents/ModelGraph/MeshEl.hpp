#pragma once

#include <OpenSimCreator/Documents/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneElClass.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneElCRTP.hpp>

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a mesh in the scene
    //
    // In this mesh importer, meshes are always positioned + oriented in ground. At OpenSim::Model generation
    // time, the implementation does necessary maths to attach the meshes into the Model in the relevant relative
    // coordinate system.
    //
    // The reason the editor uses ground-based coordinates is so that users have freeform control over where
    // the mesh will be positioned in the model, and so that the user can freely re-attach the mesh and freely
    // move meshes/bodies/joints in the mesh importer without everything else in the scene moving around (which
    // is what would happen in a relative topology-sensitive attachment graph).
    class MeshEl final : public SceneElCRTP<MeshEl> {
    public:
        MeshEl(
            UID id,
            UID attachment,  // can be ModelGraphIDs::Ground()
            Mesh meshData,
            std::filesystem::path path
        );

        Mesh const& getMeshData() const
        {
            return m_MeshData;
        }

        std::filesystem::path const& getPath() const
        {
            return m_Path;
        }

        UID getParentID() const
        {
            return m_Attachment;
        }

        void setParentID(UID newParent)
        {
            m_Attachment = newParent;
        }

        Transform getXForm() const
        {
            return m_Transform;
        }

        void setXform(Transform const& t)
        {
            m_Transform = t;
        }

        AABB calcBounds() const;

    private:
        friend class SceneElCRTP<MeshEl>;
        static SceneElClass CreateClass();

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final;

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            if (i != 0)
            {
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
            m_Attachment = id;
        }

        SceneElFlags implGetFlags() const final
        {
            return
                SceneElFlags::CanChangeLabel |
                SceneElFlags::CanChangePosition |
                SceneElFlags::CanChangeRotation |
                SceneElFlags::CanChangeScale |
                SceneElFlags::CanDelete |
                SceneElFlags::CanSelect |
                SceneElFlags::HasPhysicalSize;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final;

        Transform implGetXform(ISceneElLookup const&) const final
        {
            return getXForm();
        }

        void implSetXform(ISceneElLookup const&, Transform const& t) final
        {
            setXform(t);
        }

        AABB implCalcBounds(ISceneElLookup const&) const final
        {
            return calcBounds();
        }

        UID m_ID;
        UID m_Attachment;  // can be ModelGraphIDs::Ground()
        Transform m_Transform;
        Mesh m_MeshData;
        std::filesystem::path m_Path;
        std::string m_Name;
    };
}
