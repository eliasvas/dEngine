cd build
#gcc -c ../src/fiber_context.s
gcc -g -w ../src/*.c ../ext/microui/microui.c ../ext/spirv_reflect/spirv_reflect.c -I../ext/SDL2/include/ -I../ext/ -L../build/ -L/usr/lib/x86_64-linux-gnu/ -L/usr/lib/x86_64-linux-gnu/plymouth/renderers/ -Wl,-Bstatic -lSDL2 -Wl,-Bdynamic -lasound -lm -ldl -lpthread -lpulse-simple -pthread -lpulse -pthread -lX11 -lXext -lXcursor -lXinerama -lXi -lXfixes -lXrandr -lXss -lXxf86vm -ldrm -lgbm -lwayland-egl -lwayland-client -lwayland-cursor -lxkbcommon -ldecor-0 -lpthread -lrt  -o engine 
cd ..
