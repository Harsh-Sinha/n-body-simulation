#!/bin/bash

python3 -m venv .venv

# deactivate any existing virtual environment
deactivate 2>/dev/null

source .venv/bin/activate

pip install matplotlib