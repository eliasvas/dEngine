@echo off
where /q cl
if ERRORLEVEL 1 (
	    call  "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
	        call  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
	)

cd build
	cl /DDEBUG -nologo -FC /W0 /Zi /EHsc -DBUILD_DEVELOPER=1 -DBUILD_DEBUG=1 -DBUILD_WIN32=1 /Tc ../src/*.c /Tc ../ext/microui/microui.c /Tc ../ext/spirv_reflect/spirv_reflect.c /I ../ext/SDL2/include/ /I ../ext/ /link /LIBPATH:../ext/ /LIBPATH:../build/ -incremental:no -opt:ref user32.lib  SDL2.lib  /OUT:engine.exe 
cd ..
