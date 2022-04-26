cd build
#gcc -c ../src/fiber_context.s
gcc -g ../src/*.c ../ext/microui/microui.c ../ext/spirv_reflect/spirv_reflect.c  -pthread -I../ext/SDL2/include/ -I../ext/ -L../build -lm -ldl -lSDL2 -o engine 
cd ..
