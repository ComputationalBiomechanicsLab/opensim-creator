#!/usr/bin/env bash 

hdiutil attach osc-build/osc-X.X.X-Darwin.dmg
rm -rf /Applications/osc.app && cp -r /Volumes/osc-X.X.X-Darwin/osc.app /Applications

