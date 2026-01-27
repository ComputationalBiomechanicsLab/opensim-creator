#!/usr/bin/env bash

set -xeuo pipefail

rm -rf third_party/opynsim
git clone https://github.com/opynsim/opynsim third_party/opynsim
rm -rf third_party/opynsim/third_party/oscar
git clone https://github.com/adamkewley/oscar third_party/opynsim/third_party/oscar
