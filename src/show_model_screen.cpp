#include "show_model_screen.hpp"

#include "osmv_config.hpp"

#include "application.hpp"
#include "screen.hpp"
#include "meshes.hpp"
#include "opensim_wrapper.hpp"
#include "loading_screen.hpp"
#include "globals.hpp"

// OpenGL
#include "gl.hpp"
#include "gl_extensions.hpp"

// glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui
#include "imgui.h"
#include "imgui_extensions.hpp"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

// sdl
#include "sdl.hpp"

// c++
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

static constexpr float pi_f = static_cast<float>(M_PI);
static const ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
static const ImVec4 dark_red{0.6f, 0.0f, 0.0f, 1.0f};
static const ImVec4 dark_green{0.0f, 0.6f, 0.0f, 1.0f};


// map lookup that assumes the entry is already in the map, asserting (in debug
// mode) on failure.
template<typename K, typename V>
static V& asserting_find(std::unordered_map<K, V>& m, K const& k) {
    auto it = m.find(k);
    assert(it != m.end());
    return it->second;
}

// shim: std::stop_token (C++20)
class Stop_token final {
    std::shared_ptr<std::atomic<bool>> shared_state;
public:
    Stop_token(std::shared_ptr<std::atomic<bool>> st)
        : shared_state{std::move(st)} {
    }
    Stop_token(Stop_token const&) = delete;
    Stop_token(Stop_token&& tmp) :
        shared_state{tmp.shared_state}  {
    }
    Stop_token& operator=(Stop_token const&) = delete;
    Stop_token& operator=(Stop_token&&) = delete;
    ~Stop_token() noexcept = default;

    bool stop_requested() const noexcept {
        return *shared_state;
    }
};

// shim: std::stop_source (C++20)
class Stop_source final {
    std::shared_ptr<std::atomic<bool>> shared_state;
public:
    Stop_source() :
        shared_state{new std::atomic<bool>{false}} {
    }
    Stop_source(Stop_source const&) = delete;
    Stop_source(Stop_source&& tmp) :
        shared_state{std::move(tmp.shared_state)} {
    }
    Stop_source& operator=(Stop_source const&) = delete;
    Stop_source& operator=(Stop_source&& tmp) {
        shared_state = std::move(tmp.shared_state);
        return *this;
    }
    ~Stop_source() = default;

    bool request_stop() noexcept {
        // as-per the C++20 spec, but always true for this impl.
        bool has_stop_state = shared_state != nullptr;
        bool already_stopped = shared_state->exchange(true);

        return has_stop_state and (not already_stopped);
    }

    Stop_token get_token() const noexcept {
        return Stop_token{shared_state};
    }
};

// sim: std::jthread (C++20)
class Jthread final {
    Stop_source s;
    std::thread t;
public:
    // Creates new thread object which does not represent a thread
    Jthread() :
        s{},
        t{} {
    }

    // Creates new thread object and associates it with a thread of execution.
    // The new thread of execution immediately starts executing
    template<class Function, class... Args>
    Jthread(Function&& f, Args&&... args) :
        s{},
        t{f, s.get_token(), std::forward<Args>(args)...} {
    }

    // threads are non-copyable
    Jthread(Jthread const&) = delete;
    Jthread& operator=(Jthread const&) = delete;

    // threads are moveable: the moved-from value is a non-joinable thread that
    // does not represent a thread
    Jthread(Jthread&& tmp) = default;
    Jthread& operator=(Jthread&&) = default;

    // jthreads (or "joining threads") cancel + join on destruction
    ~Jthread() noexcept {
        if (joinable()) {
            s.request_stop();
            t.join();
        }
    }

    std::thread::id get_id() const noexcept {
        return t.get_id();
    }

    bool joinable() const noexcept {
        return t.joinable();
    }

    bool request_stop() noexcept {
        return s.request_stop();
    }

    void join() {
        return t.join();
    }
};

// thrown mid-simulation to cause the simulator thread to early-exit in a
// controlled manner (assuming OpenSim is exception-safe...)
class Stopped_exception final {};

// status of an OpenSim simulation
enum class Sim_status {
    Running = 1,
    Completed = 2,
    Cancelled = 4,
    Error = 8,
};

static char const* str(Sim_status s) noexcept {
    switch (s) {
    case Sim_status::Running:
        return "running";
    case Sim_status::Completed:
        return "completed";
    case Sim_status::Cancelled:
        return "cancelled";
    case Sim_status::Error:
        return "error";
    default:
        return "UNKNOWN STATUS: DEV ERROR";
    }
}

// Bounded single producer, single consumer channel with basic data recycling
template<typename Msg, size_t Capacity = 1>
class Bounded_spsc_channel final {
    enum class Holder_st { writeable, writing, readable };

    std::mutex m;
    std::array<Msg, Capacity> msgs;
    std::array<Holder_st, Capacity> states = []() {
        std::array<Holder_st, Capacity> rv;
        rv.fill(Holder_st::writeable);
        return rv;
    }();
    size_t reader_pos = 0;
    size_t writer_pos = 0;

public:

    // returned by writing methods
    class Message_writer final {
        Bounded_spsc_channel<Msg>& chan;
        size_t slot;

    public:
        Message_writer(Bounded_spsc_channel<Msg>& _chan, size_t _slot) :
            chan{_chan}, slot{_slot} {
            assert(chan.states[slot] == Holder_st::writing);
        }
        Message_writer(Message_writer const&) = delete;
        Message_writer(Message_writer&&) = delete;
        Message_writer& operator=(Message_writer const&) = delete;
        Message_writer& operator=(Message_writer&&) = delete;
        ~Message_writer() noexcept {
            std::lock_guard g{chan.m};

            assert(chan.states[slot] == Holder_st::writing);
            assert(chan.writer_pos == slot);

            chan.states[slot] = Holder_st::readable;
            chan.writer_pos = (chan.writer_pos + 1) % Capacity;
        }

        Msg& get() noexcept {
            return chan.msgs[slot];
        }

        Msg const& get() const noexcept {
            return chan.msgs[slot];
        }
    };

