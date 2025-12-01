// The reason <Python.h> is included this way is because Visual Studio will
// look for (e.g.) `python313_d.lib` if building in debug mode, which won't
// work for typical installs of python
#ifdef _DEBUG
    #undef _DEBUG
    #include <Python.h>
    #define _DEBUG
#else
    #include <Python.h>
#endif

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    // Helper: slurp a file into a `std::string`
    std::string read_file(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary); // binary avoids newline translation
        if (not file) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Helper: set an environment variable called `name` to `value`
    void set_env(const std::string& name, const std::string& value)
    {
#if defined(_WIN32)
        const std::string env = name + '=' + value;
        _putenv(env.c_str());
#else
        setenv(name.c_str(), value.c_str(), 1);
#endif
    }

    // Helper: set PYTHONPATH to the given paths
    void set_pythonpath(std::vector<std::filesystem::path> paths)
    {
#if defined(_WIN32)
        constexpr std::string_view separator = ";";
#else
        constexpr std::string_view separator = ":";
#endif
        std::string value;
        std::string_view delim;
        for (const auto& path : paths) {
            value += delim;
            value += path.string();
            delim = separator;
        }
        set_env("PYTHONPATH", value);
    }
}

// This is what's ran when running `opynsim_debugger.exe`
int main()
{
    // Set PYTHONPATH to the local virtual environment and `opynsim`
    set_pythonpath({ "@OPYN_VENV@/Lib/site-packages", "@CMAKE_CURRENT_SOURCE_DIR@/.." });

    // Set the current working directory to the debugger's source directory (it's where
    // developers will probably dump data files etc. during development).
    std::filesystem::current_path("@CMAKE_CURRENT_SOURCE_DIR@");

    // Read the source `debugscript.py`, ready for the Python interpreter
    std::string code = read_file("@CMAKE_CURRENT_SOURCE_DIR@/debugscript.py");

    // Initialize the Python interpreter and pump the script into it
    Py_Initialize();
    int result = 0;
    if (PyRun_SimpleString(code.c_str()) != 0) {
        PyErr_Print();
        result = 1;
    }
    Py_Finalize();
    return result;
}
