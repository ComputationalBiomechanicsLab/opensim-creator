#pragma once

#include "src/3d/bvh.hpp"
#include "src/3d/model.hpp"
#include "src/3d/instanced_renderer.hpp"

#include <SimTKcommon.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
    class ModelDisplayHints;
}

namespace osc {

    // CPU-side mesh that has already been loaded, deduped, BVHed, AABBed, etc.
    struct CPU_mesh final {
        NewMesh data;
        AABB aabb;
        BVH triangle_bvh;  // prim id indexes into data.verts
    };

    // precomputed meshdata that can be cached between geometry-generation calls
    struct Cached_meshdata final {
        std::shared_ptr<CPU_mesh> cpu_meshdata;
        Refcounted_instance_meshdata instance_meshdata;
    };

    // output from decoration generation
    struct Scene_decorations final {

        // instanced drawlist
        //
        // .instances might be reordered arbitrarily. the `.data` field will be packed
        // with the original index (24-bit little-endian)
        Mesh_instance_drawlist drawlist;

        // full (not just instancing-specific) meshdata, same order as drawlist.meshes
        std::vector<std::shared_ptr<CPU_mesh>> meshes_data;

        // instance AABBs in worldspace
        //
        // ordered as instances were emitted, you can use the .data field in the instance
        // to map an instance into this
        std::vector<AABB> aabbs;

        // instance mesh indexes
        //
        // ordered as instances were emitted, handy for mapping AABB collisions onto mesh
        // data (e.g. for mesh hit testing)
        std::vector<unsigned short> meshidxs;

        // model matrices
        std::vector<glm::mat4x3> model_mtxs;

        // scene-level BVH of the instances
        BVH aabb_bvh;  // prim id indexes into aabbs/mesh_idxs

        // components
        //
        // ordered as instances were emitted, you can use the .data field in the instance
        // to map an instance into this
        std::vector<OpenSim::Component const*> components;

        // wipe everything in this struct, but retain memory
        void clear();
    };

    using Modelstate_decoration_generator_flags = int;
    enum Modelstate_decoration_generator_flags_ {
        Modelstate_decoration_generator_flags_None = 0<<0,
        Modelstate_decoration_generator_flags_GenerateDynamicDecorations = 1<<0,
        Modelstate_decoration_generator_flags_GenerateStaticDecorations = 1<<1,
        Modelstate_decoration_generator_flags_GenerateFloor = 1<<2,
        Modelstate_decoration_generator_flags_Default =
            Modelstate_decoration_generator_flags_GenerateDynamicDecorations | Modelstate_decoration_generator_flags_GenerateStaticDecorations | Modelstate_decoration_generator_flags_GenerateFloor,
    };

    class Scene_generator final {
        // commonly-used analytic geometry: skip lookup
        Cached_meshdata m_CachedSphere;
        Cached_meshdata m_CachedCylinder;
        Cached_meshdata m_CachedBrick;
        Cached_meshdata m_CachedCone;

        // mesh files are cached
        std::unordered_map<std::string, std::unique_ptr<Cached_meshdata>> m_CachedMeshes;

        // this is used to ensure multiple instances of the same meshfile end up with the
        // same instance meshidx - cleared per-call
        std::unordered_map<Cached_meshdata*, int> m_MeshPtr2Meshidx;

        // used to store intermediate data
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomListCache;

    public:
        Scene_generator(Instanced_renderer&);

        void generate(
            Instanced_renderer&,
            OpenSim::Component const&,
            SimTK::State const&,
            OpenSim::ModelDisplayHints const&,
            Scene_decorations&,
            Modelstate_decoration_generator_flags = Modelstate_decoration_generator_flags_Default);
    };
}
