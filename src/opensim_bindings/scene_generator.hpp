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

    // output from decoration generation
    struct Scene_decorations final {
        std::vector<glm::mat4x3> model_xforms;
        std::vector<glm::mat3> normal_xforms;
        std::vector<Rgba32> rgbas;
        std::vector<Instanceable_meshdata> gpu_meshes;
        std::vector<std::shared_ptr<CPU_mesh>> cpu_meshes;
        std::vector<AABB> aabbs;
        std::vector<OpenSim::Component const*> components;
        BVH aabb_bvh;

        // wipe everything in this struct, but retain memory
        void clear();
    };

    // flags that affect what decorations get emitted into the output
    using Modelstate_decoration_generator_flags = int;
    enum Modelstate_decoration_generator_flags_ {
        Modelstate_decoration_generator_flags_None = 0<<0,
        Modelstate_decoration_generator_flags_GenerateDynamicDecorations = 1<<0,
        Modelstate_decoration_generator_flags_GenerateStaticDecorations = 1<<1,
        Modelstate_decoration_generator_flags_GenerateFloor = 1<<2,
        Modelstate_decoration_generator_flags_Default =
            Modelstate_decoration_generator_flags_GenerateDynamicDecorations | Modelstate_decoration_generator_flags_GenerateStaticDecorations | Modelstate_decoration_generator_flags_GenerateFloor,
    };

    struct Cached_meshdata final {
        std::shared_ptr<CPU_mesh> cpu_meshdata;
        Instanceable_meshdata instance_meshdata;
    };

    // class that can populate Scene_decorations lists
    class Scene_generator final {
        std::unordered_map<std::string, std::unique_ptr<Cached_meshdata>> m_CachedMeshes;
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomListCache;

    public:
        Scene_generator();

        void generate(
            OpenSim::Component const&,
            SimTK::State const&,
            OpenSim::ModelDisplayHints const&,
            Modelstate_decoration_generator_flags,
            float fixup_scale_factor,
            Scene_decorations&);
    };
}
