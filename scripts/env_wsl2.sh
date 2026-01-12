# Sets up the calling WSL2 bash terminal to use WSL2 optimally.
#
#     usage (WSL2 unix terminal only): `source env_wsl2.sh`

# LIBGL_ALWAYS_SOFTWARE=1  # Force OSC to render via Mesa (software)

# Force mesa to use Microsoft's D3D12 Mesa extension that effectively
# forwards Mesa (OpenGL) draw commands via the host machine's NVIDIA/intel
# hardware graphics card
#
# (this can be effectively necessary for high-perf rendering via WSL)
export MESA_D3D12_DEFAULT_ADAPTER_NAME=nvidia  #  alternatively, intel

# A session DBUS must be launched in order for the file dialogs to work.
eval $(dbus-launch --sh-syntax)
