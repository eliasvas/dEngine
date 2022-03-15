cd build
#gcc -c ../src/fiber_context.s
gcc -g ../src/*.c ../build/fiber_context.o -pthread -I../ext/SDL2/include/ -I../ext/ -L../build -ldl -lSDL2 -o engine