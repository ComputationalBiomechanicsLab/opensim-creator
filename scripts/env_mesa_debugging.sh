# these are environment variables for strictly debugging the graphics layer
# with Mesa (Linux)
#
# the idea is that you run OSC via Mesa with various Mesa options enabled so
# that any GPU API errors are kicked out by Mesa (because, sometimes, the
# hardware drivers won't kick out OpenGL bugs)

export INTEL_DEBUG=tex,stall,perf,fail,color,blit
export LIBGL_NO_DRAWARRAYS=true
export LIBGL_DEBUG=verbose
export MESA_DEBUG=flush,incomplete_tex,incomplete_fbo,context
