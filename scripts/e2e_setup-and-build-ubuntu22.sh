#!/usr/bin/env bash

sudo apt-get install gcc-12 g++-12
CC=gcc-12 CXX=g++-12 ./scripts/e2e_setup-and-build-ubuntu22.sh