    // non-blocking: will only create a writer only if a writeable slot is
    // present.
    std::optional<Message_writer> try_write() {
        std::lock_guard g{m};

        if (states[writer_pos] == Holder_st::writeable) {
            states[writer_pos] = Holder_st::writing;
            return std::optional<Message_writer>(std::in_place, *this, writer_pos);
        }

        return std::nullopt;
    }

    bool try_pop(Msg& dest) {
        std::lock_guard g{m};

        if (states[reader_pos] == Holder_st::readable) {
            std::swap(dest, msgs[reader_pos]);
            states[reader_pos] = Holder_st::writeable;
            return true;
        }
        return false;
    }
};

// message from a forward-dynamic simulator thread
struct FD_simulator_msg final {
    osim::OSMV_State state;
    double sim_cur_time = 0.0;
    int num_prescribeq_calls = 0;
    float ui_overhead = 0.0;
};

// returns true if ran to completion, false if cancelled
static bool run_fd_simulation(
    Stop_token t,
    osim::OSMV_Model m,
    osim::OSMV_State s,
    double final_time,
    Bounded_spsc_channel<FD_simulator_msg>& chan) {

    using clock = std::chrono::steady_clock;

    clock::time_point last_report_end = clock::now();
    float ui_overhead = 0.0f;
    int ui_overhead_n = 0;
    float avg_overhead = 0.0f;

    // what happens during an intermittent update
    auto on_report = [&](osim::Simulation_update_event const& e) {
        if (t.stop_requested()) {
            throw Stopped_exception{};
        }

        clock::time_point report_start = clock::now();

        {
            auto maybe_writer = chan.try_write();
            if (maybe_writer) {
                FD_simulator_msg& msg = maybe_writer->get();
                msg.state = osim::copy_state(e.state);
                msg.sim_cur_time = e.simulation_time;
                msg.num_prescribeq_calls = e.num_prescribe_q_calls;
                msg.ui_overhead = avg_overhead;
                // message is auto-committed when writer destructs
            }
        }

        clock::time_point report_end = clock::now();

        if (ui_overhead_n > 0) {
            clock::duration sim_time = report_start - last_report_end;
            clock::duration ui_time = report_end - report_start;
            clock::duration total = sim_time + ui_time;
            float this_overhead = static_cast<float>(ui_time.count()) / static_cast<float>(total.count());
            float prev_overheads = ui_overhead;
            avg_overhead = (this_overhead + (ui_overhead_n-1)*prev_overheads) / ui_overhead_n;
        }

        last_report_end = report_end;
        ++ui_overhead_n;
    };

    // run the simulation
    try {
        osim::fd_simulation(m, s, final_time, on_report);
        return true;
    } catch (Stopped_exception const&) {
        return false;
    }
}

// represents a simulation running on a background thread
struct FDSim_new final {
    using clock = std::chrono::steady_clock;

    static constexpr double sim_start_time = 0.0;
    double _sim_final_time;

    // we hold a "latest" IPC message (defaulted) as a "swap" space for the IPC
    FD_simulator_msg latest;
    Bounded_spsc_channel<FD_simulator_msg> ipc;
    std::atomic<clock::duration> wall_start = clock::now().time_since_epoch();
    std::atomic<clock::duration> wall_end;
    std::atomic<Sim_status> status = Sim_status::Running;
    Jthread worker;
    int states_popped = 0;

    FDSim_new(OpenSim::Model const& model,
              SimTK::State const& initial_state,
              double _final_time) :
        _sim_final_time{_final_time} {

        osim::OSMV_Model copy = osim::copy_model(model);
        osim::finalize_properties_from_state(copy, initial_state);
        osim::init_system(copy);

        auto background_task = [this](Stop_token t, osim::OSMV_Model m, osim::OSMV_State s) {
            wall_start = clock::now().time_since_epoch();
            try {
                bool cancelled = run_fd_simulation(std::move(t), std::move(m), std::move(s), _sim_final_time, ipc);
                status = cancelled ? Sim_status::Cancelled : Sim_status::Completed;
            } catch (...) {
                status = Sim_status::Error;
                // TODO: not do this shit.
            }
            wall_end = clock::now().time_since_epoch();
        };

        worker = Jthread{std::move(background_task), std::move(copy), osim::copy_state(initial_state)};
    }

    bool try_pop_latest(osim::OSMV_State& dest) {
        std::swap(dest, latest.state);
        bool popped = ipc.try_pop(latest);
        std::swap(dest, latest.state);

        assert(dest.handle != nullptr);

        if (popped) {
            ++states_popped;
        }

        return popped;
    }

    bool request_stop() noexcept {
        return worker.request_stop();
    }

    bool running() const noexcept {
        return status == Sim_status::Running;
    }

    clock::duration wall_duration() const noexcept {
        clock::duration endpoint =
            running() ? clock::now().time_since_epoch() : wall_end.load();

        return endpoint - wall_start.load();
    }

    double sim_final_time() const noexcept {
        return _sim_final_time;
    }

    double current_sim_time() const noexcept {
        return latest.sim_cur_time;
    }

    char const* status_str() const noexcept {
        return str(status);
    }

    int prescribeq_calls() const noexcept {
        return latest.num_prescribeq_calls;
    }

    float ui_overhead() const noexcept {
        return latest.ui_overhead;
    }

    int states_sent_to_ui() const noexcept {
        return states_popped;
    }
};

// represents a running/ran OpenSim forward dynamics simulation
class FD_Simulation final {
    using clock = std::chrono::steady_clock;

    // mutex for latest state
    std::mutex mut;
    std::optional<osim::OSMV_State> latest_state;

    std::atomic<Sim_status> _status = Sim_status::Running;

    // wall clocks are "time since epoch" of the clock, because C++ does not
    // permit storing a clock::time_point in an `atomic`
    std::atomic<clock::duration> _wall_start_time;
    std::atomic<clock::duration> wall_end_time;

    static constexpr double sim_start_time = 0.0;
    std::atomic<double> sim_cur_time = 0.0;
    double _sim_final_time;
    std::atomic<int> num_prescribeq_calls = 0;
    std::atomic<float> _ui_overhead = 0.0;
    std::atomic<int> _states_sent_to_ui =  0;

    Jthread simulator_thread;

public:

    // immediately starts a simulation for `model`
    FD_Simulation(OpenSim::Model const& model,
                  SimTK::State const& initial_state,
                  double final_time) :

