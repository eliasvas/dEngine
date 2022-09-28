mkdir -p build/shaders
cd assets/shaders

for f in *.vert
do
	echo "Compiling $f"
	../../ext/glslang/StandAlone/glslangValidator -V "$f" -o ../../build/shaders/"$f".spv
done

for f in *.frag
do
	echo "Compiling $f"
	../../ext/glslang/StandAlone/glslangValidator -V "$f" -o ../../build/shaders/"$f".spv
done

for f in *.geom
do
	echo "Compiling $f"
	../../ext/glslang/StandAlone/glslangValidator -V "$f" -o ../../build/shaders/"$f".spv
done


cd ..
cd ..
