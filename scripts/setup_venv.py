#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path

def main():
    venv_path = Path(".venv")
    requirements_file_path = Path("requirements") / "all_requirements.txt"

    # Create virtual environment if it doesn't exist yet
    #
    # An empty directory should be treated as "not created yet" also
    # so that systems like docker can mask the host's venv
    if not venv_path.exists() or not any(venv_path.iterdir()):
        print(f"Creating project virtual environment at {venv_path}")
        subprocess.run([sys.executable, "-m", "venv", str(venv_path)], check=True)
        print("Virtual environment created.")
    else:
        print("Virtual environment already exists.")

    # Find pip inside virtual environment (OS-dependent)
    windows_pip_path = venv_path / "Scripts" / "pip.exe"
    unix_pip_path = venv_path / "bin" / "pip"
    pip_path = windows_pip_path if windows_pip_path.exists() else unix_pip_path

    # Install OPynSim dependencies into the environment
    print(f"Installing OPynSim dependencies from {requirements_file_path}")
    subprocess.run([str(pip_path), "install", "-r", str(requirements_file_path)], check=True)
    print("OPynSim Python dependencies installed successfully")

if __name__ == "__main__":
    main()
