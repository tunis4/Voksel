# Voksel

a simple voxel engine, very wip

```console
$ git clone --recurse-submodules https://github.com/Tunacan427/Voksel
$ cd Voksel
$ meson setup build
$ meson compile -C build
$ ./build/voksel
```

libraries used:
- glfw
- vulkan
- vulkan memory allocator (vma) (included)
- glm
- stb image (included)
- dear imgui (included)
- fastnoise2 (git submodule)
- freetype2

also the Roboto font is included