        _sim_final_time{final_time} {

        osim::OSMV_Model copy = osim::copy_model(model);
        osim::finalize_properties_from_state(copy, initial_state);
        osim::init_system(copy);

        // there may be a delay between this timepoint and when the thread is
        // scheduled by the OS, but I want to ensure, as a class invariant,
        // that class users can ask how long the sim has been running for

        _wall_start_time = clock::now().time_since_epoch();

        // the simulator thread's top-level function
        auto f = [this](
            Stop_token t,
            osim::OSMV_Model m,
            osim::OSMV_State s,
            double final_time) {

            _wall_start_time = clock::now().time_since_epoch();
            try {
                int ui_overhead_n = 0;
                clock::time_point last_end;

                osim::fd_simulation(
                    m,
                    s,
                    final_time,
                    [this, &t, &ui_overhead_n, &last_end](osim::Simulation_update_event const& e) {
                        clock::time_point start = clock::now();

                        this->sim_cur_time = e.simulation_time;
                        this->num_prescribeq_calls = e.num_prescribe_q_calls;

                        if (t.stop_requested()) {
                            throw Stopped_exception{};
                        }

                        if (not this->latest_state.has_value()) {
                            std::lock_guard g{this->mut};
                            this->latest_state = osim::copy_state(e.state);
                            ++this->_states_sent_to_ui;
                        }

                        clock::time_point end = clock::now();

                        if (ui_overhead_n) {
                            clock::duration sim_time = start - last_end;
                            clock::duration ui_time = end - start;
                            clock::duration total = sim_time + ui_time;
                            float this_overhead = static_cast<float>(ui_time.count()) / static_cast<float>(total.count());
                            float prev_overheads = this->_ui_overhead;
                            float avg_overhead = (this_overhead + (ui_overhead_n-1)*prev_overheads) / ui_overhead_n;

                            this->_ui_overhead = avg_overhead;
                        }

                        last_end = end;
                        ++ui_overhead_n;
                });
                this->_status = Sim_status::Completed;
            } catch (Stopped_exception const&) {
                this->_status = Sim_status::Cancelled;
            } catch (...) {
                this->_status = Sim_status::Error;
                throw;
            }
            wall_end_time = clock::now().time_since_epoch();
        };

        // start the simulator thread
        simulator_thread = Jthread{
            std::move(f),
            std::move(copy),
            osim::copy_state(initial_state), final_time
        };
    }

    std::optional<osim::OSMV_State> try_take_latest_update() {
        if (not latest_state.has_value()) {
            return std::nullopt;
        }

        std::lock_guard g{mut};
        return std::exchange(latest_state, std::nullopt);
    }

    bool request_stop() noexcept {
        return simulator_thread.request_stop();
    }

    bool running() const noexcept {
        return _status == Sim_status::Running;
    }

    clock::duration wall_duration() const noexcept {
        clock::duration endpoint =
            running() ? clock::now().time_since_epoch() : wall_end_time.load();

        return endpoint - _wall_start_time.load();
    }

    double sim_final_time() const noexcept {
        return _sim_final_time;
    }

    double current_sim_time() const noexcept {
        return sim_cur_time;
    }

    char const* status_str() const noexcept {
        return str(_status);
    }

    int prescribeq_calls() const noexcept {
        return num_prescribeq_calls;
    }

    float ui_overhead() const noexcept {
        return _ui_overhead;
    }

    int states_sent_to_ui() const noexcept {
        return _states_sent_to_ui;
    }
};

// represents a vbo containing some verts /w normals
struct Vbo_Triangles_with_norms final {
    GLsizei num_verts = 0;
    gl::Array_buffer vbo = gl::GenArrayBuffer();

    Vbo_Triangles_with_norms(std::vector<osim::Untextured_triangle> const& triangles)
        : num_verts(static_cast<GLsizei>(3 * triangles.size())) {

        static_assert(sizeof(osim::Untextured_triangle) == 3 * sizeof(osim::Untextured_vert));

        gl::BindBuffer(vbo.type, vbo);
        gl::BufferData(vbo.type, sizeof(osim::Untextured_triangle) * triangles.size(), triangles.data(), GL_STATIC_DRAW);
    }
};

static gl::Texture_2d generate_chequered_floor_texture() {
    struct Rgb { unsigned char r, g, b; };
    constexpr size_t w = 512;
    constexpr size_t h = 512;
    constexpr Rgb on_color = {0xfd, 0xfd, 0xfd};
    constexpr Rgb off_color = {0xeb, 0xeb, 0xeb};

    std::array<Rgb, w*h> pixels;
    for (size_t row = 0; row < h; ++row) {
        size_t row_start = row * w;
        bool y_on = (row/32) % 2 == 0;
        for (size_t col = 0; col < w; ++col) {
            bool x_on = (col/32) % 2 == 0;
            pixels[row_start + col] = y_on xor x_on ? on_color : off_color;
        }
    }

    gl::Texture_2d rv = gl::GenTexture2d();
    gl::BindTexture(rv.type, rv);
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    return rv;
}

