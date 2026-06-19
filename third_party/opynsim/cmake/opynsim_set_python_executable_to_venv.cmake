function(opyn_set_python_executable_to_venv venv_path)
    if(WIN32)
        set(_python_exe "${venv_path}/Scripts/python.exe")
    else()
        set(_python_exe "${venv_path}/bin/python")
    endif()

    if(NOT EXISTS "${_python_exe}")
        message(FATAL_ERROR "${_python_exe}: python executable not found in this virtual environment, but `opyn_set_python_executable_to_venv` was called on it?")
    endif()

    set(Python_EXECUTABLE "${_python_exe}" CACHE FILEPATH "Path to Python interpreter" FORCE)
endfunction()
