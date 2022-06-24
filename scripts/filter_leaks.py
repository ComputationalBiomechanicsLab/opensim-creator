#!/usr/bin/env python3

import re
import sys

blacklist = [
    # on application init (osc)
    "/lib/x86_64-linux-gnu/libGLX_nvidia.so.0+0xb1c55",  # my graphics driver leaks stuff on SDL_Init(SDL_INIT_VIDEO);

    # when opening a file dialog
    "libgtk-3",  # nativefiledialog leaks via GTK on linux?
    "libpango",  # also nativefiledialog
    "libfontconfig.so.1",  # also nativefiledialog
    "libglib-2.0",  # also nativefiledialog

    # during static init (object registration)
    "OpenSim::CMC_TaskSet::CMC_TaskSet",   # this thing leaks stuff in the object registry on static init
    "OpenSim::Delp1990Muscle_Deprecated",  # this thing leaks stuff in the object registry on static init

    # when loading an osim
    "OpenSim::Object::updateFromXMLDocument()",  # SimTK's XML parser leaks strings
    "SimTK::TiXmlDocument::LoadFile(char const*, SimTK::TiXmlEncoding)",  # SimTK's XML parser leaks when loading a model file

    # when loading, or copying, an OpenSim::Model
    "OpenSim::CoordinateSet::CoordinateSet(OpenSim::CoordinateSet const&)",  # CooordinateSet leaks whenever model is copied

    # with more complicated models:
    "OpenSim::CoordinateReference::CoordinateReference(OpenSim::CoordinateReference const&)",  # OpenSim/Simulation/CoordinateReference.cpp
    "OpenSim::AssemblySolver::updateCoordinateReference(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, double, double)",  # OpenSim/Simulation/AssemblySolver.cpp:157
]

pattern = re.compile("leak of (\\d+) byte")

blacklist_buf = {}

def bytes_leaked(lines):
    for l in lines:
        m = re.search(pattern, l)
        if m:
            return int(m.group(1))
    return 0

def dump_leak(lines):
    for l in lines:
        for b in blacklist:
            if b in l:
                v = blacklist_buf.get(b, 0)
                v += bytes_leaked(lines)
                blacklist_buf[b] = v
                return

    print("---")
    for l in lines:
        print(l, end='')
    print("---")

leak_lines = []
for line in sys.stdin:
    leak_lines += [line]
    if "leak of" in line or "SUMMARY" in line:
       dump_leak(leak_lines)
       leak_lines = []
    if "SUMMARY" in line:
        print(line, end='')
        break

for el,bs in blacklist_buf.items():
    print(f"{el}    {bs}")