// OpenGL shader for the floor
struct Floor_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "floor.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "floor.frag"));
    gl::Texture_2d tex = generate_chequered_floor_texture();
    gl::Uniform_mat4 projMat = gl::GetUniformLocation(p, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(p, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(p, "modelMat");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

    // instance stuff
    gl::Array_buffer quad_buf = []() {
        static const float vals[] = {
            // location         // tex coords
             1.0f,  1.0f, 0.0f,   100.0f, 100.0f,
             1.0f, -1.0f, 0.0f,   100.0f,  0.0f,
            -1.0f, -1.0f, 0.0f,    0.0f,  0.0f,

            -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,    0.0f, 100.0f,
             1.0f,  1.0f, 0.0f,   100.0f, 100.0f,
        };

        auto buf = gl::GenArrayBuffer();
        gl::BindBuffer(buf.type, buf);
        gl::BufferData(buf.type, sizeof(vals), vals, GL_STATIC_DRAW);
        return buf;
    }();

    gl::Vertex_array vao = [&]() {
        auto vao = gl::GenVertexArrays();
        gl::BindVertexArray(vao);
        gl::BindBuffer(quad_buf.type, quad_buf);
        gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
        gl::EnableVertexAttribArray(aTexCoord);
        gl::BindVertexArray();
        return vao;
    }();

    glm::mat4 model_mtx = glm::scale(glm::rotate(glm::identity<glm::mat4>(), pi_f/2, {1.0, 0.0, 0.0}), {100.0f, 100.0f, 0.0f});
};

// OpenGL shader for debugging normals
//
// uses geometry shader to draw each scene normal as a red line
struct Show_normals_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "normals.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "normals.frag"),
        gl::CompileGeometryShaderFile(OSMV_SHADERS_DIR "normals.geom"));

    gl::Uniform_mat4 projMat = gl::GetUniformLocation(program, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(program, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(program, "modelMat");
    gl::Uniform_mat4 normalMat = gl::GetUniformLocation(program, "normalMat");
    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
};

// OpenGL shader for rendering colored (not textured) geometry with Gouraud shading
struct Colored_gouraud_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(OSMV_SHADERS_DIR "main.vert"),
        gl::CompileFragmentShaderFile(OSMV_SHADERS_DIR "main.frag"));

    gl::Uniform_mat4 projMat = gl::GetUniformLocation(program, "projMat");
    gl::Uniform_mat4 viewMat = gl::GetUniformLocation(program, "viewMat");
    gl::Uniform_mat4 modelMat = gl::GetUniformLocation(program, "modelMat");
    gl::Uniform_mat4 normalMat = gl::GetUniformLocation(program, "normalMat");
    gl::Uniform_vec4 rgba = gl::GetUniformLocation(program, "rgba");
    gl::Uniform_vec3 light_pos = gl::GetUniformLocation(program, "lightPos");
    gl::Uniform_vec3 light_color = gl::GetUniformLocation(program, "lightColor");
    gl::Uniform_vec3 view_pos = gl::GetUniformLocation(program, "viewPos");

    gl::Attribute location = gl::GetAttribLocation(program, "location");
    gl::Attribute in_normal = gl::GetAttribLocation(program, "in_normal");
};

// A (potentially shared) mesh that can be rendered by `Main_program`
struct Main_program_renderable final {
    std::shared_ptr<Vbo_Triangles_with_norms> verts;
    gl::Vertex_array vao;

    Main_program_renderable(Colored_gouraud_shader& p,
                            std::shared_ptr<Vbo_Triangles_with_norms> _verts) :
        verts{std::move(_verts)},
        vao{[&]() {
            auto vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            {
                gl::BindBuffer(verts->vbo.type, verts->vbo);
                gl::VertexAttribPointer(p.location, 3, GL_FLOAT, GL_FALSE, sizeof(osim::Untextured_vert), reinterpret_cast<void*>(offsetof(osim::Untextured_vert, pos)));
                gl::EnableVertexAttribArray(p.location);
                gl::VertexAttribPointer(p.in_normal, 3, GL_FLOAT, GL_FALSE, sizeof(osim::Untextured_vert), reinterpret_cast<void*>(offsetof(osim::Untextured_vert, normal)));
                gl::EnableVertexAttribArray(p.in_normal);
                gl::BindVertexArray();
            }
            return vao;
        }()} {
    }
};

// a (potentially shared) mesh that is ready for `Normals_program` to render
struct Normals_program_renderable final {
    std::shared_ptr<Vbo_Triangles_with_norms> verts;
    gl::Vertex_array vao;

    Normals_program_renderable(Show_normals_shader& p,
                               std::shared_ptr<Vbo_Triangles_with_norms> _verts) :
        verts{std::move(_verts)},
        vao{[&]() {
            auto vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(verts->vbo.type, verts->vbo);
            gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(osim::Untextured_vert), reinterpret_cast<void*>(offsetof(osim::Untextured_vert, pos)));
            gl::EnableVertexAttribArray(p.aPos);
            gl::VertexAttribPointer(p.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(osim::Untextured_vert), reinterpret_cast<void*>(offsetof(osim::Untextured_vert, normal)));
            gl::EnableVertexAttribArray(p.aNormal);
            gl::BindVertexArray();
            return vao;
        }()} {
    }
};

// mesh received from OpenSim model and then uploaded into OpenGL
struct Osim_mesh final {
    Main_program_renderable main_prog_renderable;
    Normals_program_renderable normals_prog_renderable;

    Osim_mesh(Colored_gouraud_shader& mp, Show_normals_shader& np, osim::Untextured_mesh const& m) :
        main_prog_renderable{mp, std::make_shared<Vbo_Triangles_with_norms>(m.triangles)},
        normals_prog_renderable{np, main_prog_renderable.verts} {
    }
};

namespace osmv {
    struct Show_model_screen_impl final {
        std::string path;

        Colored_gouraud_shader pMain = {};
        Show_normals_shader pNormals = {};

        osim::Untextured_mesh mesh_swap_space;

        Main_program_renderable pMain_cylinder = [&]() {
            // render triangles into swap space
            osmv::simbody_cylinder_triangles(12, mesh_swap_space.triangles);

            return Main_program_renderable{
                pMain,
                std::make_shared<Vbo_Triangles_with_norms>(mesh_swap_space.triangles)
            };
        }();

        Normals_program_renderable pNormals_cylinder =
            {pNormals, pMain_cylinder.verts};

        Main_program_renderable pMain_sphere = [&]() {
            // render triangles into swap space
            osmv::unit_sphere_triangles(mesh_swap_space.triangles);

            return Main_program_renderable{
                pMain,
                std::make_shared<Vbo_Triangles_with_norms>(mesh_swap_space.triangles)
            };
        }();

        Main_program_renderable pNormals_sphere = {pMain, pMain_sphere.verts};

        Floor_shader pFloor;


        float radius = 5.0f;
        float wheel_sensitivity = 0.9f;

        float fov = 120.0f;

        bool dragging = false;
        float theta = 0.88f;
        float phi =  0.4f;
        float sensitivity = 1.0f;
        float fd_final_time = 0.5f;

        bool panning = false;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};

        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        float light_color[3] = {0.9607f, 0.9176f, 0.8863f};
        bool wireframe_mode = false;
        bool show_light = false;
        bool show_unit_cylinder = false;

        bool gamma_correction = false;
        bool show_mesh_normals = false;
        bool show_floor = true;

