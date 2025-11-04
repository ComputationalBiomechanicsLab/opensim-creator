#!/usr/bin/env bash
#
# Sets up a virtual environment at `docs/venv/` with the necessary python
# dependencies to build the documentation.

python -m venv docs/venv/

# Determine which `activate` script to `source`
if [ -f "docs/venv/bin/activate" ]; then
    source "docs/venv/bin/activate"      # Linux/macOS
elif [ -f "docs/venv/Scripts/activate" ]; then
    source "docs/venv/Scripts/activate"  # Windows (Git Bash, Cygwin, WSL)
else
    echo "ERROR: Could not find a valid Python virtual environment in 'docs/venv/'."
    exit 1
fi

pip install -r docs/requirements.txt -r docs/requirements-dev.txt
