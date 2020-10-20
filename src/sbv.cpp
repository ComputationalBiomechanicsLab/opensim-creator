#include "application.hpp"

#include "screen.hpp"
#include "imgui_extensions.hpp"
#include "gl.hpp"

#include "Simbody.h"
#include <SDL.h>
#include "imgui.h"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

#include <thread>
#include <memory>


// sbv: simbody visualizer: an API-compatible visualizer for simbody
//
// currently experimental. Just seeing what's necessary to enforce the same API
// without having to resort to IPC. Ideally, I want a low-overhead UI that can
// be dropped into any existing Simbody code

using namespace SimTK;

// I wrote these because I cba'd inventing my way around something that comes
// with C++20. Just wait until C++20 comes and nuke this code.
namespace {
    // replace me with C++20's std::stop_token
    class Adams_stop_token final {
        std::shared_ptr<std::atomic<bool>> shared_state;
    public:
        Adams_stop_token(std::shared_ptr<std::atomic<bool>> st)
            : shared_state{std::move(st)} {
        }
        // these are deleted to ensure the API is a strict subset of C++20
        Adams_stop_token(Adams_stop_token const&) = delete;
        Adams_stop_token(Adams_stop_token&& tmp) :
            shared_state{tmp.shared_state}  {
        }
        Adams_stop_token& operator=(Adams_stop_token const&) = delete;
        Adams_stop_token& operator=(Adams_stop_token&&) = delete;
        ~Adams_stop_token() noexcept = default;

        bool stop_requested() const noexcept {
            return *shared_state;
        }
    };

    // replace me with C++20's std::stop_source
    class Adams_stop_source final {
        std::shared_ptr<std::atomic<bool>> shared_state;
    public:
        Adams_stop_source() :
            shared_state{new std::atomic<bool>{false}} {
        }
        // these are deleted to ensure the API is a strict subset of C++20
        Adams_stop_source(Adams_stop_source const&) = delete;
        Adams_stop_source(Adams_stop_source&&) = delete;
        Adams_stop_source& operator=(Adams_stop_source const&) = delete;
        Adams_stop_source& operator=(Adams_stop_source&&) = delete;
        ~Adams_stop_source() = default;

        bool request_stop() noexcept {
            // as-per the spec, but always true for this impl.
            bool has_stop_state = shared_state != nullptr;
            bool already_stopped = shared_state->exchange(true);

            return has_stop_state and (not already_stopped);
        }

        Adams_stop_token get_token() const noexcept {
            return Adams_stop_token{shared_state};
        }
    };

    // replace me with C++20's std::jthread
    class Adams_jthread final {
        Adams_stop_source s;
        std::thread t;
    public:
        template<class Function, class... Args>
        Adams_jthread(Function&& f, Args&&... args) :
            s{},
            t{f, s.get_token(), std::forward<Args>(args)...} {
        }
        Adams_jthread(Adams_jthread const&) = delete;
        Adams_jthread(Adams_jthread&&) = delete;
        Adams_jthread& operator=(Adams_jthread const&) = delete;
        Adams_jthread& operator=(Adams_jthread&&) = delete;
        ~Adams_jthread() noexcept {
            s.request_stop();
            t.join();
        }

        bool request_stop() noexcept {
            return s.request_stop();
        }
    };
}

namespace sbv {
    // message sent by the simulation to the visualizer
    struct Frame final {
    };

    // communication channel between the main (simulation) thread and the
    // background (UI) thread
    //
    // the channel is one-to-one and non-blocking, with a bounded capacity of
    // one, where new messages overwrite not-yet-recieved ones (i.e. frames
    // are "dropped" if the simulation races ahead of the visualizer)
    struct Visualizer_channel final {
        std::mutex lock;
        std::unique_ptr<Frame> latest;
    };

    // "sender" side of the visualizer channel (i.e. the simulation)
    class Visualizer_tx final {
        std::shared_ptr<Visualizer_channel> channel;
    public:
        Visualizer_tx(std::shared_ptr<Visualizer_channel> _channel) :
            channel{std::move(_channel)} {
        }
        Visualizer_tx(Visualizer_tx const&) = delete;
        Visualizer_tx(Visualizer_tx&&) = default;
        Visualizer_tx& operator=(Visualizer_tx const&) = delete;
        Visualizer_tx& operator=(Visualizer_tx&&) = delete;
        ~Visualizer_tx() noexcept = default;

        void send(std::unique_ptr<Frame> frame) {
            if (channel.use_count() == 1) {
                throw std::runtime_error{"Visualizer_tx::send error: tried to send a frame when nothing was available to receive it: this can happen if the visualizer is closed while the simulation is still running"};
            }
            std::lock_guard l{channel->lock};
            std::swap(channel->latest, frame);
        }
    };

