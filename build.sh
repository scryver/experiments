#!/bin/bash

set -e

WITH_EXPECT=1
WITH_FFTW=1

curDir="$(pwd)"
codeDir="$curDir/src"
testDir="$curDir/test"
buildDir="$curDir/gebouw"
fftwDir="$HOME/Programs/fftw-3.3.8/api"

flags="-O3 -g -ggdb -Wall -Werror -pedantic -std=c++11 -pthread -msse4 -DLIBBERDIP_EXPECT=$WITH_EXPECT"

exceptions="-Wno-unused-function -Wno-writable-strings -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-missing-braces -Wno-gnu-zero-variadic-macro-arguments -Wno-c99-extensions"
exceptions="$exceptions -Wno-unused-variable"

echo "Start build..."

mkdir -p "$buildDir"

pushd "$buildDir" > /dev/null
#    clang++ $flags $exceptions "$codeDir/precompile.cpp" -o precompile
#    ./precompile "$codeDir/flowfield.cpp"

cat > /dev/null << IGNORE_ME
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
    clang++ $flags $exceptions "$codeDir/flowers.cpp" -o flowers -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mandelbrot.cpp" -o mandelbrot -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/mandelbrotset.cpp" -o mandelbrotset -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/blackbox.cpp" -o blackbox -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/phasespace.cpp" -o phase-space -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/fluider.cpp" -o fluider -lX11 -lGL &

    clang++ $flags $exceptions -I/usr/include/harfbuzz $(pkg-config --cflags freetype2) "$codeDir/freefont.cpp" -o freefont $(pkg-config --libs freetype2) -lharfbuzz -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/stbfont.cpp" -o stbfont -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/stbfont2.cpp" -o stbfont2 -lX11 -lGL &

    clang++ $flags $exceptions "$testDir/test_neuron.cpp" -o test-neuron
    clang++ $flags $exceptions "$testDir/test_neural_net.cpp" -o test-neural-net
    clang++ $flags $exceptions "$testDir/test_neural_layer.cpp" -o test-neural-layer

    clang++ $flags $exceptions "$codeDir/selector.cpp" -o selector -lX11 -lGL &
IGNORE_ME

    clang++ $flags $exceptions "$codeDir/maurerrose.cpp" -o maurer-rose -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/rdplinesimplefy.cpp" -o rdp-line -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/collatzconjecture.cpp" -o collatz-conjecture -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/tictactoe.cpp" -o tic-tac-toe -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/hilbertcurve.cpp" -o hilbert-curve -lX11 -lGL &
    clang++ $flags $exceptions -I"$fftwDir" "$codeDir/fft.cpp" -o fft -lX11 -lGL -L$HOME/Programs/fftw-3.3.8/build/.libs -lfftw3f -lm &
if [ $WITH_FFTW -ne 0 ]; then
    clang++ $flags $exceptions -I"$fftwDir" "$codeDir/fft_memaccess.cpp" -o fft-memaccess -lX11 -lGL -L$HOME/Programs/fftw-3.3.8/build/.libs -lfftw3f -lm &
else
    clang++ $flags $exceptions -DWITH_FFTW=0 "$codeDir/fft_memaccess.cpp" -o fft-memaccess -lX11 -lGL &
fi
    clang++ $flags $exceptions "$codeDir/bitreversal.cpp" -o bitreversal &
    clang++ $flags $exceptions "$codeDir/clampline.cpp" -o clamp-line &
    clang++ $flags $exceptions "$codeDir/drawfft.cpp" -o draw-fft &
    clang++ $flags $exceptions "$codeDir/starpatterns.cpp" -o star-patterns -lX11 -lGL &
#    clang++ $flags $exceptions "$codeDir/memory_test.cpp" -o memory-test &
    clang++ $flags $exceptions "$codeDir/raymarching2d.cpp" -o ray-marching2d -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/raymarching3d.cpp" -o ray-marching3d -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/raymarchingshadertoy.cpp" -o ray-marching-shadertoy -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/pixeldiff.cpp" -o pixel-diff-visual -lX11 -lGL &
    clang++ $flags $exceptions "$codeDir/breakout.cpp" -o breakout -lX11 -lGL &

popd > /dev/null

#flagsC="-O0 -g -ggdb -Wall -Werror -pedantic -pthread"
#clang $flagsC $exceptions "$testDir/test_waves.c" -o test-waves -lX11 -lm &

wait
