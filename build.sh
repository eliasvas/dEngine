cd build
#gcc -c ../src/fiber_context.s
cc -g ../src/*.c ../build/fiber_context.o ../ext/spirv_reflect/spirv_reflect.c  -pthread -I../ext/SDL2/include/ -I../ext/ -L../build -ldl -lSDL2 -o engine 
