@echo off


if not exist build mkdir build

cd build
gcc -c ../src/fiber_context.s
gcc -c ../src/fiber_context_msvc.s 
gcc -g -std=c99 ../src/*.c ../build/fiber_context.o  -I../ext/SDL2/include/ -I../ext/ -L../build -lSDL2 -o engine
cd ..