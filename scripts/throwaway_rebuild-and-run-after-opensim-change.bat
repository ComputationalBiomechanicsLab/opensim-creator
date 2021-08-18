REM rebuild opensim (e.g. because FBP has been updated)
cd opensim-build
cmake --build . --config RelWithDebInfo --target install  || exit /b
cd .. 

REM rebuilt OSC (in case a header changed and OSC needs to know about it)
cd osc-build 
cmake --build . --config RelWithDebInfo || exit /b
cd ..

REM run OSC and show suitable model file
osc-build\RelWithDebInfo\osc.exe resources\models\ToyLanding\ToyLandingModel.osim