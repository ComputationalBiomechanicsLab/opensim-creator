%module(directors="1") opensimExampleComponents
%module opensimExampleComponents

#pragma SWIG nowarn=822,451,503,516,325,401

%{
#include <Bindings/OpenSimHeaders_common.h>
#include <Bindings/OpenSimHeaders_actuators.h>
#include <Bindings/OpenSimHeaders_examplecomponents.h>
#include <Bindings/Java/OpenSimJNI/OpenSimContext.h>

using namespace OpenSim;
using namespace SimTK;
%}

%include "java_preliminaries.i";

%import "java_actuators.i"

%include <Bindings/examplecomponents.i>
