#!/bin/bash

set -e

curDir="$(pwd)"
codeDir="$curDir/src"
testDir="$curDir/test"
buildDir="$curDir/gebouw"

flags="-O2 -g -ggdb -Wall -Werror -pedantic -std=c++11 -pthread"

exceptions="-Wno-unused-function -Wno-writable-strings -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-missing-braces -Wno-gnu-zero-variadic-macro-arguments -Wno-c99-extensions"

echo "Start build..."

mkdir -p "$buildDir"

pushd "$buildDir" > /dev/null
#    clang++ $flags $exceptions "$codeDir/precompile.cpp" -o precompile
#    ./precompile "$codeDir/flowfield.cpp"

    clang++ $flags $exceptions "$codeDir/randomness.cpp" -o randomness -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mazer.cpp" -o mazer -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/salesman.cpp" -o salesman -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/lexicographorder.cpp" -o lexico &
    clang++ $flags $exceptions "$codeDir/physics.cpp" -o physics -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/forces.cpp" -o forces -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/polar.cpp" -o polar -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/oscillation.cpp" -o oscillation -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/pendulum.cpp" -o pendulum -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/springs.cpp" -o springs -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/particles.cpp" -o particles -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/simple_vehicle.cpp" -o simple-vehicle -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/flowfield.cpp" -o flowfield -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/perlin.cpp" -o perlin -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/gameoflife.cpp" -o game-of-life -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/fractal01.cpp" -o fractal01 -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/lsystem.cpp" -o lsystem -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mandelbrot.cpp" -o mandelbrot -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mousemove.cpp" -o mousemove -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/evolving_vehicle.cpp" -o evolving-vehicle -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/evolving_salesman.cpp" -o evolving-salesman -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/neural_network.cpp" -o neural-network -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/colour_predict.cpp" -o colour-predict -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/flappy_brain.cpp" -o flappy-brain -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/lissajous_curve.cpp" -o lissajous-curve -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/chaos_game.cpp" -o chaos-game -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/path_finding.cpp" -o path-finding -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/hexmap.cpp" -o hexmap -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mnist_parser.cpp" -o mnist-parser -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/handwriting.cpp" -o handwriting -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/neuroning.cpp" -o neuroning -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/neurons_calc.cpp" -o neurons-calc -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/handlayered.cpp" -o handlayered -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/convoluted.cpp" -o convoluted -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/acoustics1.cpp" -o acoustics1 -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/acoustics2.cpp" -o acoustics2 -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mechanics1.cpp" -o mechanics1 -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/x11mouse.cpp" -o x11mouse -lX11 &
    clang++ $flags $exceptions "$codeDir/x11bitmap.cpp" -o x11bitmap -lX11 &
    clang++ $flags $exceptions "$codeDir/logo.cpp" -o logo -lX11 -lGL &
#    clang++ $flags $exceptions "$codeDir/yatzee.cpp" -o yatzee -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/fxer.cpp" -o fxer -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/fluids.cpp" -o fluids -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/strandbeest.cpp" -o strandbeest -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/raycast.cpp" -o raycast -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/raytrace.cpp" -o raytrace -lX11 -lGL &

    clang++ $flags $exceptions "$testDir/test_neuron.cpp" -o test-neuron
    clang++ $flags $exceptions "$testDir/test_neural_net.cpp" -o test-neural-net
    clang++ $flags $exceptions "$testDir/test_neural_layer.cpp" -o test-neural-layer

    clang++ $flags $exceptions "$codeDir/selector.cpp" -o selector -lX11 -lGL &
popd > /dev/null

#flagsC="-O0 -g -ggdb -Wall -Werror -pedantic -pthread"
#clang $flagsC $exceptions "$testDir/test_waves.c" -o test-waves -lX11 -lm &