        // coordinates tab state
        char coords_tab_filter[64]{};
        std::vector<osim::Coordinate> model_coords_swap;
        bool sort_coords_by_name = true;
        bool show_rotational_coords = true;
        bool show_translational_coords = true;
        bool show_coupled_coords = true;

        // model tab stuff
        std::vector<osim::Muscle_stat> model_muscs_swap;
        char model_tab_filter[64]{};

        // MAs tab state
        std::string const* mas_muscle_selection = nullptr;
        std::string const* mas_coord_selection = nullptr;
        static constexpr unsigned num_steps = 50;
        struct MA_plot final {
            std::string muscle_name;
            std::string coord_name;
            float x_begin;
            float x_end;
            std::array<float, num_steps> y_vals;
            float min;
            float max;
        };
        std::vector<std::unique_ptr<MA_plot>> mas_plots;

        sdl::Window_dimensions window_dims;

        std::optional<FD_Simulation> simulator;

        osim::OSMV_Model model;
        osim::State_geometry geom;
        osim::Geometry_loader geom_loader;
        osim::OSMV_State latest_state;
        std::unordered_map<osim::Mesh_id, Osim_mesh> meshes;

        Show_model_screen_impl(std::string _path, osim::OSMV_Model model);

        void init(Application&);
        osmv::Screen_response handle_event(Application&, SDL_Event&);
        Screen_response tick(Application&);

        void on_user_edited_model() {
            if (simulator) {
                simulator->request_stop();
                simulator->try_take_latest_update();
            }

            SimTK::State& s = osim::init_system(model);
            latest_state = osim::copy_state(s);

            mas_muscle_selection = nullptr;
            mas_coord_selection = nullptr;

            update_scene();
        }

        void on_user_edited_state() {
            simulator and simulator->request_stop();
            update_scene();
        }

        void update_scene();

        [[nodiscard]] bool simulator_running() const noexcept {
            return simulator and simulator->running();
        }

        void draw(Application&);
        void draw_model_scene(Application&);
        void draw_imgui_ui(Application&);
        void draw_menu_bar();
        void draw_lhs_panel(Application&);
        void draw_simulate_tab();
        void draw_ui_tab(Application&);
        void draw_coords_tab();
        void draw_coordinate_slider(osim::Coordinate const&);
        void draw_utils_tab();
        void draw_muscles_tab();
        void draw_mas_tab();
    };
}

osmv::Show_model_screen::Show_model_screen(std::string path, osim::OSMV_Model model) :
    impl{new Show_model_screen_impl{std::move(path), std::move(model)}} {
}

osmv::Show_model_screen_impl::Show_model_screen_impl(std::string _path, osim::OSMV_Model _model) :
    path{std::move(_path)},
    model{std::move(_model)},
    latest_state{[this]() {
        osim::finalize_from_properties(model);
        SimTK::State& s = osim::init_system(model);
        return osim::copy_state(s);
    }()}
{
    update_scene();
}

void osmv::Show_model_screen_impl::update_scene() {
    osim::realize_velocity(model, latest_state);
    geom_loader.geometry_in(model, latest_state, geom);

    // populate mesh data, if necessary
    for (osim::Mesh_instance const& mi : geom.mesh_instances) {
        if (meshes.find(mi.mesh) == meshes.end()) {
            geom_loader.load_mesh(mi.mesh, mesh_swap_space);
            meshes.emplace(mi.mesh, Osim_mesh{pMain, pNormals, mesh_swap_space});
        }
    }
}

osmv::Show_model_screen::~Show_model_screen() noexcept = default;

void osmv::Show_model_screen::init(osmv::Application& s) {
    impl->init(s);
}

void osmv::Show_model_screen_impl::init(Application & s) {
    window_dims = sdl::GetWindowSize(s.window);
}

osmv::Screen_response osmv::Show_model_screen::handle_event(Application& s, SDL_Event& e) {
    return impl->handle_event(s, e);
}

osmv::Screen_response osmv::Show_model_screen_impl::handle_event(Application& ui, SDL_Event& e) {
    ImGuiIO& io = ImGui::GetIO();
    float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                return Resp_Please_quit{};
            case SDLK_w:
                wireframe_mode = not wireframe_mode;
                break;
            case SDLK_r: {
                auto km = SDL_GetModState();
                if (km & (KMOD_LCTRL | KMOD_RCTRL)) {
                    return Resp_Transition_to{std::make_unique<osmv::Loading_screen>(path)};
                } else {
                    latest_state = osim::copy_state(osim::init_system(model));
                    osim::realize_velocity(model, latest_state);
                    update_scene();
                }
                break;
            }
            case SDLK_SPACE: {
                if (simulator and simulator->running()) {
                    simulator->request_stop();
                } else {
                    simulator.emplace(model, latest_state, fd_final_time);
                }
                break;
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = true;
            break;
        case SDL_BUTTON_RIGHT:
            panning = true;
            break;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = false;
            break;
        case SDL_BUTTON_RIGHT:
            panning = false;
            break;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return Resp_Ok{};
        }

        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return Resp_Ok{};
        }

        if (dragging) {
            // alter camera position while dragging
            float dx = -static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);
            theta += 2.0f * static_cast<float>(M_PI) * sensitivity * dx;
            phi += 2.0f * static_cast<float>(M_PI) * sensitivity * dy;
        }

        if (panning) {
            float dx = static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = -static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);

            // how much panning is done depends on how far the camera is from the
            // origin (easy, with polar coordinates) *and* the FoV of the camera.
            float x_amt = dx * aspect_ratio * (2.0f * tan(fov / 2.0f) * radius);
            float y_amt = dy * (1.0f / aspect_ratio) * (2.0f * tan(fov / 2.0f) * radius);

            // this assumes the scene is not rotated, so we need to rotate these
            // axes to match the scene's rotation
            glm::vec4 default_panning_axis = { x_amt, y_amt, 0.0f, 1.0f };
            auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
            auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
            auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
            auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), phi, phi_axis);

            glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
            pan.x += panning_axes.x;
            pan.y += panning_axes.y;
            pan.z += panning_axes.z;
        }

        // wrap mouse if it hits edges
        if (dragging or panning) {
            constexpr int edge_width = 5;
            if (e.motion.x + edge_width > window_dims.w) {
                SDL_WarpMouseInWindow(ui.window, edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                SDL_WarpMouseInWindow(ui.window, window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                SDL_WarpMouseInWindow(ui.window, e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                SDL_WarpMouseInWindow(ui.window, e.motion.x, window_dims.h - edge_width);
            }
        }
    } else if (e.type == SDL_WINDOWEVENT) {
        window_dims = sdl::GetWindowSize(ui.window);
        glViewport(0, 0, window_dims.w, window_dims.h);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (io.WantCaptureMouse) {
            // if ImGUI wants to capture the mouse, then the mouse
            // is probably interacting with an ImGUI panel and,
            // therefore, the dragging/panning shouldn't be handled
            return Resp_Ok{};
        }

        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= wheel_sensitivity;
        }
    }

    return Resp_Ok{};
}

