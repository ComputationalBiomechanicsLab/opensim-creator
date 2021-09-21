#!/usr/bin/env bash 

hdiutil attach osc-build/osc-0.0.4-Darwin.dmg
rm -rf /Applications/osc.app && cp -r /Volumes/osc-0.0.4-Darwin/osc.app /Applications

