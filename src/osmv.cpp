#include "opensim_wrapper.hpp"
#include "osmv_config.hpp"

// sdl
#include <SDL.h>
#undef main

// glew
#include <GL/glew.h>

// glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui
#include "imgui.h"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

// c++
#include <string>
#include <exception>
#include <optional>
#include <vector>
#include <sstream>
#include <chrono>
#include <iostream>
#include <tuple>


using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
constexpr float pi_f = static_cast<float>(M_PI);
constexpr double pi_d = M_PI;

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Thin C++ wrappers around SDL2, so that downstream code can use SDL2 in an
// exception-safe way
namespace sdl {
    // RAII wrapper for the SDL library that calls SDL_Quit() on dtor
    //     https://wiki.libsdl.org/SDL_Quit
    class [[nodiscard]] Context final {
        Context() {}
        friend Context Init(Uint32);
    public:
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            SDL_Quit();
        }
    };

    // RAII'ed version of SDL_Init() that returns a lifetime wrapper that calls
    // SDL_Quit on destruction:
    //     https://wiki.libsdl.org/SDL_Init
    //     https://wiki.libsdl.org/SDL_Quit
    Context Init(Uint32 flags) {
        if (SDL_Init(flags) != 0) {
            throw std::runtime_error{"SDL_Init: failed: "s + SDL_GetError()};
        }
        return Context{};
    }

    // RAII wrapper around SDL_Window that calls SDL_DestroyWindow on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class [[nodiscard]] Window final {
        SDL_Window* ptr;
    public:
        Window(SDL_Window* _ptr) : ptr{_ptr} {
            if (ptr == nullptr) {
                throw std::runtime_error{"sdl::Window: window passed in was null: "s + SDL_GetError()};
            }
        }
        Window(Window const&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) = delete;
        ~Window() noexcept {
            SDL_DestroyWindow(ptr);
        }

        operator SDL_Window* () noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateWindow. The name is a typo because
    // `CreateWindow` is defined in the preprocessor:
    //     https://wiki.libsdl.org/SDL_CreateWindow
    Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags) {
        SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);

        if (win == nullptr) {
            throw std::runtime_error{"SDL_CreateWindow failed: "s + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around an SDL_Renderer that calls SDL_DestroyRenderer on dtor
    //     https://wiki.libsdl.org/SDL_Renderer
    //     https://wiki.libsdl.org/SDL_DestroyRenderer
    class [[nodiscard]] Renderer final {
        SDL_Renderer* ptr;
    public:
        Renderer(SDL_Renderer* _ptr) : ptr{ _ptr } {
            if (ptr == nullptr) {
                throw std::runtime_error{"sdl::Renderer constructor called with null argument: "s + SDL_GetError()};
            }
        }
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept {
            SDL_DestroyRenderer(ptr);
        }

        operator SDL_Renderer* () noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateRenderer
    //     https://wiki.libsdl.org/SDL_CreateRenderer
    Renderer CreateRenderer(SDL_Window* w, int index, Uint32 flags) {
        SDL_Renderer* r = SDL_CreateRenderer(w, index, flags);

        if (r == nullptr) {
            throw std::runtime_error{"SDL_CreateRenderer: failed: "s + SDL_GetError()};
        }

        return Renderer{r};
    }

    // RAII wrapper around SDL_GLContext that calls SDL_GL_DeleteContext on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
        SDL_GLContext ctx;
    public:
        GLContext(SDL_GLContext _ctx) : ctx{ _ctx } {
            if (ctx == nullptr) {
                throw std::runtime_error{"sdl::GLContext: constructor called with null context"};
            }
        }
        GLContext(GLContext const&) = delete;
        GLContext(GLContext&&) = delete;
        GLContext& operator=(GLContext const&) = delete;
        GLContext& operator=(GLContext&&) = delete;
        ~GLContext() noexcept {
            SDL_GL_DeleteContext(ctx);
        }

        operator SDL_GLContext () noexcept {
            return ctx;
        }
    };

    // RAII'ed version of SDL_GL_CreateContext:
    //     https://wiki.libsdl.org/SDL_GL_CreateContext
    GLContext GL_CreateContext(SDL_Window* w) {
        SDL_GLContext ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr) {
            throw std::runtime_error{"SDL_GL_CreateContext failed: "s + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // RAII wrapper for SDL_Surface that calls SDL_FreeSurface on dtor:
    //     https://wiki.libsdl.org/SDL_Surface
    //     https://wiki.libsdl.org/SDL_FreeSurface
    class Surface final {
        SDL_Surface* handle;
    public:
        Surface(SDL_Surface* _handle) :
            handle{_handle} {

            if (handle == nullptr) {
                throw std::runtime_error{"sdl::Surface: null handle passed into constructor"};
            }
        }
        Surface(Surface const&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface const&) = delete;
        Surface& operator=(Surface&&) = delete;
        ~Surface() noexcept {
            SDL_FreeSurface(handle);
        }

        operator SDL_Surface*() noexcept {
            return handle;
        }
        SDL_Surface* operator->() const noexcept {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateRGBSurface:
    //     https://wiki.libsdl.org/SDL_CreateRGBSurface
    Surface CreateRGBSurface(
        Uint32 flags,
        int    width,
        int    height,
        int    depth,
        Uint32 Rmask,
        Uint32 Gmask,
        Uint32 Bmask,
        Uint32 Amask) {

        SDL_Surface* handle = SDL_CreateRGBSurface(
            flags,
            width,
            height,
            depth,
            Rmask,
            Gmask,
            Bmask,
            Amask);

        if (handle == nullptr) {
            throw std::runtime_error{"SDL_CreateRGBSurface: "s + SDL_GetError()};
        }

        return Surface{handle};
    }

    // RAII wrapper around SDL_LockSurface/SDL_UnlockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    //     https://wiki.libsdl.org/SDL_UnlockSurface
    class Surface_lock final {
        SDL_Surface* ptr;
    public:
        Surface_lock(SDL_Surface* s) : ptr{s} {
            if (SDL_LockSurface(ptr) != 0) {
                throw std::runtime_error{"SDL_LockSurface failed: "s + SDL_GetError()};
            }
        }
        Surface_lock(Surface_lock const&) = delete;
        Surface_lock(Surface_lock&&) = delete;
        Surface_lock& operator=(Surface_lock const&) = delete;
        Surface_lock& operator=(Surface_lock&&) = delete;
        ~Surface_lock() noexcept {
            SDL_UnlockSurface(ptr);
        }
    };

    // RAII'ed version of SDL_LockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    Surface_lock LockSurface(SDL_Surface* s) {
        return Surface_lock{s};
    }

    // RAII wrapper around SDL_Texture that calls SDL_DestroyTexture on dtor:
    //     https://wiki.libsdl.org/SDL_Texture
    //     https://wiki.libsdl.org/SDL_DestroyTexture
    class Texture final {
        SDL_Texture* handle;
    public:
        Texture(SDL_Texture* _handle) : handle{_handle} {
            if (handle == nullptr) {
                throw std::runtime_error{"sdl::Texture: null handle passed into constructor"};
            }
        }
        Texture(Texture const&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept {
            SDL_DestroyTexture(handle);
        }

        operator SDL_Texture*() {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateTextureFromSurface:
    //     https://wiki.libsdl.org/SDL_CreateTextureFromSurface
    Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s) {
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        if (t == nullptr) {
            throw std::runtime_error{"SDL_CreateTextureFromSurface failed: "s + SDL_GetError()};
        }
        return Texture{t};
    }

    // https://wiki.libsdl.org/SDL_RenderCopy
    void RenderCopy(SDL_Renderer* r, SDL_Texture* t, SDL_Rect* src, SDL_Rect* dest) {
        int rv = SDL_RenderCopy(r, t, src, dest);
        if (rv != 0) {
            throw std::runtime_error{"SDL_RenderCopy failed: "s + SDL_GetError()};
        }
    }

    // https://wiki.libsdl.org/SDL_RenderPresent
    void RenderPresent(SDL_Renderer* r) {
        // this method exists just so that the namespace-based naming is
        // consistent
        SDL_RenderPresent(r);
    }

    // https://wiki.libsdl.org/SDL_GetWindowSize
    void GetWindowSize(SDL_Window* window, int* w, int* h) {
        SDL_GetWindowSize(window, w, h);
    }

    void GL_SetSwapInterval(int interval) {
        int rv = SDL_GL_SetSwapInterval(interval);

        if (rv != 0) {
            throw std::runtime_error{"SDL_GL_SetSwapInterval failed: "s + SDL_GetError()};
        }
    }

    using Event = SDL_Event;
    using Rect = SDL_Rect;

    class Timer final {
        SDL_TimerID handle;
    public:
        Timer(SDL_TimerID _handle) : handle{_handle} {
            if (handle == 0) {
                throw std::runtime_error{"0 timer handle passed to sdl::Timer"};
            }
        }
        Timer(Timer const&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer const&) = delete;
        Timer& operator=(Timer&&) = delete;
        ~Timer() noexcept {
            SDL_RemoveTimer(handle);
        }
    };

    Timer AddTimer(Uint32 interval,
                         SDL_TimerCallback callback,
                         void* param) {
        SDL_TimerID handle = SDL_AddTimer(interval, callback, param);
        if (handle == 0) {
            throw std::runtime_error{"SDL_AddTimer failed: "s + SDL_GetError()};
        }

        return Timer{handle};
    }
}

namespace gl {
    std::string to_string(GLubyte const* err_string) {
        return std::string{reinterpret_cast<char const*>(err_string)};
    }

    void assert_no_errors(char const* func) {
        std::vector<GLenum> errors;
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
            errors.push_back(error);
        }

        if (errors.empty()) {
            return;
        }

        std::stringstream msg;
        msg << func << " failed";
        if (errors.size() == 1) {
            msg << ": ";
        } else {
            msg << " with " << errors.size() << " errors: ";
        }
        for (auto it = errors.begin(); it != errors.end()-1; ++it) {
            msg << to_string(gluErrorString(*it)) << ", ";
        }
        msg << to_string(gluErrorString(errors.back()));

        throw std::runtime_error{msg.str()};
    }

    class Program final {
        GLuint handle = glCreateProgram();
    public:
        Program() {
            if (handle == 0) {
                throw std::runtime_error{"glCreateProgram() failed"};
            }
        }
        Program(Program const&) = delete;
        Program(Program&& tmp) :
            handle{tmp.handle} {
            tmp.handle = 0;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&&) = delete;
        ~Program() noexcept {
            if (handle != 0) {
                glDeleteProgram(handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void UseProgram(Program& p) {
        glUseProgram(p);
        assert_no_errors("glUseProgram");
    }

    void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    static std::optional<std::string> get_shader_compile_errors(GLuint shader) {
        GLint params = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
        if (params == GL_FALSE) {
            GLint log_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

            std::vector<GLchar> errmsg(log_len);
            glGetShaderInfoLog(shader, log_len, &log_len, errmsg.data());

            std::stringstream ss;
            ss << "glCompileShader() failed: ";
            ss << errmsg.data();
            return std::optional<std::string>{ss.str()};
        }
        return std::nullopt;
    }

    struct Shader {
        static Shader Compile(GLenum shaderType, char const* src) {
            Shader shader{shaderType};
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            if (auto compile_errors = get_shader_compile_errors(shader); compile_errors) {
                throw std::runtime_error{"error compiling vertex shader: "s + compile_errors.value()};
            }

            return shader;
        }
    private:
        GLuint handle;
    public:
        Shader(GLenum shaderType) :
            handle{glCreateShader(shaderType)} {

            if (handle == 0) {
                throw std::runtime_error{"glCreateShader() failed"};
            }
        }
        Shader(Shader const&) = delete;
        Shader(Shader&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Shader& operator=(Shader const&) = delete;
        Shader& operator=(Shader&&) = delete;
        ~Shader() noexcept {
            if (handle != 0) {
                glDeleteShader(handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void AttachShader(Program& p, Shader& s) {
        glAttachShader(p, s);
        assert_no_errors("glAttachShader");
    }

    struct Vertex_shader final : public Shader {
        static Vertex_shader Compile(char const* src) {
            return Vertex_shader{Shader::Compile(GL_VERTEX_SHADER, src)};
        }
    };

    struct Fragment_shader final : public Shader {
        static Fragment_shader Compile(char const* src) {
            return Fragment_shader{Shader::Compile(GL_FRAGMENT_SHADER, src)};
        }
    };

    void LinkProgram(gl::Program& prog) {
        glLinkProgram(prog);

        GLint link_status = GL_FALSE;
        glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
        if (link_status == GL_FALSE) {
            GLint log_len = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);

            std::vector<GLchar> errmsg(log_len);
            glGetProgramInfoLog(prog, static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

            std::stringstream ss;
            ss << "OpenGL: glLinkProgram() failed: ";
            ss << errmsg.data();
            throw std::runtime_error{ss.str()};
        }
    }

    class Uniform {
    protected:
        GLint handle;
    public:
        Uniform(Program& p, char const* name) :
            handle{glGetUniformLocation(p, name)} {

            if (handle == -1) {
                throw std::runtime_error{"glGetUniformLocation() failed: cannot get "s + name};
            }
        }

        operator GLint () noexcept {
            return handle;
        }
    };

    struct Uniform1f final : public Uniform {
        Uniform1f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(Uniform1f& u, GLfloat value) {
        glUniform1f(u, value);
    }

    struct Uniform1i final : public Uniform {
        Uniform1i(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(Uniform1i& u, GLint value) {
        glUniform1i(u, value);
    }

    struct UniformMatrix4fv final : public Uniform {
        UniformMatrix4fv(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(UniformMatrix4fv& u, GLfloat const* value) {
        glUniformMatrix4fv(u, 1, false, value);
    }

    struct UniformVec4f final : public Uniform {
        UniformVec4f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    struct UniformVec3f final : public Uniform {
        UniformVec3f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    class Attribute final {
        GLint handle;
    public:
        Attribute(Program& p, char const* name) :
            handle{glGetAttribLocation(p, name)} {
            if (handle == -1) {
                throw std::runtime_error{"glGetAttribLocation() failed: cannot get "s + name};
            }
        }

        operator GLint () noexcept {
            return handle;
        }
    };

    void VertexAttributePointer(Attribute& a,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                const void * pointer) {
        glVertexAttribPointer(a, size, type, normalized, stride, pointer);
    }

    void EnableVertexAttribArray(Attribute& a) {
        glEnableVertexAttribArray(a);
    }

    class Buffer {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Buffer(GLenum target) {
            glGenBuffers(1, &handle);
        }
        Buffer(Buffer const&) = delete;
        Buffer(Buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Buffer& operator=(Buffer const&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        ~Buffer() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteBuffers(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void BindBuffer(GLenum target, Buffer& buffer) {
        glBindBuffer(target, buffer);
    }

    void BufferData(GLenum target, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
    }

    struct Array_buffer final : public Buffer {
        Array_buffer() : Buffer{GL_ARRAY_BUFFER} {
        }
    };

    void BindBuffer(Array_buffer& buffer) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
    }

    void BufferData(Array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(GL_ARRAY_BUFFER, num_bytes, data, usage);
    }

    struct Element_array_buffer final : public Buffer {
        Element_array_buffer() : Buffer{GL_ELEMENT_ARRAY_BUFFER} {
        }
    };

    void BindBuffer(Element_array_buffer& buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    }

    void BufferData(Element_array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, data, usage);
    }

    class Vertex_array final {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Vertex_array() {
            glGenVertexArrays(1, &handle);
        }
        Vertex_array(Vertex_array const&) = delete;
        Vertex_array(Vertex_array&& tmp) :
            handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Vertex_array& operator=(Vertex_array const&) = delete;
        Vertex_array& operator=(Vertex_array&&) = delete;
        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao);
    }

    void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    class Texture {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Texture() {
            glGenTextures(1, &handle);
        }
        Texture(Texture const&) = delete;
        Texture(Texture&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteTextures(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    struct Texture_2d final : public Texture {
        Texture_2d() : Texture{} {
        }
    };

    void BindTexture(Texture_2d& texture) {
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GenerateMipMap(Texture_2d&) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

namespace glglm {
    void Uniform(gl::UniformMatrix4fv& u, glm::mat4 const& mat) {
        gl::Uniform(u, glm::value_ptr(mat));
    }

    void Uniform(gl::UniformVec4f& u, glm::vec4 const& v) {
        glUniform4f(u, v.x, v.y, v.z, v.w);
    }

    void Uniform(gl::UniformVec3f& u, glm::vec3 const& v) {
        glUniform3f(u, v.x, v.y, v.z);
    }
}

namespace glm {
    std::ostream& operator<<(std::ostream& o, vec3 const& v) {
        return o << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    }

    std::ostream& operator<<(std::ostream& o, vec4 const& v) {
        return o << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    }

    std::ostream& operator<<(std::ostream& o, mat4 const& m) {
        o << "[";
        for (auto i = 0U; i < 3; ++i) {
            o << m[i];
            o << ", ";
        }
        o << m[3];
        o << "]";
        return o;
    }
}

namespace ig {
    struct Context final {
        Context() {
            ImGui::CreateContext();
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            ImGui::DestroyContext();
        }
    };

    struct SDL2_Context final {
        SDL2_Context(SDL_Window* w, SDL_GLContext gl) {
            ImGui_ImplSDL2_InitForOpenGL(w, gl);
        }
        SDL2_Context(SDL2_Context const&) = delete;
        SDL2_Context(SDL2_Context&&) = delete;
        SDL2_Context& operator=(SDL2_Context const&) = delete;
        SDL2_Context& operator=(SDL2_Context&&) = delete;
        ~SDL2_Context() noexcept {
            ImGui_ImplSDL2_Shutdown();
        }
    };

    struct OpenGL3_Context final {
        OpenGL3_Context(char const* version) {
            ImGui_ImplOpenGL3_Init(version);
        }
        OpenGL3_Context(OpenGL3_Context const&) = delete;
        OpenGL3_Context(OpenGL3_Context&&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context const&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context&&) = delete;
        ~OpenGL3_Context() noexcept {
            ImGui_ImplOpenGL3_Shutdown();
        }
    };
}

namespace  osim {
    std::ostream& operator<<(std::ostream& o, osim::Cylinder const& c) {
        o << "cylinder:"  << std::endl
          << "    scale = " << c.scale << std::endl
          << "    rgba = " << c.rgba << std::endl
          << "    transform = " << c.transform << std::endl;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, osim::Line const& l) {
        o << "line:" << std::endl
          << "     p1 = " << l.p1 << std::endl
          << "     p2 = " << l.p2 << std::endl
          << "     rgba = " << l.rgba << std::endl;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, osim::Sphere const& s) {
        o << "sphere:" << std::endl
          << "    transform = " << s.transform << std::endl
          << "    color = " << s.rgba << std::endl
          << "    radius = " << s.radius << std::endl;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, osim::Mesh const& m) {
        o << "mesh:" << std::endl
          << "    transform = " << m.transform << std::endl
          << "    scale = " << m.scale << std::endl
          << "    rgba = " << m.rgba << std::endl
          << "    num_triangles = " << m.triangles.size() << std::endl;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, osim::Geometry const& g) {
        std::visit([&](auto concrete) { o << concrete; }, g);
        return o;
    }
}

namespace ui {
    sdl::Window init_gl_window(sdl::Context&) {
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, OSC_GL_CTX_FLAGS);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, OSC_GL_CTX_FLAGS);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, OSC_GL_CTX_MAJOR_VERSION);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, OSC_GL_CTX_MINOR_VERSION);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_DEPTH_SIZE, 24);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_STENCIL_SIZE, 8);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLEBUFFERS, 1);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLESAMPLES, 16);

        return sdl::CreateWindoww(
            "osmv " OSMV_VERSION_STRING,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1024,
            768,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    }

    struct State final {
        sdl::Context context = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
        sdl::Window window = init_gl_window(context);
        sdl::GLContext gl = sdl::GL_CreateContext(window);

        State() {
            gl::assert_no_errors("ui::State::constructor::onEnter");
            sdl::GL_SetSwapInterval(0);  // disable VSYNC

            // enable SDL's OpenGL context
            if (SDL_GL_MakeCurrent(window, gl) != 0) {
                throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s  + SDL_GetError()};
            }

            // initialize GLEW, which is what imgui is using under the hood
            if (auto err = glewInit(); err != GLEW_OK) {
                std::stringstream ss;
                ss << "glewInit() failed: ";
                ss << glewGetErrorString(err);
                throw std::runtime_error{ss.str()};
            }

            DEBUG_PRINT("OpenGL info: %s: %s (%s) /w GLSL: %s\n",
                        glGetString(GL_VENDOR),
                        glGetString(GL_RENDERER),
                        glGetString(GL_VERSION),
                        glGetString(GL_SHADING_LANGUAGE_VERSION));

            gl::assert_no_errors("ui::State::constructor::onExit");
        }
    };
}

namespace examples::imgui {
    static const char vertex_shader_src[] = OSC_GLSL_VERSION R"(
        // A non-texture-mapping Gouraud shader that just adds some basic
        // (diffuse / specular) highlights to the model

        uniform mat4 projMat;
        uniform mat4 viewMat;
        uniform mat4 modelMat;

        uniform vec4 rgba;
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;

        in vec3 location;
        in vec3 in_normal;

        out vec4 frag_color;

        void main() {
            // apply xforms (model, view, perspective) to vertex
            gl_Position = projMat * viewMat * modelMat * vec4(location, 1.0f);
            // passthrough the normals (used by frag shader)
            vec3 normal = in_normal;
            // pass fragment pos in world coordinates to frag shader
            vec3 frag_pos = vec3(modelMat * vec4(location, 1.0f));


            // normalized surface normal
            vec3 norm = normalize(normal);
            // direction of light, relative to fragment, in world coords
            vec3 light_dir = normalize(lightPos - frag_pos);

            // strength of diffuse (Phong model) lighting
            float diffuse_strength = 0.3f;
            float diff = max(dot(norm, light_dir), 0.0);
            vec3 diffuse = diffuse_strength * diff * lightColor;

            // strength of ambient (Phong model) lighting
            float ambient_strength = 0.5f;
            vec3 ambient = ambient_strength * lightColor;

            // strength of specular (Blinn-Phong model) lighting
            // Blinn-Phong is a modified Phong model that
            float specularStrength = 0.1f;
            vec3 lightDir = normalize(lightPos - frag_pos);
            vec3 viewDir = normalize(viewPos - frag_pos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            vec3 reflectDir = reflect(-light_dir, norm);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 rgb = (ambient + diffuse + specular) * rgba.rgb;
            frag_color = vec4(rgb, rgba.a);
        }
    )";

    static const char frag_shader_src[] = OSC_GLSL_VERSION R"(
        // passthrough frag shader: rendering does not use textures and we're
        // using a "good enough" Gouraud shader (rather than a Phong shader,
        // which is per-fragment).

        in vec4 frag_color;
        out vec4 color;

        void main() {
            color = frag_color;
        }
    )";

    // Vector of 3 floats with no padding, so that it can be passed to OpenGL
    struct Vec3 {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    };

    struct Mesh_point {
        Vec3 position;
        Vec3 normal;
    };

    // Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
    std::vector<Mesh_point> unit_sphere_triangles() {
        // this is a shitty alg that produces a shitty UV sphere. I don't have
        // enough time to implement something better, like an isosphere, or
        // something like a patched sphere:
        //
        // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
        //
        // This one is adapted from:
        //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere

        size_t sectors = 32;
        size_t stacks = 16;

        // polar coords, with [0, 0, -1] pointing towards the screen with polar
        // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
        // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
        std::vector<Mesh_point> points;

        float theta_step = 2.0f*pi_f / sectors;
        float phi_step = pi_f / stacks;

        for (size_t stack = 0; stack <= stacks; ++stack) {
            float phi = pi_f/2.0f - static_cast<float>(stack)*phi_step;
            float y = sin(phi);

            for (unsigned sector = 0; sector <= sectors; ++sector) {
                float theta = sector * theta_step;
                float x = sin(theta) * cos(phi);
                float z = -cos(theta) * cos(phi);
                points.push_back(Mesh_point{
                    .position = {x, y, z},
                    .normal = {x, y, z},  // sphere is at the origin, so nothing fancy needed
                });
            }
        }

        // the points are not triangles. They are *points of a triangle*, so the
        // points must be triangulated
        std::vector<Mesh_point> triangles;

        for (size_t stack = 0; stack < stacks; ++stack) {
            size_t k1 = stack * (sectors + 1);
            size_t k2 = k1 + sectors + 1;

            for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
                // 2 triangles per sector - excluding the first and last stacks
                // (which contain one triangle, at the poles)
                Mesh_point p1 = points.at(k1);
                Mesh_point p2 = points.at(k2);
                Mesh_point p1_plus1 = points.at(k1+1u);
                Mesh_point p2_plus1 = points.at(k2+1u);

                if (stack != 0) {
                    triangles.push_back(p1);
                    triangles.push_back(p2);
                    triangles.push_back(p1_plus1);
                }

                if (stack != (stacks-1)) {
                    triangles.push_back(p1_plus1);
                    triangles.push_back(p2);
                    triangles.push_back(p2_plus1);
                }
            }
        }

        return triangles;
    }

    // Returns triangles for a "unit" cylinder with `num_sides` sides.
    //
    // Here, "unit" means:
    //
    // - radius == 1.0f
    // - top == [0.0f, 0.0f, -1.0f]
    // - bottom == [0.0f, 0.0f, +1.0f]
    // - (so the height is 2.0f, not 1.0f)
    std::vector<Mesh_point> unit_cylinder_triangles(size_t num_sides) {
        // TODO: this is dumb because a cylinder can be EBO-ed quite easily, which
        //       would reduce the amount of vertices needed
        if (num_sides < 3) {
            throw std::runtime_error{"cannot create a cylinder with fewer than 3 sides"};
        }

        std::vector<Mesh_point> rv;
        rv.reserve(2u*3u*num_sides + 6u*num_sides);

        float step_angle = (2.0f*pi_f)/num_sides;
        float top_z = -1.0f;
        float bottom_z = +1.0f;

        // top
        {
            Vec3 normal = {0.0f, 0.0f, -1.0f};
            Mesh_point top_middle = {
                .position = {0.0f, 0.0f, top_z},
                .normal = normal,
            };
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i*step_angle;
                float theta_end = (i+1)*step_angle;
                rv.push_back(top_middle);
                rv.push_back(Mesh_point {
                    .position = {
                        .x = sin(theta_start),
                        .y = cos(theta_start),
                        .z = top_z,
                    },
                    .normal = normal,
                });
                rv.push_back(Mesh_point {
                     .position = {
                        .x = sin(theta_end),
                        .y = cos(theta_end),
                        .z = top_z,
                    },
                    .normal = normal,
                });
            }
        }

        // bottom
        {
            Vec3 bottom_normal = {0.0f, 0.0f, -1.0f};
            Mesh_point top_middle = {
                .position = {0.0f, 0.0f, bottom_z},
                .normal = bottom_normal,
            };
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i*step_angle;
                float theta_end = (i+1)*step_angle;

                rv.push_back(top_middle);
                rv.push_back(Mesh_point {
                    .position = {
                        .x = sin(theta_start),
                        .y = cos(theta_start),
                        .z = bottom_z,
                    },
                    .normal = bottom_normal,
                });
                rv.push_back(Mesh_point {
                     .position = {
                        .x = sin(theta_end),
                        .y = cos(theta_end),
                        .z = bottom_z,
                    },
                    .normal = bottom_normal,
                });
            }
        }

        // sides
        {
            float norm_start = step_angle/2.0f;
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i * step_angle;
                float theta_end = theta_start + step_angle;
                float norm_theta = theta_start + norm_start;

                Vec3 normal = {sin(norm_theta), cos(norm_theta), 0.0f};
                Vec3 top1 = {sin(theta_start), cos(theta_start), top_z};
                Vec3 top2 = {sin(theta_end), cos(theta_end), top_z};
                Vec3 bottom1 = top1;
                bottom1.z = bottom_z;
                Vec3 bottom2 = top2;
                bottom2.z = bottom_z;

                rv.push_back(Mesh_point{top1, normal});
                rv.push_back(Mesh_point{top2, normal});
                rv.push_back(Mesh_point{bottom1, normal});

                rv.push_back(Mesh_point{bottom1, normal});
                rv.push_back(Mesh_point{bottom2, normal});
                rv.push_back(Mesh_point{top2, normal});
            }
        }

        return rv;
    }

    // Returns triangles for a "simbody" cylinder with `num_sides` sides.
    //
    // This matches simbody-visualizer.cpp's definition of a cylinder, which
    // is:
    //
    // radius
    //     1.0f
    // top
    //     [0.0f, 1.0f, 0.0f]
    // bottom
    //     [0.0f, -1.0f, 0.0f]
    //
    // see simbody-visualizer.cpp::makeCylinder for my source material
    std::vector<Mesh_point> simbody_cylinder_triangles(size_t num_sides) {
        // TODO: this is dumb because a cylinder can be EBO-ed quite easily, which
        //       would reduce the amount of vertices needed
        if (num_sides < 3) {
            throw std::runtime_error{"cannot create a cylinder with fewer than 3 sides"};
        }

        std::vector<Mesh_point> rv;
        rv.reserve(2*3*num_sides + 6*num_sides);

        float step_angle = (2.0f*pi_f)/num_sides;
        float top_y = +1.0f;
        float bottom_y = -1.0f;

        // top
        {
            Vec3 normal = {0.0f, 1.0f, 0.0f};
            Mesh_point top_middle = {
                .position = {0.0f, top_y, 0.0f},
                .normal = normal,
            };
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i*step_angle;
                float theta_end = (i+1)*step_angle;
                rv.push_back(top_middle);
                rv.push_back(Mesh_point {
                    .position = {
                        .x = cos(theta_start),
                        .y = top_y,
                        .z = sin(theta_start),
                    },
                    .normal = normal,
                });
                rv.push_back(Mesh_point {
                     .position = {
                        .x = cos(theta_end),
                        .y = top_y,
                        .z = sin(theta_end),
                    },
                    .normal = normal,
                });
            }
        }

        // bottom
        {
            Vec3 bottom_normal = {0.0f, -1.0f, 0.0f};
            Mesh_point top_middle = {
                .position = {0.0f, bottom_y, 0.0f},
                .normal = bottom_normal,
            };
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i*step_angle;
                float theta_end = (i+1)*step_angle;

                rv.push_back(top_middle);
                rv.push_back(Mesh_point {
                    .position = {
                        .x = cos(theta_start),
                        .y = bottom_y,
                        .z = sin(theta_start),
                    },
                    .normal = bottom_normal,
                });
                rv.push_back(Mesh_point {
                     .position = {
                        .x = cos(theta_end),
                        .y = bottom_y,
                        .z = sin(theta_end),
                    },
                    .normal = bottom_normal,
                });
            }
        }

        // sides
        {
            float norm_start = step_angle/2.0f;
            for (auto i = 0U; i < num_sides; ++i) {
                float theta_start = i * step_angle;
                float theta_end = theta_start + step_angle;
                float norm_theta = theta_start + norm_start;

                Vec3 normal = {cos(norm_theta), 0.0f, sin(norm_theta)};
                Vec3 top1 = {cos(theta_start), top_y, sin(theta_start)};
                Vec3 top2 = {cos(theta_end), top_y, sin(theta_end)};

                Vec3 bottom1 = top1;
                bottom1.y = bottom_y;
                Vec3 bottom2 = top2;
                bottom2.y = bottom_y;

                // draw 2 triangles per rectangular side
                rv.push_back(Mesh_point{top1, normal});
                rv.push_back(Mesh_point{top2, normal});
                rv.push_back(Mesh_point{bottom1, normal});

                rv.push_back(Mesh_point{bottom1, normal});
                rv.push_back(Mesh_point{bottom2, normal});
                rv.push_back(Mesh_point{top2, normal});
            }
        }

        return rv;
    }

    // Basic mesh composed of triangles with normals for all vertices
    struct Triangle_mesh {
        GLsizei num_verts;
        gl::Array_buffer vbo;
        gl::Vertex_array vao;

        Triangle_mesh(gl::Attribute& in_attr,
                      gl::Attribute& normal_attr,
                      std::vector<Mesh_point> const& points) :
            num_verts(static_cast<GLsizei>(points.size())) {

            gl::BindVertexArray(vao);
            {
                gl::BindBuffer(vbo);
                gl::BufferData(vbo, sizeof(Mesh_point) * points.size(), points.data(), GL_STATIC_DRAW);
                gl::VertexAttributePointer(in_attr, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), 0);
                gl::VertexAttributePointer(normal_attr, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_point), (void*)sizeof(Vec3));
                gl::EnableVertexAttribArray(in_attr);
            }
            gl::BindVertexArray();
        }
    };

    Triangle_mesh gen_cylinder_mesh(gl::Attribute& in_attr,
                                    gl::Attribute& normal_attr,
                                    unsigned num_sides) {
        return Triangle_mesh{
            in_attr,
            normal_attr,
            simbody_cylinder_triangles(num_sides),
        };
    }

    Triangle_mesh gen_sphere_mesh(gl::Attribute& in_attr,
                                  gl::Attribute& normal_attr) {
        auto points = unit_sphere_triangles();
        return Triangle_mesh{in_attr, normal_attr, points};
    }

    struct App_static_glstate {
        gl::Program program;

        gl::UniformMatrix4fv projMat;
        gl::UniformMatrix4fv viewMat;
        gl::UniformMatrix4fv modelMat;
        gl::UniformVec4f rgba;
        gl::UniformVec3f light_pos;
        gl::UniformVec3f light_color;
        gl::UniformVec3f view_pos;

        gl::Attribute location;
        gl::Attribute in_normal;

        Triangle_mesh cylinder;
        Triangle_mesh sphere;
    };

    App_static_glstate initialize() {
        auto program = gl::Program{};
        auto vertex_shader = gl::Vertex_shader::Compile(vertex_shader_src);
        gl::AttachShader(program, vertex_shader);
        auto frag_shader = gl::Fragment_shader::Compile(frag_shader_src);
        gl::AttachShader(program, frag_shader);

        gl::LinkProgram(program);

        auto projMat = gl::UniformMatrix4fv{program, "projMat"};
        auto viewMat = gl::UniformMatrix4fv{program, "viewMat"};
        auto modelMat = gl::UniformMatrix4fv{program, "modelMat"};
        auto rgba = gl::UniformVec4f{program, "rgba"};
        auto light_pos = gl::UniformVec3f{program, "lightPos"};
        auto light_color = gl::UniformVec3f{program, "lightColor"};
        auto view_pos = gl::UniformVec3f{program, "viewPos"};

        auto in_position = gl::Attribute{program, "location"};
        auto in_normal = gl::Attribute{program, "in_normal"};

        return App_static_glstate {
            .program = std::move(program),

            .projMat = std::move(projMat),
            .viewMat = std::move(viewMat),
            .modelMat = std::move(modelMat),
            .rgba = std::move(rgba),
            .light_pos = std::move(light_pos),
            .light_color = std::move(light_color),
            .view_pos = std::move(view_pos),

            .location = std::move(in_position),
            .in_normal = std::move(in_normal),

            .cylinder = gen_cylinder_mesh(in_position, in_normal, 24),
            .sphere = gen_sphere_mesh(in_position, in_normal),
        };
    }

    struct Line {
        gl::Array_buffer vbo;
        gl::Vertex_array vao;
        osim::Line data;

        Line(gl::Attribute& in_attr, osim::Line const& _data) :
            data{_data} {
            Vec3 points[2] = {
                {_data.p1.x, _data.p1.y, _data.p1.z},
                {_data.p2.x, _data.p2.y, _data.p2.z},
            };
            gl::BindVertexArray(vao);
            {
                gl::BindBuffer(vbo);
                gl::BufferData(vbo, sizeof(points), points, GL_STATIC_DRAW);
                gl::VertexAttributePointer(in_attr, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
                gl::EnableVertexAttribArray(in_attr);
            }
            gl::BindVertexArray();
        }
    };

    std::tuple<Mesh_point, Mesh_point, Mesh_point> to_mesh_points(osim::Triangle const& t) {
        glm::vec3 normal = (t.p2 - t.p1) * (t.p3 - t.p1);
        Vec3 normal_vec3 = Vec3{normal.x, normal.y, normal.z};

        return {
            Mesh_point{Vec3{t.p1.x, t.p1.y, t.p1.z}, normal_vec3},
            Mesh_point{Vec3{t.p2.x, t.p2.y, t.p2.z}, normal_vec3},
            Mesh_point{Vec3{t.p3.x, t.p3.y, t.p3.z}, normal_vec3}
        };
    }

    // TODO: this is hacked together
    Triangle_mesh make_mesh(gl::Attribute& in_attr, gl::Attribute& in_normal, osim::Mesh const& data) {
        std::vector<Mesh_point> triangles;
        triangles.reserve(3*data.triangles.size());
        for (osim::Triangle const& t : data.triangles) {
            auto [p1, p2, p3] = to_mesh_points(t);
            triangles.emplace_back(p1);
            triangles.emplace_back(p2);
            triangles.emplace_back(p3);
        }
        return Triangle_mesh{in_attr, in_normal, triangles};
    }

    struct Osim_mesh {
        osim::Mesh data;
        Triangle_mesh mesh;

        Osim_mesh(gl::Attribute& in_attr, gl::Attribute& in_normal, osim::Mesh _data) :
            data{std::move(_data)},
            mesh{make_mesh(in_attr, in_normal, data)} {
        }
    };

    struct ModelState {
        std::vector<osim::Cylinder> cylinders;
        std::vector<Line> lines;
        std::vector<osim::Sphere> spheres;
        std::vector<Osim_mesh> meshes;
    };

    ModelState load_model(App_static_glstate& gls, std::string_view path) {
        ModelState rv;
        for (osim::Geometry const& g : osim::geometry_in(path)) {
            std::visit(overloaded {
                [&](osim::Cylinder const& c) {
                    rv.cylinders.push_back(c);
                },
                [&](osim::Line const& l) {
                    rv.lines.emplace_back(gls.location, l);
                },
                [&](osim::Sphere const& s) {
                    rv.spheres.push_back(s);
                },
                [&](osim::Mesh const& m) {
                    rv.meshes.emplace_back(gls.location, gls.in_normal, m);
                }
            }, g);
        }
        return rv;
    }

    struct ScreenDims {
        int w = 0;
        int h = 0;

        ScreenDims(std::pair<int, int> p) :
            w{p.first}, h{p.second} {
        }
    };

    struct Window_dimensions {
        int w;
        int h;
    };

    Window_dimensions GetWindowSize(SDL_Window* w) {
        Window_dimensions d;
        sdl::GetWindowSize(w, &d.w, &d.h);
        return d;
    }

    void show(ui::State& s, std::string file) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);
        OSC_GL_CALL_CHECK(glEnable, GL_BLEND);
        OSC_GL_CALL_CHECK(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // ImGUI
        auto imgui_ctx = ig::Context{};
        auto imgui_sdl2_ctx = ig::SDL2_Context{s.window, s.gl};
        auto imgui_ogl3_ctx = ig::OpenGL3_Context{OSC_GLSL_VERSION};
        ImGui::StyleColorsLight();
        ImGuiIO& io = ImGui::GetIO();

        // Unchanging OpenGL state (which programs are used, uniforms, etc.)
        App_static_glstate gls = initialize();

        // Mutable runtime state
        ModelState ms = load_model(gls, file);

        bool wireframe_mode = false;
        Window_dimensions window_dims = GetWindowSize(s.window);
        float radius = 1.0f;
        float wheel_sensitivity = 0.9f;
        float line_width = 0.002f;

        float fov = 120.0f;

        bool dragging = false;
        float theta = 0.0f;
        float phi = 0.0f;
        float sensitivity = 1.0f;

        bool panning = false;
        glm::vec3 pan = {0.0f, 0.0f, 0.0f};

        // initial pan position is the average center of *some of the* geometry
        // in the scene, which is found in an extremely dumb way.
        if (true) {  // auto-centering
            glm::vec3 middle = {0.0f, 0.0f, 0.0f};
            unsigned n = 0;
            auto update_middle = [&](glm::vec3 const& v) {
                middle = glm::vec3{
                    (n*middle.x - v.x)/(n+1),
                    (n*middle.y - v.y)/(n+1),
                    (n*middle.z - v.z)/(n+1)
                };
                ++n;
            };

            for (auto const& l : ms.lines) {
                update_middle(l.data.p1);
                update_middle(l.data.p2);
            }

            for (auto const& p : ms.spheres) {
                glm::vec3 translation = {p.transform[3][0], p.transform[3][1], p.transform[3][2]};
                update_middle(translation);
            }

            pan = middle;
        }

        auto light_pos = glm::vec3{1.0f, 1.0f, 0.0f};
        float light_color[3] = {0.98f, 0.95f, 0.95f};
        bool show_light = false;
        bool show_unit_cylinder = false;

        bool gamma_correction = false;
        bool user_gamma_correction = gamma_correction;
        if (gamma_correction) {
            OSC_GL_CALL_CHECK(glEnable, GL_FRAMEBUFFER_SRGB);
        } else {
            OSC_GL_CALL_CHECK(glDisable, GL_FRAMEBUFFER_SRGB);
        }

        auto last_render_timepoint = std::chrono::high_resolution_clock::now();
        auto min_delay_between_frames = 8ms;

        while (true) {
            auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{ 0.0f, 1.0f, 0.0f });
            auto theta_vec = glm::normalize(glm::vec3{ sin(theta), 0.0f, cos(theta) });
            auto phi_axis = glm::cross(theta_vec, glm::vec3{ 0.0, 1.0f, 0.0f });
            auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
            auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
            float aspect_ratio = static_cast<float>(window_dims.w) / static_cast<float>(window_dims.h);

            // event loop
            SDL_Event e;
            SDL_WaitEvent(&e);
            do {
                ImGui_ImplSDL2_ProcessEvent(&e);
                if (e.type == SDL_QUIT) {
                    return;
                } else if (e.type == SDL_KEYDOWN) {
                    switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        return;  // quit visualizer
                    case SDLK_w:
                        wireframe_mode = not wireframe_mode;
                        break;
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
                        continue;
                    }

                    if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
                        // probably a frameskip or the mouse was forcibly teleported
                        // because it hit the edge of the screen
                        continue;
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

                    if (dragging or panning) {
                        // wrap mouse if it hits edges
                        constexpr int edge_width = 5;
                        if (e.motion.x + edge_width > window_dims.w) {
                            SDL_WarpMouseInWindow(s.window, edge_width, e.motion.y);
                        }
                        if (e.motion.x - edge_width < 0) {
                            SDL_WarpMouseInWindow(s.window, window_dims.w - edge_width, e.motion.y);
                        }
                        if (e.motion.y + edge_width > window_dims.h) {
                            SDL_WarpMouseInWindow(s.window, e.motion.x, edge_width);
                        }
                        if (e.motion.y - edge_width < 0) {
                            SDL_WarpMouseInWindow(s.window, e.motion.x, window_dims.h - edge_width);
                        }
                    }
                } else if (e.type == SDL_WINDOWEVENT) {
                    window_dims = GetWindowSize(s.window);
                    glViewport(0, 0, window_dims.w, window_dims.h);
                } else if (e.type == SDL_MOUSEWHEEL) {
                    if (e.wheel.y > 0 and radius >= 0.1f) {
                        radius *= wheel_sensitivity;
                    }

                    if (e.wheel.y <= 0 and radius < 100.0f) {
                        radius /= wheel_sensitivity;
                    }
                }
            } while (SDL_PollEvent(&e) == 1);

            if (user_gamma_correction != gamma_correction) {
                if (user_gamma_correction) {
                    OSC_GL_CALL_CHECK(glEnable, GL_FRAMEBUFFER_SRGB);
                } else {
                    OSC_GL_CALL_CHECK(glDisable, GL_FRAMEBUFFER_SRGB);
                }
                gamma_correction = user_gamma_correction;
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

            gl::UseProgram(gls.program);

            // set *invariant* uniforms
            {
                auto proj_matrix = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
                // camera: at a fixed position pointing at a fixed origin. The "camera" works by translating +
                // rotating all objects around that origin. Rotation is expressed as polar coordinates. Camera
                // panning is represented as a translation vector.
                auto camera_pos = glm::vec3(0.0f, 0.0f, radius);
                glglm::Uniform(gls.projMat, proj_matrix);
                glglm::Uniform(gls.viewMat,
                               glm::lookAt(camera_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) * rot_theta * rot_phi * pan_translate);
                glglm::Uniform(gls.light_pos, light_pos);
                glglm::Uniform(gls.light_color, glm::vec3(light_color[0], light_color[1], light_color[2]));
                glglm::Uniform(gls.view_pos, glm::vec3{radius * sin(theta) * cos(phi), radius * sin(phi), radius * cos(theta) * cos(phi)});
            }

            for (auto const& c : ms.cylinders) {
                gl::BindVertexArray(gls.cylinder.vao);
                glglm::Uniform(gls.rgba, c.rgba);

                auto scaler = glm::scale(c.transform, c.scale);
                glglm::Uniform(gls.modelMat, scaler);
                glDrawArrays(GL_TRIANGLES, 0, gls.cylinder.num_verts);
                gl::BindVertexArray();
            }

            for (auto const& c : ms.spheres) {
                gl::BindVertexArray(gls.sphere.vao);
                glglm::Uniform(gls.rgba, c.rgba);
                auto scaler = glm::scale(c.transform, glm::vec3{c.radius, c.radius, c.radius});
                glglm::Uniform(gls.modelMat, scaler);
                glDrawArrays(GL_TRIANGLES, 0, gls.sphere.num_verts);
                gl::BindVertexArray();
            }

            for (auto& l : ms.lines) {
                gl::BindVertexArray(gls.cylinder.vao);

                // color
                glglm::Uniform(gls.rgba, l.data.rgba);

                glm::vec3 p1_to_p2 = l.data.p2 - l.data.p1;
                glm::vec3 c1_to_c2 = glm::vec3{0.0f, 0.0f, 2.0f};
                auto rotation =
                        glm::rotate(glm::identity<glm::mat4>(),
                                    glm::acos(glm::dot(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2))),
                                    glm::cross(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2)));
                float scale = glm::length(p1_to_p2)/glm::length(c1_to_c2);
                auto scale_xform = glm::scale(glm::identity<glm::mat4>(), glm::vec3{line_width, line_width, scale});
                auto translation = glm::translate(glm::identity<glm::mat4>(), l.data.p1 + p1_to_p2/2.0f);

                glglm::Uniform(gls.modelMat, translation * rotation * scale_xform);
                glDrawArrays(GL_TRIANGLES, 0, gls.cylinder.num_verts);

                gl::BindVertexArray();
            }

            for (auto& m : ms.meshes) {
                gl::BindVertexArray(m.mesh.vao);
                glglm::Uniform(gls.rgba, m.data.rgba);
                auto scaler = glm::scale(m.data.transform, m.data.scale);
                glglm::Uniform(gls.modelMat, scaler);
                glDrawArrays(GL_TRIANGLES, 0, m.mesh.num_verts);
                gl::BindVertexArray();
            }

            // draw lamp
            if (show_light) {
                gl::BindVertexArray(gls.sphere.vao);
                glglm::Uniform(gls.rgba, glm::vec4{1.0f, 1.0f, 0.0f, 0.3f});
                glglm::Uniform(gls.modelMat, glm::scale(glm::translate(glm::identity<glm::mat4>(), light_pos), {0.05, 0.05, 0.05}));
                glDrawArrays(GL_TRIANGLES, 0 , gls.sphere.num_verts);
                gl::BindVertexArray();
            }

            if (show_unit_cylinder) {
                gl::BindVertexArray(gls.cylinder.vao);
                glglm::Uniform(gls.rgba, glm::vec4{0.9f, 0.9f, 0.9f, 1.0f});
                glglm::Uniform(gls.modelMat, glm::identity<glm::mat4>());
                glDrawArrays(GL_TRIANGLES, 0 , gls.cylinder.num_verts);
                gl::BindVertexArray();
            }

            gl::UseProgram();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(s.window);

            ImGui::NewFrame();
            bool b = true;
            ImGui::Begin("Scene", &b, ImGuiWindowFlags_MenuBar);

            {
                std::stringstream fps;
                fps << "Fps: " << io.Framerate;
                ImGui::Text(fps.str().c_str());
            }
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
            ImGui::SliderFloat("line_width", &line_width, 0.0f, 0.01f);
            ImGui::Checkbox("gamma_correction", &user_gamma_correction);

            ImGui::NewLine();
            ImGui::Text("Interaction:");
            if (dragging) {
                ImGui::Text("rotating");
            }
            if (panning) {
                ImGui::Text("panning");
            }

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // software-throttle the framerate: no need to render at an insane
            // (e.g. 2000 FPS, on my machine) FPS, but do not use VSYNC because
            // it makes the entire application feel *very* laggy.
            auto now = std::chrono::high_resolution_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_timepoint);
            if (dt < min_delay_between_frames) {
                SDL_Delay(static_cast<Uint32>((min_delay_between_frames - dt).count()));
            }

            // draw
            SDL_GL_SwapWindow(s.window);
            last_render_timepoint = std::chrono::high_resolution_clock::now();
        }
    }
}

static const char usage[] = "usage: osmv model.osim [other_models.osim]";

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cerr << usage << std::endl;
        return EXIT_FAILURE;
    }

    auto ui = ui::State{};;

    for (int i = 1; i < argc; ++i) {
        examples::imgui::show(ui, argv[i]);
    }

    return EXIT_SUCCESS;
};
