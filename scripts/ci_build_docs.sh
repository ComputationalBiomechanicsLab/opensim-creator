#!/usr/bin/env bash

python -m venv .venv && source .venv/Scripts/activate && pip install -r requirements/all_requirements.txt && sphinx-build docs/source build/docs
