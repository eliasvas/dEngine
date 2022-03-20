cd assets/shaders

for f in *.vert
do
	echo "Compiling $f"
	glslc "$f" -o ../../build/shaders/"$f".spv
done

for f in *.frag
do
	echo "Compiling $f"
	glslc "$f" -o ../../build/shaders/"$f".spv
done
cd ..
cd ..
