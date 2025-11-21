#!/bin/bash

git submodule init

git submodule update --recursive

./build.sh