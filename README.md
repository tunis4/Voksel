# Voksel

A shrimple (for now) voxel engine, very WIP

```console
$ git clone --recurse-submodules https://github.com/Tunacan427/Voksel
$ cd Voksel
$ meson setup build
$ meson compile -C build
$ ./build/voksel
```

Libraries used:
- GLFW
- Vulkan
- Vulkan Memory Allocator (VMA) (included)
- GLM
- stb_image (included)
- Dear ImGui (included)
- FastNoise2 (git submodule)
- FreeType2
- [PackedArray](https://github.com/gpakosz/PackedArray) (included)

also the Roboto font is included
