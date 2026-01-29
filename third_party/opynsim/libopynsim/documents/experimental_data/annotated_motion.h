#pragma once

#include <OpenSim/Simulation/Model/ModelComponent.h>

#include <filesystem>
#include <memory>

namespace OpenSim { class Storage; }

namespace opyn
{
    // Holds an annotated motion track.
    //
    // Note: This is similar to OpenSim GUI (4.5)'s `AnnotatedMotion.java` class. The
    //       reason it's reproduced here is to provide like-for-like (ish) behavior
    //       between OSC's 'Preview Experimental Data' workflow and OpenSim GUI's.
    class AnnotatedMotion final : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(AnnotatedMotion, OpenSim::ModelComponent)
    public:

        // Constructs an `AnnotationMotion` that was loaded from the given filesystem
        // path, or throws an `std::exception` if any error occurs.
        explicit AnnotatedMotion(const std::filesystem::path& path);

        // Returns the number of data series in the motion.
        size_t getNumDataSeries() const;
    private:
        static std::shared_ptr<OpenSim::Storage> loadPathIntoStorage(const std::filesystem::path&);
        explicit AnnotatedMotion(std::shared_ptr<OpenSim::Storage>);

        std::shared_ptr<OpenSim::Storage> m_Storage;
    };
}