osmv::Screen_response osmv::Show_model_screen::tick(Application& a) {
    return impl->tick(a);
}

osmv::Screen_response osmv::Show_model_screen_impl::tick(Application &) {
    if (simulator) {
        std::optional<osim::OSMV_State> latest = simulator->try_take_latest_update();
        if (latest) {
            latest_state = std::move(*latest);
            osim::realize_report(model, latest_state);
            update_scene();
        }
    }

    return Resp_Ok{};
}

void osmv::Show_model_screen::draw(osmv::Application& ui) {
    impl->draw(ui);
}

void osmv::Show_model_screen_impl::draw(osmv::Application& ui) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

    if (gamma_correction) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    draw_model_scene(ui);
    draw_imgui_ui(ui);
}

// draw main 3D scene
void osmv::Show_model_screen_impl::draw_model_scene(Application& ui) {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    glm::mat4 view_mtx = [&]() {
        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
        auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
        auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
        auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
        auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
        return glm::lookAt(
            glm::vec3(0.0f, 0.0f, radius),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3{0.0f, 1.0f, 0.0f}) * rot_theta * rot_phi * pan_translate;
    }();

    glm::mat4 proj_mtx = [&]() {
        float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);
        return glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
    }();

    glm::vec3 view_pos = [&]() {
        // polar/spherical to cartesian
        return glm::vec3{
            radius * sin(theta) * cos(phi),
            radius * sin(phi),
            radius * cos(theta) * cos(phi)
        };
    }();

    // render elements that have a solid color
    {
        gl::UseProgram(pMain.program);

        gl::Uniform(pMain.projMat, proj_mtx);
        gl::Uniform(pMain.viewMat, view_mtx);
        gl::Uniform(pMain.light_pos, light_pos);
        gl::Uniform(pMain.light_color, glm::vec3(light_color[0], light_color[1], light_color[2]));
        gl::Uniform(pMain.view_pos, view_pos);

        // draw model meshes
        for (auto& m : geom.mesh_instances) {
            gl::Uniform(pMain.rgba, m.rgba);
            gl::Uniform(pMain.modelMat, m.transform);
            gl::Uniform(pMain.normalMat, m.normal_xform);

            Osim_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.main_prog_renderable.vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.main_prog_renderable.verts->num_verts);
            gl::BindVertexArray();
        }

        // debugging: draw unit cylinder
        if (show_unit_cylinder) {
            gl::BindVertexArray(pMain_cylinder.vao);

            gl::Uniform(pMain.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
            gl::Uniform(pMain.modelMat, glm::identity<glm::mat4>());
            gl::Uniform(pMain.normalMat, glm::identity<glm::mat4>());

            auto num_verts = pMain_cylinder.verts->num_verts;
            gl::DrawArrays(GL_TRIANGLES, 0, num_verts);

            gl::BindVertexArray();
        }

        // debugging: draw light location
        if (show_light) {
            gl::BindVertexArray(pMain_sphere.vao);

            gl::Uniform(pMain.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
            auto xform = glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05});
            gl::Uniform(pMain.modelMat, xform);
            gl::Uniform(pMain.normalMat, glm::transpose(glm::inverse(xform)));
            auto num_verts = pMain_sphere.verts->num_verts;
            gl::DrawArrays(GL_TRIANGLES, 0, num_verts);

            gl::BindVertexArray();
        }
    }

    // floor is rendered with a texturing program
    if (show_floor) {
        gl::UseProgram(pFloor.p);
        gl::Uniform(pFloor.projMat, proj_mtx);
        gl::Uniform(pFloor.viewMat, view_mtx);
        gl::Uniform(pFloor.modelMat, pFloor.model_mtx);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(pFloor.tex.type, pFloor.tex);
        gl::Uniform(pFloor.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(pFloor.vao);
        gl::DrawArrays(GL_TRIANGLES, 0, 6);
        gl::BindVertexArray();
    }

    // debugging: draw mesh normals
    if (show_mesh_normals) {
        gl::UseProgram(pNormals.program);
        gl::Uniform(pNormals.projMat, proj_mtx);
        gl::Uniform(pNormals.viewMat, view_mtx);

        for (auto& m : geom.mesh_instances) {
            gl::Uniform(pNormals.modelMat, m.transform);
            gl::Uniform(pNormals.normalMat, m.normal_xform);

            Osim_mesh& md = asserting_find(meshes, m.mesh);
            gl::BindVertexArray(md.normals_prog_renderable.vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.normals_prog_renderable.verts->num_verts);
            gl::BindVertexArray();
        }
    }
}

void osmv::Show_model_screen_impl::draw_imgui_ui(Application& ui) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(ui.window);
    ImGui::NewFrame();

    draw_menu_bar();
    draw_lhs_panel(ui);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void osmv::Show_model_screen_impl::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            if (ImGui::BeginTabItem(path.c_str())) {
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndMainMenuBar();
    }
}

