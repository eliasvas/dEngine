# dEngine
## _A Game Engine from Scratch!_
[![Build Status](https://travis-ci.org/joemccann/dillinger.svg?branch=master)](https://travis-ci.org/joemccann/dillinger)

dEngine is a cross-platform game engine made the old-school way.
A game might be made with the engine down the road, maybe.

## Features
- Multithreaded Vulkan Rendering Backend
- Multithreaded Entity Component System + Job System
- Physically Based Renderer w/ Skeletal Animations
- Real-Time Profiling
- Cascaded Shadow Maps
## Building
Building is very easy since pretty much every dependency except Vulkan is statically linked.
There are different build scripts for Linux and Windows, but the process is as follows:
```sh
cd engine
make
cd build
./engine
```
(Optional): If you want faster shader compilation you can precompile all shaders from glsl to SPIR-V at build time, so you don't need to do it at runtime. All you have to do is execute the *compile_shaders.sh* script.
```sh
cd build
mkdir shaders
cd ..
./compile_shaders.sh
```
After that there should be a directory called */shaders* inside */build* that contains all shaders in SPIR-V format.

## Dependencies
- Vulkan 1.3 (update your drivers!)
- shaderc (optional)


