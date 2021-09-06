#pragma once

#include "src/3d/BVH.hpp"
#include "src/3d/Model.hpp"
#include "src/3d/InstancedRenderer.hpp"

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
    struct CPUMesh final {
        Mesh data;
        AABB aabb;
        BVH triangleBVH;  // prim id indexes into data.verts
    };

    // output from decoration generation
    struct SceneDecorations final {
        std::vector<glm::mat4x3> modelMtxs;
        std::vector<glm::mat3> normalMtxs;
        std::vector<Rgba32> cols;
        std::vector<InstanceableMeshdata> gpuMeshes;
        std::vector<std::shared_ptr<CPUMesh>> cpuMeshes;
        std::vector<AABB> aabbs;
        std::vector<OpenSim::Component const*> components;
        BVH sceneBVH;

        // wipe everything in this struct, but retain memory
        void clear();
    };

    // flags that affect what decorations get emitted into the output
    using SceneGeneratorFlags = int;
    enum SceneGeneratorFlags_ {
        SceneGeneratorFlags_None = 0<<0,
        SceneGeneratorFlags_GenerateDynamicDecorations = 1<<0,
        SceneGeneratorFlags_GenerateStaticDecorations = 1<<1,
        SceneGeneratorFlags_GenerateFloor = 1<<2,
        SceneGeneratorFlags_Default =
            SceneGeneratorFlags_GenerateDynamicDecorations | SceneGeneratorFlags_GenerateStaticDecorations | SceneGeneratorFlags_GenerateFloor,
    };

    struct CachedMeshdata final {
        std::shared_ptr<CPUMesh> cpuMeshdata;
        InstanceableMeshdata instanceMeshdata;
    };

    // class that can populate Scene_decorations lists
    class SceneGenerator final {
        std::unordered_map<std::string, std::unique_ptr<CachedMeshdata>> m_CachedMeshes;
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomListCache;

    public:
        SceneGenerator();

        void generate(
            OpenSim::Component const&,
            SimTK::State const&,
            OpenSim::ModelDisplayHints const&,
            SceneGeneratorFlags,
            float fixupScaleFactor,
            SceneDecorations&);
    };
}
