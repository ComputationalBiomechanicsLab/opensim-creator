# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release amd64 binaries of OpenSim
# Creator on Windows

# Hide window creation in CI, because CI runners typically do not have
# a desktop environment.
$env:OSC_INTERNAL_HIDE_WINDOW="1"

# --system-version is necessary because, otherwise, the wrong Windows
# SDK might be chosen, resulting in either missing headers (e.g. https://github.com/actions/runner-images/issues/10980)
# or shipping a binary that doesn't run on target systems.
#
# The specified SDK supports Windows 10, version 1507 or higher (developer.microsoft.com/en-us/windows/downloads/windows-sdk/)
python scripts\\build_windows.py --system-version=10.0.26100.0
if ($LASTEXITCODE -ne 0) {
    Write-Error "Command failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}