void osmv::Show_model_screen_impl::draw_lhs_panel(Application& ui) {
    bool b = true;
    ImGuiWindowFlags flags = 0;
    ImGui::Begin("Model", &b, flags);

    if (ImGui::BeginTabBar("SomeTabBar")) {

        if (ImGui::BeginTabItem("Simulate")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_simulate_tab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_ui_tab(ui);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Muscles")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_muscles_tab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Coords")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_coords_tab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Utils")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_utils_tab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("MAs")) {
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            draw_mas_tab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void osmv::Show_model_screen_impl::draw_simulate_tab() {
    // start/stop button
    if (simulator_running()) {
        ImGui::PushStyleColor(ImGuiCol_Button, red);
        if (ImGui::Button("stop [SPC]")) {
            simulator->request_stop();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, dark_green);
        if (ImGui::Button("start [SPC]")) {
            simulator.emplace(model, latest_state, fd_final_time);
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (ImGui::Button("reset [r]")) {
        latest_state = osim::copy_state(osim::init_system(model));
        osim::realize_velocity(model, latest_state);
        update_scene();
    }

    ImGui::Dummy(ImVec2{0.0f, 20.0f});

    ImGui::Text("simulation config");
    ImGui::Separator();
    ImGui::SliderFloat("final time", &fd_final_time, 0.01f, 20.0f);

    if (simulator) {
        std::chrono::milliseconds wall_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(simulator->wall_duration());
        double wall_secs = static_cast<double>(wall_ms.count())/1000.0;
        double sim_secs = simulator->current_sim_time();
        double pct_completed = sim_secs/simulator->sim_final_time() * 100.0;

        ImGui::Dummy(ImVec2{0.0f, 20.0f});
        ImGui::Text("simulator stats");
        ImGui::Separator();
        ImGui::Text("status: %s", simulator->status_str());
        ImGui::Text("progress: %.2f %%", pct_completed);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("simulation time: %.2f", sim_secs);
        ImGui::Text("wall time: %.2f secs", wall_secs);
        ImGui::Text("sim_time/wall_time: %.4f", sim_secs/wall_secs);
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("prescribeQ calls: %i", simulator->prescribeq_calls());
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::Text("States sent to UI thread: %i", simulator->states_sent_to_ui());
        ImGui::Text("Avg. UI overhead: %.1f %%", 100.0 * simulator->ui_overhead());
    }
}

void osmv::Show_model_screen_impl::draw_ui_tab(Application& ui) {
    ImGui::Text("Fps: %.1f", static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::NewLine();

    ImGui::Text("Camera Position:");

    ImGui::NewLine();

    if (ImGui::Button("Front")) {
        // assumes models tend to point upwards in Y and forwards in +X
        theta = pi_f/2.0f;
        phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Back")) {
        // assumes models tend to point upwards in Y and forwards in +X
        theta = 3.0f * (pi_f/2.0f);
        phi = 0.0f;
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    if (ImGui::Button("Left")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        theta = pi_f;
        phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Right")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        theta = 0.0f;
        phi = 0.0f;
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    if (ImGui::Button("Top")) {
        theta = 0.0f;
        phi = pi_f/2.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Bottom")) {
        theta = 0.0f;
        phi = 3.0f * (pi_f/2.0f);
    }

    ImGui::NewLine();

    ImGui::SliderFloat("radius", &radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &theta, 0.0f, 2.0f*pi_f);
    ImGui::SliderFloat("phi", &phi, 0.0f, 2.0f*pi_f);
    ImGui::NewLine();
    ImGui::SliderFloat("pan_x", &pan.x, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_y", &pan.y, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_z", &pan.z, -100.0f, 100.0f);

    ImGui::NewLine();
    ImGui::Text("Lighting:");
    ImGui::SliderFloat("light_x", &light_pos.x, -30.0f, 30.0f);
    ImGui::SliderFloat("light_y", &light_pos.y, -30.0f, 30.0f);
    ImGui::SliderFloat("light_z", &light_pos.z, -30.0f, 30.0f);
    ImGui::ColorEdit3("light_color", light_color);
    ImGui::Checkbox("show_light", &show_light);
    ImGui::Checkbox("show_unit_cylinder", &show_unit_cylinder);
    ImGui::Checkbox("show_floor", &show_floor);
    ImGui::Checkbox("gamma_correction", &gamma_correction);
    ImGui::Checkbox("software_throttle", &ui.software_throttle);
    ImGui::Checkbox("show_mesh_normals", &show_mesh_normals);

    ImGui::NewLine();
    ImGui::Text("Interaction: ");
    if (dragging) {
        ImGui::SameLine();
        ImGui::Text("rotating ");
    }
    if (panning) {
        ImGui::SameLine();
        ImGui::Text("panning ");
    }
}

void osmv::Show_model_screen_impl::draw_coords_tab() {
    ImGui::InputText("search", coords_tab_filter, sizeof(coords_tab_filter));
    ImGui::SameLine();
    if (std::strlen(coords_tab_filter) > 0 and ImGui::Button("clear")) {
        coords_tab_filter[0] = '\0';
    }

    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    ImGui::Checkbox("sort", &sort_coords_by_name);
    ImGui::SameLine();
    ImGui::Checkbox("rotational", &show_rotational_coords);
    ImGui::SameLine();
    ImGui::Checkbox("translational", &show_translational_coords);

    ImGui::Checkbox("coupled", &show_coupled_coords);

    // get coords

    model_coords_swap.clear();
    osim::get_coordinates(model, latest_state, model_coords_swap);


    // apply filters etc.

    int coordtypes_to_filter_out = 0;
    if (not show_rotational_coords) {
        coordtypes_to_filter_out |= osim::Rotational;
    }
    if (not show_translational_coords) {
        coordtypes_to_filter_out |= osim::Translational;
    }
    if (not show_coupled_coords) {
        coordtypes_to_filter_out |= osim::Coupled;
    }

    auto it = std::remove_if(model_coords_swap.begin(), model_coords_swap.end(), [&](osim::Coordinate const& c) {
        if (c.type & coordtypes_to_filter_out) {
            return true;
        }

        if (c.name->find(coords_tab_filter) == c.name->npos) {
            return true;
        }

        return false;
    });
    model_coords_swap.erase(it, model_coords_swap.end());

    if (sort_coords_by_name) {
        std::sort(model_coords_swap.begin(), model_coords_swap.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });
    }


    // render sliders

    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::Text("Coordinates (%i)", static_cast<int>(model_coords_swap.size()));
    ImGui::Separator();

    int i = 0;
    for (osim::Coordinate const& c : model_coords_swap) {
        ImGui::PushID(i++);
        draw_coordinate_slider(c);
        ImGui::PopID();
    }
}

void osmv::Show_model_screen_impl::draw_coordinate_slider(osim::Coordinate const& c) {
    // lock button
    if (c.locked) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, dark_red);
    }

    char const* btn_label = c.locked ? "u" : "l";
    if (ImGui::Button(btn_label)) {
        if (c.locked) {
            osim::unlock_coord(*c.ptr, latest_state);
        } else {
            osim::lock_coord(*c.ptr, latest_state);
        }
        on_user_edited_state();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    float v = c.value;
    if (ImGui::SliderFloat(c.name->c_str(), &v, c.min, c.max)) {
        osim::set_coord_value(*c.ptr, latest_state, static_cast<double>(v));
        on_user_edited_state();
    }

    if (c.locked) {
        ImGui::PopStyleColor();
    }
}

void osmv::Show_model_screen_impl::draw_utils_tab() {
    // tab containing one-off utilities that are useful when diagnosing a model

    ImGui::Text("wrapping surfaces: ");
    ImGui::SameLine();
    if (ImGui::Button("disable")) {
        osim::disable_wrapping_surfaces(model);
        on_user_edited_model();
    }
    ImGui::SameLine();
    if (ImGui::Button("enable")) {
        osim::enable_wrapping_surfaces(model);
        on_user_edited_model();
    }
}

void osmv::Show_model_screen_impl::draw_muscles_tab() {
    // extract muscles details from model
    model_muscs_swap.clear();
    osim::get_muscle_stats(model, latest_state, model_muscs_swap);

    // sort muscles alphabetically by name
    std::sort(model_muscs_swap.begin(), model_muscs_swap.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
        return *m1.name < *m2.name;
    });

    // allow user filtering
    ImGui::InputText("filter muscles", model_tab_filter, sizeof(model_tab_filter));
    ImGui::Separator();

    // draw muscle list
    for (osim::Muscle_stat const& musc : model_muscs_swap) {
        if (musc.name->find(model_tab_filter) != musc.name->npos) {
            ImGui::Text("%s (len = %.2f)", musc.name->c_str(), musc.length);
        }
    }
}

void osmv::Show_model_screen_impl::draw_mas_tab() {
    ImGui::Text("Moment arms");
    ImGui::Separator();

    ImGui::Columns(2);
    // lhs: muscle selection
    {
        model_muscs_swap.clear();
        osim::get_muscle_stats(model, latest_state, model_muscs_swap);

        // usability: sort by name
        std::sort(model_muscs_swap.begin(), model_muscs_swap.end(), [](osim::Muscle_stat const& m1, osim::Muscle_stat const& m2) {
            return *m1.name < *m2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotMuscleSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Muscle_stat const& m : model_muscs_swap) {
            if (ImGui::Selectable(m.name->c_str(), m.name == mas_muscle_selection)) {
                mas_muscle_selection = m.name;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }
    // rhs: coord selection
    {
        model_coords_swap.clear();
        osim::get_coordinates(model, latest_state, model_coords_swap);

        // usability: sort by name
        std::sort(model_coords_swap.begin(), model_coords_swap.end(), [](osim::Coordinate const& c1, osim::Coordinate const& c2) {
            return *c1.name < *c2.name;
        });

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("MomentArmPlotCoordSelection", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false, window_flags);
        for (osim::Coordinate const& c : model_coords_swap) {
            if (ImGui::Selectable(c.name->c_str(), c.name == mas_coord_selection)) {
                mas_coord_selection = c.name;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }
    ImGui::Columns();

    if (mas_muscle_selection and mas_coord_selection) {
        if (ImGui::Button("+ add plot")) {
            auto it = std::find_if(model_muscs_swap.begin(),
                                   model_muscs_swap.end(),
                                   [this](osim::Muscle_stat const& ms) {
                    return ms.name == mas_muscle_selection;
            });
            assert(it != model_muscs_swap.end());

            auto it2 = std::find_if(model_coords_swap.begin(),
                                    model_coords_swap.end(),
                                    [this](osim::Coordinate const& c) {
                    return c.name == mas_coord_selection;
            });
            assert(it2 != model_coords_swap.end());

            auto p = std::make_unique<MA_plot>();
            p->muscle_name = *mas_muscle_selection;
            p->coord_name = *mas_coord_selection;
            p->x_begin = it2->min;
            p->x_end = it2->max;

            // populate y values
            osim::compute_moment_arms(
                *it->ptr,
                latest_state,
                *it2->ptr,
                p->y_vals.data(),
                p->y_vals.size());
            float min = std::numeric_limits<float>::max();
            float max = std::numeric_limits<float>::min();
            for (float v : p->y_vals) {
                min = std::min(min, v);
                max = std::max(max, v);
            }
            p->min = min;
            p->max = max;

            // clear current coordinate selection to prevent user from double
            // clicking plot by accident *but* don't clear muscle because it's
            // feasible that a user will want to plot other coords against the
            // same muscle
            mas_coord_selection = nullptr;

            mas_plots.push_back(std::move(p));
        }
    }

    if (ImGui::Button("refresh TODO")) {
        // iterate through each plot in plots vector and recompute the moment
        // arms from the UI's current model + state
        throw std::runtime_error{"refreshing moment arm plots NYI"};
    }

    if (not mas_plots.empty() and ImGui::Button("clear all")) {
        mas_plots.clear();
    }

    ImGui::Separator();

    ImGui::Columns(2);
    for (size_t i = 0; i < mas_plots.size(); ++i) {
        MA_plot const& p = *mas_plots[i];
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth()/2.0f);
        ImGui::PlotLines(
            "",
            p.y_vals.data(),
            static_cast<int>(p.y_vals.size()),
            0,
            nullptr,
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::max(),
            ImVec2(0, 100.0f));
        ImGui::NextColumn();
        ImGui::Text("muscle: %s", p.muscle_name.c_str());
        ImGui::Text("coord : %s", p.coord_name.c_str());
        ImGui::Text("min   : %f", static_cast<double>(p.min));
        ImGui::Text("max   : %f", static_cast<double>(p.max));
        if (ImGui::Button("delete")) {
            auto it = mas_plots.begin() + i;
            mas_plots.erase(it, it + 1);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns();
}