    // "receiver" side of the visualizer channel (i.e. the UI)
    class Visualizer_rx final {
        std::shared_ptr<Visualizer_channel> channel;
    public:
        Visualizer_rx(std::shared_ptr<Visualizer_channel> _channel) :
            channel{std::move(_channel)} {
        }
        Visualizer_rx(Visualizer_rx const&) = delete;
        Visualizer_rx(Visualizer_rx&&) = default;
        Visualizer_rx& operator=(Visualizer_rx const&) = delete;
        Visualizer_rx& operator=(Visualizer_rx&&) = delete;
        ~Visualizer_rx() noexcept = default;

        // the unique_ptr will contain `nullptr` if no frame is available
        std::unique_ptr<Frame> poll() {
            if (channel.use_count() == 1) {
                throw std::runtime_error{"Visualizer_rx::poll error: tried to poll a frame when nothing is available to send one (ever): this can happen if the simulation code forgot to close the visualizer before finishing"};
            }

            std::lock_guard l{channel->lock};
            return std::move(channel->latest);
        }
    };

    std::pair<Visualizer_tx, Visualizer_rx> make_channel() {
        auto chan = std::make_shared<Visualizer_channel>();
        return {Visualizer_tx{chan}, Visualizer_rx{chan}};
    }

    class Visualizer_screen final : public osmv::Screen {
        Adams_stop_token stopper;
        Visualizer_rx rx;
        std::unique_ptr<Frame> frame;
    public:
        Visualizer_screen(Adams_stop_token _stopper,
                          Visualizer_rx _rx,
                          std::unique_ptr<Frame> _frame) :
            stopper{std::move(_stopper)},
            rx{std::move(_rx)},
            frame{std::move(_frame)} {
        }

        osmv::Screen_response tick(osmv::Application&) override {
            if (stopper.stop_requested()) {
                return osmv::Resp_Please_quit{};
            }
            return osmv::Resp_Ok{};
        }

        void draw(osmv::Application& s) override {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(s.window);

            ImGui::NewFrame();

            {
                bool b = true;
                ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar);
            }

            ImGui::Text("showing");

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    };

    class Waiting_screen final : public osmv::Screen {
        Adams_stop_token stopper;
        Visualizer_rx rx;
    public:
        Waiting_screen(Adams_stop_token _stopper, Visualizer_rx _rx) :
            stopper{std::move(_stopper)}, rx{std::move(_rx)} {
        }

        osmv::Screen_response tick(osmv::Application&) override {
            if (stopper.stop_requested()) {
                return osmv::Resp_Please_quit{};
            }
            auto maybe_frame = rx.poll();
            if (maybe_frame) {
                return osmv::Resp_Transition_to{std::make_unique<Visualizer_screen>(
                    std::move(stopper),
                    std::move(rx),
                    std::move(maybe_frame)
                )};
            }
            return osmv::Resp_Ok{};
        }

        void draw(osmv::Application& s) override {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(s.window);

            ImGui::NewFrame();

            {
                bool b = true;
                ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar);
            }

            ImGui::Text("waiting for simbody to send the first frame");

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    };

    // main thread for visualizer
    void visualizer_main(Adams_stop_token stopper, Visualizer_rx frames) {
        auto a = osmv::Application{};
        a.current_screen = std::make_unique<Waiting_screen>(std::move(stopper), std::move(frames));
        a.show();
    }

    // internal implementation of the visualizer
    class Visualizer_impl final {
        Adams_jthread thread;
        Visualizer_tx sender;
    public:
        Visualizer_impl(std::pair<Visualizer_tx, Visualizer_rx> channel) :
            thread{visualizer_main, std::move(channel.second)},
            sender{std::move(channel.first)} {
        }

        void send(std::unique_ptr<Frame> frame) {
            sender.send(std::move(frame));
        }
    };


    // SimTK-compatible Visualizer
    class Visualizer final {
        Visualizer_impl impl{make_channel()};
        SimTK::MultibodySystem& mobod;
    public:
        class Reporter;

        Visualizer(MultibodySystem& _mobod) :
            mobod{_mobod} {
        }

        void drawFrameNow(State const&) {
            impl.send(std::make_unique<Frame>());
            // TODO:
            // - create frame in this thread
            // - send it down the impl's channel to the UI
        }
    };


    // SimTK-compatible visualizer reporter
    class Visualizer::Reporter final : public PeriodicEventReporter {
        Visualizer& viz;
    public:
        Reporter(Visualizer& _viz, Real _reportInterval) :
            PeriodicEventReporter{_reportInterval},
            viz{_viz} {
        }

        void handleEvent(State const& s) const override {
            viz.drawFrameNow(s);
        }
    };
}

