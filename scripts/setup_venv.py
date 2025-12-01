#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path

def main():
    venv_path = Path(".venv")
    requirements_file_path = Path("requirements") / "all_requirements.txt"
    opynsim_source_path = Path("opynsim").resolve()

    # Create virtual environment if it doesn't exist yet
    if not venv_path.exists():
        print(f"Creating project virtual environment at {venv_path}")
        subprocess.run([sys.executable, "-m", "venv", str(venv_path)], check=True)
        print("Virtual environment created.")
    else:
        print("Virtual environment already exists.")

    # Find pip inside virtual environment (OS-dependent)
    windows_pip_path = venv_path / "Scripts" / "pip.exe"
    unix_pip_path = venv_path / "bin" / "pip"
    pip_path = windows_pip_path if windows_pip_path.exists() else unix_pip_path

    # Install dependencies into the environment
    print(f"Installing dependencies from {requirements_file_path}")
    subprocess.run([str(pip_path), "install", "-r", str(requirements_file_path)], check=True)
    print("Python dependencies installed successfully")

    # Install `opynsim` as an __editable__ package in the environment, so that
    # various debuggers, test runners, etc. don't need to be separately configured
    subprocess.run([str(pip_path), "install", "-e", str(opynsim_source_path)], check=True)
    print(f"Successfully linked the python virtual environment to {opynsim_source_path}")

if __name__ == "__main__":
    main()
