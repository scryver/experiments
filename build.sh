#!/bin/bash

set -e

curDir="$(pwd)"
codeDir="$curDir/src"
buildDir="$curDir/gebouw"

flags="-O2 -g -ggdb -Wall -Werror -pedantic -std=c++11 -pthread"

exceptions="-Wno-unused-function -Wno-writable-strings -Wno-gnu-anonymous-struct -Wno-nested-anon-types"

mkdir -p "$buildDir"

pushd "$buildDir" > /dev/null
#    clang++ $flags $exceptions "$codeDir/randomness.cpp" -o randomness -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/mazer.cpp" -o mazer -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/salesman.cpp" -o salesman -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/lexicographorder.cpp" -o lexico
#    clang++ $flags $exceptions "$codeDir/physics.cpp" -o physics -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/forces.cpp" -o forces -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/polar.cpp" -o polar -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/oscillation.cpp" -o oscillation -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/pendulum.cpp" -o pendulum -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/springs.cpp" -o springs -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/particles.cpp" -o particles -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/simple_vehicle.cpp" -o simple_vehicle -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/flowfield.cpp" -o flowfield -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/perlin.cpp" -o perlin -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/gameoflife.cpp" -o game-of-life -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/fractal01.cpp" -o fractal01 -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/lsystem.cpp" -o lsystem -lX11 -lGL
#    clang++ $flags $exceptions "$codeDir/mandelbrot.cpp" -o mandelbrot -lX11 -lGL
    clang++ $flags $exceptions "$codeDir/mousemove.cpp" -o mousemove -lX11 -lGL
popd > /dev/null