int main(int, char**) {
    auto system = MultibodySystem{};
    auto matter = SimbodyMatterSubsystem{system};
    auto forces = GeneralForceSubsystem{system};
    auto contact = GeneralContactSubsystem{system};
    auto cables = CableTrackerSubsystem{system};

    auto gravity = Force::UniformGravity{
        forces,
        matter,
        Vec3{0.0, -9.80665, 0.0},
    };

    double body_mass = 30.0;
    double body_side_len = 0.1;
    auto center_of_mass = Vec3{0.0, 0.0, 0.0};
    auto body_inertia = body_mass * Inertia::brick(Vec3{body_side_len / 2.0});
    auto slider_orientation = Rotation{Pi/2.0, ZAxis};
    auto body_offset = Vec3{0.4, 0.0, 0.0};

    // left mass
    auto body_left = Body::Rigid{
        MassProperties{body_mass, center_of_mass, body_inertia},
    };
    // decorate masses as cubes
    body_left.addDecoration(
        SimTK::Transform{},
        SimTK::DecorativeBrick{Vec3{body_side_len / 2.0}}.setColor({0.8, 0.1, 0.1})
    );
    auto slider_left = MobilizedBody::Slider{
        matter.Ground(),
        Transform{slider_orientation, -body_offset},
        body_left,
        Transform{slider_orientation, Vec3{0.0, 0.0, 0.0}},
    };
    slider_left.setDefaultQ(0.5);  // simbody equivalent to the coordinate bs
    auto spring_to_left = SimTK::Force::TwoPointLinearSpring{
            forces,
            matter.Ground(),
            Vec3(0),
            slider_left,
            Vec3(0, -body_side_len / 2, 0),
            100,
            0.5
    };



    // right mass
    auto body_right = Body::Rigid{
        MassProperties{body_mass, center_of_mass, body_inertia},
    };
    body_right.addDecoration(
        SimTK::Transform{},
        SimTK::DecorativeBrick{Vec3{body_side_len / 2.0}}.setColor({0.8, 0.1, 0.1})
    );
    auto slider_right = MobilizedBody::Slider{
        matter.Ground(),
        Transform{slider_orientation, body_offset},
        body_right,
        Transform{slider_orientation, Vec3{0.0, 0.0, 0.0}},
    };
    slider_right.setDefaultQ(0.5);  // simbody equivalent to the coordinate bs
    auto spring_to_right = SimTK::Force::TwoPointLinearSpring{
            forces,
            matter.Ground(),
            Vec3(0),
            slider_right,
            Vec3(0, -body_side_len / 2, 0),
            100,
            0.5
    };


    // cable path
    auto cable = CablePath{
        cables,
        slider_left,
        Vec3{0},
        slider_right,
        Vec3{0},
    };
    SimTK::CableSpring cable2(forces, cable, 50., 1.0, 0.1);


    // cable obstacle: a sphere that is fixed to the ground at some offset


    // obstacle in cable path
    double obstacle_radius = 0.08;
    auto obstacle_surface = CableObstacle::Surface{
        cable,
        matter.Ground(),
        Transform{Rotation{}, Vec3{0.0, 1.0, 0.0}},
        ContactGeometry::Cylinder{obstacle_radius},
    };
    // obstacles *require* contact point hints so that the wrapping cable
    // knows how to start wrapping over it
    obstacle_surface.setContactPointHints(
        // lhs
        Vec3{-obstacle_radius, 0.001, 0.0},
        // rhs
        Vec3{+obstacle_radius, 0.001, 0.0}
    );


    // set up visualization to match OpenSim (but without OpenSim) see:
    //     OpenSim: ModelVisualizer.cpp + SimulationUtilities.cpp
    system.setUseUniformBackground(true);
    auto visualizer = sbv::Visualizer{system};
    SDL_Delay(5000);
    // visualizer.setShowFrameRate(true);
    system.addEventReporter(new sbv::Visualizer::Reporter{
                                visualizer,
                                0.01
                            });

    // auto silo = SimTK::Visualizer::InputSilo{};
    // visualizer.addInputListener(&silo);

    auto help =
        SimTK::DecorativeText("Press any key to start a new simulation; ESC to quit.");
    help.setIsScreenText(true);
    // visualizer.addDecoration(SimTK::MobilizedBodyIndex(0), SimTK::Vec3(0), help);
    // visualizer.setShowSimTime(true);
    // visualizer.setMode(Visualizer::RealTime);

    // set up system
    system.realizeTopology();
    State s = system.getDefaultState();
    visualizer.drawFrameNow(s);

    auto integrator = RungeKuttaMersonIntegrator{system};
    auto time_stepper = SimTK::TimeStepper{system, integrator};
    time_stepper.initialize(s);
    time_stepper.stepTo(10.0);

    return 0;
}
