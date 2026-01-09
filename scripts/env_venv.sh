# usage: `source scripts/env_venv.sh`
#
# Sets up a bash terminal to use the project's python virtual environment (assuming
# one was set up with `setup_venv-all` or similar.

# Determine which activate script exists
if [ -f ".venv/bin/activate" ]; then
    source ".venv/bin/activate"      # Linux/macOS
elif [ -f ".venv/Scripts/activate" ]; then
    source ".venv/Scripts/activate"  # Windows (Git Bash, Cygwin, MSYS2, Mingw)
else
    echo "ERROR: Could not find a valid Python virtual environment in '.venv/'."
    exit 1
fi
